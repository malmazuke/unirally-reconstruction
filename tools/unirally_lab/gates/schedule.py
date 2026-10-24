"""Run the selected gate steps: in parallel where they do not contend, under temp and disk limits.

Lanes:
- `exclusive`: runs with nothing else (builds, and the synthetic suite, whose timeout probe fails
  under load: memory of 18 September and 14 October 2026);
- `realtime`: the app's real-time runs, together with each other but with nothing else, so their
  late-pair counts are not load artefacts;
- `pool`: everything else, up to `cores` of weight at once (a step that runs its own workers has
  the weight of its workers).
Steps start in catalogue order; an exclusive or real-time step waits for the pool to drain and
holds back the steps after it until it has run.

Each step gets its own `TMPDIR` under the run's temp root and its own output directory. Both, and
anything new in an evidence directory the step is known to write into, are measured every few
seconds; a step
past its cap is stopped and fails. No step starts while free disk is under the floor plus its cap.
"""
from __future__ import annotations

import json
import os
import shutil
import signal
import subprocess
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

from .disk import GB, fmt, free_bytes, tree_bytes


@dataclass
class Step:
    id: str
    title: str
    command: str                       # bash; see Context.env for the variables it can use
    system: str = 'all'                # for the report: build, race, front-end, audio, app, rules
    lane: str = 'pool'                 # exclusive | realtime | pool
    weight: int = 1
    temp_cap: int = 1 * GB
    timeout: int = 3600
    stall: int = 900                   # seconds with no CPU used and nothing written: stopped
    writes: tuple[str, ...] = ()       # other directories it writes into, measured with its temp
    shell: str = 'bash'
    after: tuple[str, ...] = ()        # steps that must have finished first
    reads_status: bool = True          # rc 0 means passed; False: it reports, a reader judges
    # Decided when the step becomes eligible: (run?, why).
    select: Callable[['Run'], tuple[bool, str]] = lambda run: (True, 'always')
    record: bool = False               # record its front_end_runner invocations (corpus)
    needs_identity: bool = True        # its selection reads which binaries changed (after the build)
    # A step that reports against the original gets its verdict from a twin run on main: `twin`
    # names the twin (on the step) or the step (on the twin). The twin's `env` and `cwd`
    # (formatted with the run's variables) point it at main's tree, binaries and pack.
    twin: str = ''
    env: dict = field(default_factory=dict)
    cwd: str = ''


@dataclass
class Outcome:
    id: str
    selected: bool
    why: str
    # passed | failed | ran | skipped | timeout | over-cap | stalled; for a step with a twin on
    # main: same-as-main | changed | changed (expected)
    status: str = 'pending'
    rc: int | None = None
    seconds: float = 0.0
    peak_temp: int = 0
    started: float = 0.0
    summary: list[str] = field(default_factory=list)


