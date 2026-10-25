#!/usr/bin/env python3
"""NATIVE-READABILITY: native-against-native equivalence sweep.

Tracked by GATES-AGAINST-MAIN (D-0012) from `local/evidence/native-readability/equivalence.py`
unchanged but for `--jobs`, which runs the scenarios in worker processes (each with its own
temporary directory) and merges their results in scenario order: the 25-minute serial sweep is
independent per scenario.

Runs every race scenario (DRAGSTER, ZOOM ZOO and classic.track.00-44) under several
controller schedules through a base and a candidate `zoom_zoo_runner`, and compares
their output byte for byte. The runner prints the full serialized state after every
update, so equal output means equal behaviour and equal serialized bytes on every
update of every run. It also:

- renders frames from each run through each side's `classic_race_presentation_runner
  --timeline` and compares the pictures (`--pictures N` per run, 0 to skip);
- restarts each run from serialized states the base emitted (`--seed`), so each side's
  deserializer is exercised on states of every track (`--restarts N` per run);
- runs the legacy DRAGSTER `movement_runner` (pack v1) under the same schedules, with
  its own restarts.

Part 1 used the first version (six schedules, pictures on Right only); review S7 of
pull request #23 asked for the restarts, the X/L/R/Select schedules, pictures on every
schedule and `movement_runner`.

Usage (from a worktree with local/rom-location.txt, the v14 pack and the v1 pack):
  equivalence.py --base DIR --candidate DIR --pack PACK --v1-pack PACK --out JSON
DIR holds zoom_zoo_runner, classic_race_presentation_runner and movement_runner.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import random
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# Controller bits of both runners: B Y Select Start Up Down Left Right A X L R.
B, Y, SELECT, START, UP, DOWN, LEFT, RIGHT, A, X, L, R = (1 << i for i in range(12))
MOVES = [0, RIGHT, LEFT, RIGHT | A, LEFT | A, RIGHT | B, RIGHT | Y, A, B, Y, UP, DOWN, RIGHT | UP,
         LEFT | DOWN, RIGHT | A | Y, LEFT | B | Y, RIGHT | DOWN, A | B]
# The buttons-N schedules add X (tricks), the shoulders (rotation) and Select.
MORE_MOVES = MOVES + [RIGHT | X, X, L, R, RIGHT | L, RIGHT | R, LEFT | X | A, SELECT, RIGHT | L | R]
SCHEDULES = "released,right,right-jumps,right-b,random-1,random-2,random-3,buttons-4,buttons-5"


def scenarios() -> list[str]:
    return ["classic.crawler.dragster", "classic.crawler.zoom-zoo"] + [f"classic.track.{n:02d}" for n in range(45)]


def masks(kind: str, updates: int) -> list[int]:
    if kind == "released":
        return [0] * updates
    if kind == "right":
        return [RIGHT] * updates
    if kind == "right-jumps":
        return [RIGHT | (A if (k // 30) % 3 == 0 else 0) for k in range(updates)]
    if kind == "right-b":                       # the legacy DRAGSTER domain: B brakes and jumps
        return [RIGHT | (B if (k // 25) % 4 == 0 else 0) for k in range(updates)]
    name, seed = kind.rsplit("-", 1)            # random-N, buttons-N: a move held 4-60 updates
    rng = random.Random(int(seed) * (1 if name == "random" else 7919))
    moves = MOVES if name == "random" else MORE_MOVES
    out: list[int] = []
    while len(out) < updates:
        move = START if rng.random() < 0.01 else rng.choice(moves)
        out += [move] * (1 if move == START else rng.randint(4, 60))
    return out[:updates]


def rows(first_frame: int, values: list[int]) -> str:
    return "".join(f"{first_frame + k + 1} {m} 0\n" for k, m in enumerate(values))


def run(binary: Path, args: list[str], timeout: int = 900) -> tuple[int, bytes, str]:
    p = subprocess.run([str(binary), *args], capture_output=True, timeout=timeout)
    return p.returncode, p.stdout, p.stderr.decode(errors="replace").strip()


def first_difference(b: bytes, c: bytes) -> int:
    bl, cl = b.splitlines(), c.splitlines()
    return next((i for i, (x, y) in enumerate(zip(bl, cl)) if x != y), min(len(bl), len(cl)))


class Sweep:
    def __init__(self, a: argparse.Namespace, tmp: Path) -> None:
        self.a, self.tmp = a, tmp
        self.results: list[dict] = []
        self.differences = 0

    def pair(self, name: str, args: list[str]) -> tuple[bool, tuple, tuple]:
        b = run(self.a.base / name, args)
        # FRONT-END-MAIN-MENU: a candidate on a newer pack profile reads its own pack (the race
        # entries are the same bytes); the base keeps the pack its profile accepts.
        if self.a.candidate_pack is not None:
            args = [str(self.a.candidate_pack) if x == str(self.a.pack) else x for x in args]
        c = run(self.a.candidate / name, args)
        return b == c, b, c

    def record(self, row: dict) -> None:
        self.differences += not row["same"]
        self.results.append(row)
        extra = f" restarts={row.get('restarts_same', '-')} pictures={row.get('pictures_same', '-')}"
        print(f"{row['runner']} {row['scenario']} {row['schedule']}: {'same' if row['same'] else 'DIFFERENT'} "
              f"rows={row.get('rows')} rc={row.get('returncode')}{extra}", flush=True)

    def restarts(self, runner: str, output: bytes, state_column: int, values: list[int], fixed: list[str]) -> tuple[int, int]:
        """Restart both sides from states the base emitted; (runs, identical runs)."""
        lines = output.splitlines()
        if len(lines) < 4 or self.a.restarts == 0:
            return 0, 0
        offset = 1 if runner == "movement_runner" else 0      # its first line is a header
        body = lines[offset:]
        picks = sorted({max(1, len(body) * i // (self.a.restarts + 1)) for i in range(1, self.a.restarts + 1)})
        done = same = 0
        for k in picks:
            fields = body[k].split()
            frame, state = int(fields[0]), bytes.fromhex(fields[state_column].decode())
            seed = self.tmp / "seed.bin"
            seed.write_bytes(state)
            rest = self.tmp / "rest.txt"
            rest.write_text(rows(frame, values[k:k + self.a.restart_updates]))
            ok, _, _ = self.pair(runner, ["--seed", str(seed), *fixed, "--inputs", str(rest)])
            done += 1
            same += ok
        return done, same

    def pictures(self, output: bytes) -> list[dict]:
        timeline = self.tmp / "timeline.txt"
        timeline.write_bytes(output)
        count = output.count(b"\n") - 1
        first = int(output.split(maxsplit=1)[0])
        n = self.a.pictures
        frames = sorted({first + max(1, count * i // n) for i in range(1, n + 1)}) if n and count > 1 else []
        out = []
        for f in frames:
            pb, pc = self.tmp / "b.ppm", self.tmp / "c.ppm"
            rb = run(self.a.base / "classic_race_presentation_runner", [str(self.a.pack), "--timeline", str(timeline), str(f), str(pb)])
            rc = run(self.a.candidate / "classic_race_presentation_runner", [str(self.a.candidate_pack or self.a.pack), "--timeline", str(timeline), str(f), str(pc)])
            db = pb.read_bytes() if rb[0] == 0 and pb.exists() else rb[2].encode()
            dc = pc.read_bytes() if rc[0] == 0 and pc.exists() else rc[2].encode()
            out.append({"frame": f, "same": db == dc and rb[0] == rc[0], "rendered": rb[0] == 0,
                        "sha256": hashlib.sha256(db).hexdigest()[:16]})
            for p in (pb, pc):
                p.unlink(missing_ok=True)
        return out

    def race(self, scenario: str) -> None:
        fixed = ["--content-pack", str(self.a.pack)]
        start_args = ["--start", scenario, *fixed]
        empty = self.tmp / "none.txt"
        empty.write_text("")
        ok, b, c = self.pair("zoom_zoo_runner", start_args + ["--inputs", str(empty)])
        if b[0] != 0 or c[0] != 0:
            self.results.append({"runner": "zoom_zoo_runner", "scenario": scenario, "schedule": None, "same": ok,
                                 "base_error": b[2], "candidate_error": c[2]})
            self.differences += not ok
            return
        start = int(b[1].split()[0])
        for kind in self.a.schedules.split(","):
            values = masks(kind, self.a.updates)
            inputs = self.tmp / "inputs.txt"
            inputs.write_text(rows(start, values))
            ok, b, c = self.pair("zoom_zoo_runner", start_args + ["--inputs", str(inputs)])
            row = {"runner": "zoom_zoo_runner", "scenario": scenario, "schedule": kind, "same": ok,
                   "rows": b[1].count(b"\n"), "returncode": b[0], "stderr": b[2][:120],
                   "sha256": hashlib.sha256(b[1]).hexdigest()[:16]}
            if not ok:
                row.update(first_differing_row=first_difference(b[1], c[1]), candidate_returncode=c[0], candidate_stderr=c[2][:120])
            elif b[0] == 0 or row["rows"] > 2:
                done, same = self.restarts("zoom_zoo_runner", b[1], 1, values, fixed)
                row.update(restarts=done, restarts_same=same)
                pics = self.pictures(b[1]) if b[1].count(b"\n") > 2 else []
                row.update(pictures=pics, pictures_same=sum(p["same"] for p in pics))
                row["same"] = done == same and all(p["same"] for p in pics)
            self.record(row)

    def legacy(self) -> None:
        fixed = ["--content-pack", str(self.a.v1_pack)]
        for kind in self.a.schedules.split(","):
            values = masks(kind, self.a.updates)
            inputs = self.tmp / "inputs.txt"
            inputs.write_text(rows(0, values))
            # The legacy runner starts at frame 0 only through its start state; find its frame first.
            empty = self.tmp / "none.txt"
            empty.write_text("")
            probe = run(self.a.base / "movement_runner", ["--start-state", "classic.crawler.dragster.race-start.v1", *fixed, "--inputs", str(empty)])
            start = int(probe[1].splitlines()[1].split()[0]) if probe[0] == 0 else 0
            inputs.write_text(rows(start, values))
            ok, b, c = self.pair("movement_runner", ["--start-state", "classic.crawler.dragster.race-start.v1", *fixed, "--inputs", str(inputs)])
            row = {"runner": "movement_runner", "scenario": "classic.crawler.dragster.race-start.v1", "schedule": kind,
                   "same": ok, "rows": b[1].count(b"\n") - 1, "returncode": b[0], "stderr": b[2][:120],
                   "sha256": hashlib.sha256(b[1]).hexdigest()[:16]}
            if not ok:
                row.update(first_differing_row=first_difference(b[1], c[1]), candidate_returncode=c[0], candidate_stderr=c[2][:120])
            else:
                done, same = self.restarts("movement_runner", b[1], 14, values, fixed)
                row.update(restarts=done, restarts_same=same)
                row["same"] = done == same
            self.record(row)


def sweep_part(a: argparse.Namespace, scenario: str | None) -> tuple[list[dict], int]:
    """One scenario's runs, or the legacy runner's when `scenario` is None."""
    with tempfile.TemporaryDirectory(prefix="equivalence-") as tmp:
        sweep = Sweep(a, Path(tmp))
        if scenario is None:
            sweep.legacy()
        else:
            sweep.race(scenario)
        return sweep.results, sweep.differences


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", required=True, type=Path)
    ap.add_argument("--candidate", required=True, type=Path)
    ap.add_argument("--pack", required=True, type=Path, help="tracks pack (v14)")
    ap.add_argument("--candidate-pack", type=Path, default=None, help="the candidate's pack, when its profile differs")
    ap.add_argument("--v1-pack", required=True, type=Path, help="classic-crawler-dragster.pack for movement_runner")
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--updates", type=int, default=6000)
    ap.add_argument("--schedules", default=SCHEDULES)
    ap.add_argument("--pictures", type=int, default=6, help="frames rendered per run")
    ap.add_argument("--restarts", type=int, default=3, help="seeded restarts per run")
    ap.add_argument("--restart-updates", type=int, default=600)
    ap.add_argument("--scenarios", help="comma-separated subset")
    ap.add_argument("--jobs", type=int, default=1, help="worker processes, one scenario each at a time")
    a = ap.parse_args()
    started = time.time()
    parts: list = (a.scenarios.split(",") if a.scenarios else scenarios()) + [None]
    results: list[dict] = []
    differences = 0
    from concurrent.futures import ProcessPoolExecutor
    with ProcessPoolExecutor(max(1, a.jobs)) as pool:
        for part_results, part_differences in pool.map(sweep_part, [a] * len(parts), parts):
            for row in part_results:
                if row["schedule"]:
                    extra = f" restarts={row.get('restarts_same', '-')} pictures={row.get('pictures_same', '-')}"
                    print(f"{row['runner']} {row['scenario']} {row['schedule']}: {'same' if row['same'] else 'DIFFERENT'} "
                          f"rows={row.get('rows')} rc={row.get('returncode')}{extra}", flush=True)
            results += part_results
            differences += part_differences
    runs = [r for r in results if r["schedule"]]
    summary = {"runs": len(runs), "differences": differences,
               "updates_compared": sum(r["rows"] for r in runs),
               "restarts_compared": sum(r.get("restarts", 0) for r in runs),
               "pictures_compared": sum(len(r.get("pictures", [])) for r in runs),
               "pictures_rendered": sum(p["rendered"] for r in runs for p in r.get("pictures", [])),
               "scenarios_without_start": [r["scenario"] for r in results if not r["schedule"]],
               "seconds": round(time.time() - started, 1)}
    a.out.write_text(json.dumps({"summary": summary, "results": results}, indent=1) + "\n")
    print(json.dumps(summary))
    print("status=" + ("passed" if differences == 0 else "failed"))
    return 0 if differences == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