class Run:
    def __init__(self, steps: list[Step], env: dict[str, str], root: Path, out: Path, temp_root: Path,
                 cores: int, disk_floor: int, cap_total: int, context: dict, log=print):
        self.steps = steps
        self.by_id = {s.id: s for s in steps}
        self.env = env
        self.root = root
        self.out = out
        self.temp_root = temp_root
        self.cores = cores
        self.disk_floor = disk_floor
        self.cap_total = cap_total
        self.context = context          # identity, replay result, options: read by `select`
        self.log = log
        self.outcomes: dict[str, Outcome] = {}
        self.running: dict[str, tuple[subprocess.Popen, Path, list[Path]]] = {}
        self.lock = threading.Lock()
        self.min_free = free_bytes(out)
        self.start_free = self.min_free
        self.peak_total = 0
        self.callbacks: dict[str, Callable[['Run', Outcome], None]] = {}
        self.progress: dict[str, tuple[float, float, int]] = {}   # id -> (last change, cpu, bytes)

    # -- monitoring ---------------------------------------------------------------------------
    def _monitor(self, stop: threading.Event) -> None:
        while not stop.wait(3):
            total = 0
            with self.lock:
                items = list(self.running.items())
            cpu = group_cpu_seconds()
            now = time.time()
            for sid, (proc, tmp, watched) in items:
                size = tree_bytes(tmp) + sum(new_entries_bytes(d, before) for d, before in watched)
                total += size
                o = self.outcomes[sid]
                o.peak_temp = max(o.peak_temp, size)
                if size > self.by_id[sid].temp_cap and proc.poll() is None:
                    o.status = 'over-cap'
                    self.log(f'  ! {sid}: temp {fmt(size)} over its cap {fmt(self.by_id[sid].temp_cap)}; stopping it')
                    _kill(proc)
                    continue
                # No CPU used by its process group and nothing written: a hang, not a slow step.
                written = size + _size(self.out / f'{sid}.log')
                seen = (cpu.get(proc.pid, 0.0), written)
                last = self.progress.get(sid)
                if last is None or (last[1], last[2]) != seen:
                    self.progress[sid] = (now, *seen)
                elif now - last[0] > self.by_id[sid].stall and proc.poll() is None:
                    o.status = 'stalled'
                    self.log(f'  ! {sid}: no CPU and no output for {now - last[0]:.0f} s; stopping it')
                    _kill(proc)
            self.peak_total = max(self.peak_total, total)
            self.min_free = min(self.min_free, free_bytes(self.out))

    # -- scheduling ---------------------------------------------------------------------------
    def _deps_done(self, step: Step) -> bool:
        # A dependency left out of this run (--only, --skip) does not hold a step back.
        return all(d not in self.by_id or self.outcomes[d].status not in ('pending', 'running') for d in step.after)

    def _start(self, step: Step) -> None:
        o = self.outcomes[step.id]
        tmp = self.temp_root / step.id
        env = dict(self.env, **{k: v.format(**self.env) for k, v in step.env.items()})
        # A twin keeps its step's directory name under its own RUN, so `$RUN/<step>` paths hold.
        out = Path(env['RUN']) / step.id.split('@')[0] if 'RUN' in env else self.out / step.id
        shutil.rmtree(tmp, ignore_errors=True); shutil.rmtree(out, ignore_errors=True)
        tmp.mkdir(parents=True); out.mkdir(parents=True)
        env.update(TMPDIR=str(tmp), G=str(out), STEP=step.id, PYTHONUNBUFFERED='1')
        # Checks against the original are recorded by a recording run; a task's own, every run.
        if step.record and (self.context.get('record') or step.system == 'task') and 'corpus_log' in self.context:
            env['GATES_RECORD'] = str(self.context['corpus_log'](step.id))
        # In an evidence directory only what appears after the start is the step's (its mkdtemp).
        watched = [(out, set())] + [(d, entries(d)) for d in (Path(w.format(**self.env)) for w in step.writes)]
        log = (self.out / f'{step.id}.log').open('w')
        cwd = Path(step.cwd.format(**self.env)) if step.cwd else self.root
        proc = subprocess.Popen([step.shell, '-c', step.command], cwd=cwd, env=env, stdout=log,
                                stderr=subprocess.STDOUT, start_new_session=True)
        log.close()                     # the child keeps its own descriptor
        o.status, o.started = 'running', time.time()
        with self.lock:
            self.running[step.id] = (proc, tmp, watched)
        self.log(f'  > {step.id} ({step.lane}, why: {o.why})')

    def _finish(self, sid: str) -> None:
        proc, tmp, _ = self.running[sid]
        step, o = self.by_id[sid], self.outcomes[sid]
        o.rc = proc.returncode
        o.seconds = round(time.time() - o.started, 1)
        if o.status == 'running':
            if o.seconds >= step.timeout:
                o.status = 'timeout'
            elif not step.reads_status:
                o.status = 'ran'   # it reports against the original; its lines are read, rc kept
            else:
                o.status = 'passed' if o.rc == 0 else 'failed'
        text = (self.out / f'{sid}.log').read_text(errors='replace').splitlines()
        o.summary = [l for l in text if l.startswith('  ') or 'status=' in l][-60:] or text[-8:]
        shutil.rmtree(tmp, ignore_errors=True)
        with self.lock:
            del self.running[sid]
        self.log(f'  < {sid}: {o.status} in {o.seconds:.0f} s, peak temp {fmt(o.peak_temp)}')
        if sid in self.callbacks:
            self.callbacks[sid](self, o)
        if step.twin and self.outcomes[step.twin].status not in ('pending', 'running'):
            self._judge(*(sorted((sid, step.twin), key=lambda x: '@' in x)))

    def _judge(self, sid: str, twin: str) -> None:
        """A step that reports against the original passes when its lines equal its twin's on
        main: the change did not move what it measures. Otherwise it is `changed`, which fails the
        run unless the task expects it (`--expect-changed`) after reading both."""
        o, t = self.outcomes[sid], self.outcomes[twin]
        if o.status != 'ran' or t.status != 'ran':
            return
        mine, main = (normal_lines(self.out / f'{x}.log', self.env) for x in (sid, twin))
        if mine == main:
            o.status = 'same-as-main'
        else:
            o.status = 'changed (expected)' if sid in self.context.get('expect_changed', ()) else 'changed'
            o.summary = ['  differs from main:'] + [f'  - {l}' for l in main if l not in mine][:20] + \
                        [f'  + {l}' for l in mine if l not in main][:20]
        t.status = 'reference'
        self.log(f'  = {sid}: {o.status} against its run on main')

    def go(self) -> dict:
        stop = threading.Event()
        monitor = threading.Thread(target=self._monitor, args=(stop,), daemon=True)
        monitor.start()
        started = time.time()
        pending = list(self.steps)
        for s in pending:
            self.outcomes[s.id] = Outcome(s.id, False, '')
        try:
            while pending or self.running:
                for sid, (proc, _, _) in list(self.running.items()):
                    step = self.by_id[sid]
                    if proc.poll() is None and time.time() - self.outcomes[sid].started > step.timeout:
                        self.outcomes[sid].status = 'timeout'
                        _kill(proc)
                    if proc.poll() is not None:
                        self._finish(sid)
                lanes = {self.by_id[s].lane for s in self.running}
                used = sum(self.by_id[s].weight for s in self.running)
                caps = sum(self.by_id[s].temp_cap for s in self.running)
                for step in list(pending):
                    if step.needs_identity and 'identity' in self.context and self.context['identity'] is None:
                        break   # selection waits for the builds, in order
                    if not self._deps_done(step):
                        if step.lane != 'pool':
                            break
                        continue
                    o = self.outcomes[step.id]
                    if not o.why:
                        o.selected, o.why = step.select(self)
                    if not o.selected:
                        o.status = 'skipped'
                        pending.remove(step)
                        self.log(f'  - {step.id}: skipped ({o.why})')
                        continue
                    if step.lane == 'exclusive':
                        if not self.running:
                            self._start(step); pending.remove(step)
                        break
                    if step.lane == 'realtime':
                        if lanes <= {'realtime'}:
                            self._start(step); pending.remove(step); lanes = {'realtime'}
                            continue
                        break
                    if lanes & {'exclusive', 'realtime'}:
                        break
                    # A wide step starts once half the cores are free rather than waiting for an
                    # empty pool, which a stream of small steps could postpone to the end.
                    if used + step.weight > self.cores and used > self.cores // 2:
                        continue
                    busy = {w for sid in self.running for w in self.by_id[sid].writes}
                    if busy & set(step.writes):
                        continue
                    if caps + step.temp_cap > self.cap_total and self.running:
                        continue
                    free = free_bytes(self.out)
                    if free < self.disk_floor + step.temp_cap:
                        if self.running:
                            continue
                        o.status, o.why = 'failed', f'{o.why}; not started: {fmt(free)} free, under the floor'
                        pending.remove(step)
                        self.log(f'  ! {step.id}: {o.why}')
                        continue
                    self._start(step); pending.remove(step)
                    used += step.weight; caps += step.temp_cap; lanes.add('pool')
                if pending and not self.running:
                    # Nothing runs and nothing could start: fail what is left rather than wait.
                    for step in pending:
                        o = self.outcomes[step.id]
                        o.status = 'failed'
                        o.why = f'{o.why or "never started"}; nothing could start it (waiting on {", ".join(step.after) or "none"})'
                        self.log(f'  ! {step.id}: {o.why}')
                    pending.clear()
                time.sleep(0.5)
        finally:
            for sid, (proc, _, _) in list(self.running.items()):
                _kill(proc)
            stop.set(); monitor.join()
        return dict(seconds=round(time.time() - started, 1), peak_temp=self.peak_total,
                    disk_used_peak=self.start_free - self.min_free, start_free=self.start_free,
                    min_free=self.min_free, steps=[vars(self.outcomes[s.id]) for s in self.steps])


def normal_lines(log: Path, env: dict) -> list[str]:
    """A step's reported lines, without what differs only by where it ran."""
    import re
    try:
        text = log.read_text(errors='replace')
    except OSError:
        return []
    if env.get('RUN'):
        text = text.replace(env['RUN'] + '/main', '<run>').replace(env['RUN'], '<run>')
    for key in ('W', 'BASE_TREE'):            # the candidate's checkout and main's are one tree
        if env.get(key):
            text = text.replace(env[key], '<tree>')
    text = re.sub(r'/(private/)?var/folders/\S*|/tmp/\S*', '<tmp>', text)
    text = re.sub(r'"seconds": [0-9.]+', '"seconds": -', text)
    return [l.rstrip() for l in text.splitlines() if l.startswith('  ')]


def group_cpu_seconds() -> dict[int, float]:
    """CPU seconds used so far by each process group's live members (`ps`, macOS and Linux)."""
    try:
        text = subprocess.run(['ps', '-A', '-o', 'pgid=,time='], capture_output=True, text=True, timeout=30).stdout
    except (OSError, subprocess.SubprocessError):
        return {}
    out: dict[int, float] = {}
    for line in text.splitlines():
        parts = line.split()
        if len(parts) != 2:
            continue
        days, _, clock = parts[1].rpartition('-')     # [days-][hours:]minutes:seconds
        seconds = 0.0
        for field in clock.split(':'):
            seconds = seconds * 60 + float(field)
        seconds += 86400 * int(days or 0)
        out[int(parts[0])] = out.get(int(parts[0]), 0.0) + seconds
    return out


def _size(path: Path) -> int:
    try:
        return path.stat().st_size
    except OSError:
        return 0


def entries(directory: Path) -> set[str]:
    try:
        return set(os.listdir(directory))
    except OSError:
        return set()


def new_entries_bytes(directory: Path, before: set[str]) -> int:
    return sum(tree_bytes(directory / name) if (directory / name).is_dir() else
               (directory / name).stat().st_blocks * 512 for name in entries(directory) - before
               if (directory / name).exists())


def _kill(proc: subprocess.Popen) -> None:
    # A group whose members have all exited answers EPERM on macOS, not ESRCH.
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(proc.pid, sig)
        except (ProcessLookupError, PermissionError):
            return
        time.sleep(2)


def write_report(path: Path, report: dict) -> None:
    path.write_text(json.dumps(report, indent=1, default=str) + '\n')
