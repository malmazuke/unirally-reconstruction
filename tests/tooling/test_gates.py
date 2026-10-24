"""The gate runner (D-0012): its scheduler's limits, the corpus records, and step selection.

These run without a build or any evidence: the scheduler on small shell steps, the corpus on
invented invocations, and selection on a hand-made identity.
"""
from __future__ import annotations

import json
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

from tools.unirally_lab.gates import catalogue, corpus, replay
from tools.unirally_lab.gates.disk import GB
from tools.unirally_lab.gates.identity import Identity
from tools.unirally_lab.gates.schedule import Run, Step


def run_steps(steps, tmp: Path, cores=4, floor=0, cap_total=64 * GB, context=None):
    out = tmp / 'out'; out.mkdir(exist_ok=True)
    temp = tmp / 'temp'; temp.mkdir(exist_ok=True)
    run = Run(steps, dict(PATH='/usr/bin:/bin'), tmp, out, temp, cores, floor, cap_total, context or {},
              log=lambda *_: None)
    return run.go(), out


class Scheduler(unittest.TestCase):
    def test_exclusive_step_runs_alone_and_pool_steps_overlap(self):
        with tempfile.TemporaryDirectory() as d:
            tmp = Path(d)
            stamp = 'python3 -c "import time;print(time.time())"'
            steps = [Step('x', 'x', f'{stamp} > "$G/start"; sleep 1; {stamp} > "$G/end"', lane='exclusive')]
            steps += [Step(f'p{i}', 'p', f'{stamp} > "$G/start"; sleep 1; {stamp} > "$G/end"') for i in range(3)]
            report, out = run_steps(steps, tmp)
            t = {s: (float((out / s / 'start').read_text()), float((out / s / 'end').read_text())) for s in ('x', 'p0', 'p1', 'p2')}
            for p in ('p0', 'p1', 'p2'):
                self.assertGreaterEqual(t[p][0], t['x'][1])          # nothing beside the exclusive step
            self.assertLess(max(t[p][0] for p in ('p0', 'p1', 'p2')), min(t[p][1] for p in ('p0', 'p1', 'p2')))  # overlap
            self.assertTrue(all(s['status'] == 'passed' for s in report['steps']))

    def test_a_step_past_its_temp_cap_is_stopped(self):
        with tempfile.TemporaryDirectory() as d:
            step = Step('big', 'big', 'for i in 1 2 3 4 5 6 7 8; do head -c 2000000 /dev/zero > "$TMPDIR/f$i"; sleep 1; done',
                        temp_cap=4_000_000, timeout=60)
            report, _ = run_steps([step], Path(d))
            self.assertEqual(report['steps'][0]['status'], 'over-cap')
            self.assertGreater(report['steps'][0]['peak_temp'], 4_000_000)

    def test_no_step_starts_under_the_free_disk_floor(self):
        with tempfile.TemporaryDirectory() as d:
            report, _ = run_steps([Step('s', 's', 'true')], Path(d), floor=1 << 60)
            self.assertEqual(report['steps'][0]['status'], 'failed')
            self.assertIn('under the floor', report['steps'][0]['why'])

    def test_step_temp_is_private_and_removed(self):
        with tempfile.TemporaryDirectory() as d:
            report, out = run_steps([Step('a', 'a', 'echo "$TMPDIR" > "$G/t"; touch "$TMPDIR/x"')], Path(d))
            used = Path((out / 'a' / 't').read_text().strip())
            self.assertEqual(used.name, 'a')
            self.assertFalse(used.exists())

    def test_unselected_and_dependent_steps(self):
        with tempfile.TemporaryDirectory() as d:
            steps = [Step('first', 'f', 'sleep 0.5; echo done > "$G/x"'),
                     Step('later', 'l', 'test -f "$RUN/first/x"', after=('first',)),
                     Step('never', 'n', 'false', select=lambda run: (False, 'not needed'))]
            out = Path(d) / 'out'
            report, _ = run_steps(steps, Path(d))
            status = {s['id']: s['status'] for s in report['steps']}
            self.assertEqual(status, {'first': 'passed', 'later': 'failed', 'never': 'skipped'})


class Corpus(unittest.TestCase):
    def test_inputs_are_stored_by_content_and_outputs_dropped(self):
        with tempfile.TemporaryDirectory() as d:
            tmp = Path(d)
            (tmp / 'pack').write_bytes(b'pack')
            (tmp / 'in.txt').write_text('5 0080 0000\n')
            argv = ['--content-pack', 'pack', '--frames', '9', '--inputs', 'in.txt', '--picture', '3', 'p.ppm',
                    '--record-write', '1', '10d3', '3', '--race-initialization', '7', '--sound-cues', 'c.txt']
            log = tmp / 'c' / 'step.jsonl'
            log.parent.mkdir()
            corpus.record(log, tmp, argv)
            entry = json.loads(log.read_text())
            self.assertEqual(entry['outputs'], ['--picture', '--sound-cues'])
            self.assertEqual(entry['pictures'], [[3, 3]])
            stored = entry['inputs'][entry['inputs'].index('--inputs') + 1]
            self.assertEqual((tmp / 'c' / 'store' / stored[1:]).read_text(), '5 0080 0000\n')
            self.assertIn('--record-write', entry['inputs'])
            # The same inputs with other outputs are one case.
            corpus.record(log, tmp, ['--content-pack', 'pack', '--frames', '9', '--inputs', 'in.txt', '--record-write', '1',
                                     '10d3', '3', '--race-initialization', '7', '--race-timeline', 't'])
            corpus.record(log, tmp, ['--content-pack', 'pack', '--frames', '9', '--inputs', 'in.txt', '--record-write', '1',
                                     '10d3', '3', '--race-initialization', '7', '--picture', '4', 'q.ppm', '--picture', '6', 'r.ppm'])
            cases, by_step = corpus.load(tmp / 'c')
            self.assertEqual(len(cases), 1)
            self.assertEqual(next(iter(cases.values()))['picture_frames'], {3, 4, 6})
            self.assertEqual(by_step['step'], {entry['key']})

    def test_an_unknown_option_is_refused(self):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / 'pack').write_bytes(b'p')
            with self.assertRaises(ValueError):
                corpus.normalize(['--content-pack', 'pack', '--frames', '1', '--new-thing', 'x'], Path(d), None)

    def test_shim_records_then_runs_the_real_binary(self):
        with tempfile.TemporaryDirectory() as d:
            tmp = Path(d)
            binary = tmp / 'front_end_runner'
            binary.write_text('#!/bin/bash\necho real "$@"\n'); binary.chmod(0o755)
            (tmp / 'pack').write_bytes(b'p')
            corpus.install_shim(binary)
            self.assertTrue(corpus.is_shim(binary))
            self.assertEqual(corpus.real_binary(binary).name, 'front_end_runner.real')
            log = tmp / 'corpus' / 's.jsonl'; log.parent.mkdir()
            out = subprocess.run([str(binary), '--content-pack', 'pack', '--frames', '2'], cwd=tmp, capture_output=True,
                                 text=True, env={'GATES_RECORD': str(log), 'PATH': '/usr/bin:/bin'})
            self.assertEqual(out.stdout.strip(), 'real --content-pack pack --frames 2')
            self.assertEqual(len(log.read_text().splitlines()), 1)
            # A rebuilt binary in place of the shim becomes the real one.
            binary.write_text('#!/bin/bash\necho rebuilt\n'); binary.chmod(0o755)
            corpus.install_shim(binary)
            self.assertIn('rebuilt', corpus.real_binary(binary).read_text())
            corpus.remove_shim(binary)
            self.assertFalse(corpus.is_shim(binary))


class Replay(unittest.TestCase):
    def test_first_difference(self):
        self.assertIsNone(replay.first_difference([b'a', b'b'], [b'a', b'b']))
        self.assertEqual(replay.first_difference([b'a', b'b'], [b'a', b'c']), 1)
        self.assertEqual(replay.first_difference([b'a'], [b'a', b'c']), 1)

    def test_a_fake_runner_pair_is_compared_on_every_stream(self):
        with tempfile.TemporaryDirectory() as d:
            tmp = Path(d)
            (tmp / 'pack').write_bytes(b'p')
            script = '''#!/usr/bin/env python3
import sys
a = sys.argv; opts = {a[i]: a[i + 1] for i in range(1, len(a) - 1) if a[i].startswith('--')}
if '--rider-word' in opts and '%s' == 'old': sys.exit(1)
assert '--picture-frames' in opts
for k in ('--race-timeline', '--one-player-timeline', '--look-timeline', '--sound-cues', '--frame-digests'):
    if k in opts:
        with open(opts[k], 'w') as f:
            for n in range(int(opts['--frames'])): f.write(f'{n} {k} {%s if k == '--frame-digests' and n == 3 else 0}\\n')
for n in range(int(opts['--frames'])): print(n, 'state')
'''
            for name, bug in (('main', '0'), ('cand', '1')):
                path = tmp / name
                path.write_text(script % ('old', bug)); path.chmod(0o755)
            case = dict(key='k', pack=dict(path=str(tmp / 'pack'), sha256='x'), inputs=['--frames', '6'], steps={'s'}, outputs=set())
            result = replay.replay({'k': case}, tmp, replay.Side(tmp / 'main', tmp / 'pack'), replay.Side(tmp / 'cand', tmp / 'pack'),
                                   'x', jobs=2)
            row = result['rows'][0]
            self.assertFalse(row['equal'])
            self.assertEqual(row['differing_streams'], ['frame-digests'])
            self.assertEqual(result['summary']['steps_with_differences'], ['s'])


class Selection(unittest.TestCase):
    def ident(self, changed=(), files=()):
        ident = Identity(base='b', changed_files=list(files), pack_changed=False)
        for name in ('front_end_runner', 'zoom_zoo_runner', 'app', 'audio_driver_runner'):
            ident.binaries[name] = dict(changed=name in changed, reason='test')
        return ident

    class FakeRun:
        def __init__(self, context):
            self.context = context

    def test_corpus_check_runs_only_where_its_own_runs_differ(self):
        rows = [dict(steps=['two-pad'], equal=True), dict(steps=['wipe-ram', 'two-pad'], equal=False)]
        ctx = dict(full=False, identity=self.ident({'front_end_runner'}, ['src/core/front_end.cpp']),
                   corpus_steps={'two-pad': {'a', 'b'}, 'look': {'c'}, 'wipe-ram': {'b'}}, replay=dict(rows=rows))
        run = self.FakeRun(ctx)
        self.assertEqual(catalogue.corpus('two-pad')(run)[0], True)
        self.assertEqual(catalogue.corpus('wipe-ram')(run)[0], True)
        ctx['replay'] = dict(rows=[dict(steps=['look'], equal=True)])
        self.assertEqual(catalogue.corpus('look')(run)[0], False)
        self.assertEqual(catalogue.corpus('unrecorded')(run)[0], True)        # no corpus: run it

    def test_unchanged_binary_skips_and_full_runs_everything(self):
        run = self.FakeRun(dict(full=False, identity=self.ident(set(), ['src/app/main.cpp']), corpus_steps={}, replay=None))
        self.assertEqual(catalogue.drives('zoom_zoo_runner')(run)[0], False)
        self.assertEqual(catalogue.corpus('two-pad')(run)[0], False)
        run.context['full'] = True
        self.assertEqual(catalogue.drives('zoom_zoo_runner')(run)[0], True)
        self.assertEqual(catalogue.corpus('two-pad')(run)[0], True)

    def test_a_trusted_manifest_change_selects_the_frozen_gates(self):
        run = self.FakeRun(dict(full=False, identity=self.ident(set(), ['tests/manifests/native/zoom-zoo-race-guards.reference.json']),
                                corpus_steps={}, replay=None))
        self.assertEqual(catalogue.drives('zoom_zoo_runner', tracked=catalogue.FROZEN_TRUSTS)(run)[0], True)

    def test_every_frozen_gate_and_section_is_in_the_catalogue(self):
        ids = [s.id for s in catalogue.steps(8, [])]
        self.assertEqual(len([i for i in ids if i.startswith('gate-')]), 11)
        self.assertEqual(len(ids), len(set(ids)))
        for required in ('build', 'synthetic', 'fe-replay', 'race-sweep', 'tooling', 'rules', 'mi-w5', 'lt-backs', 'pa-cues', 'pa-pcm'):
            self.assertIn(required, ids)


if __name__ == '__main__':
    unittest.main()


class FrozenOriginal(unittest.TestCase):
    """`--frozen-original` trusts only a report whose digest the contract froze."""

    def test_the_report_must_match_the_contract(self):
        from tools.unirally_lab.native import dragster_playable, zoom_zoo_playable
        from tools.unirally_lab.native.zoom_zoo_trial_reference import digest
        with tempfile.TemporaryDirectory() as d:
            ref = Path(d)
            document = dict(frames=[1207, 1300], timeline=[[[]]], case=dict(id='x'))
            (ref / 'reference.json').write_text(json.dumps(document))
            frozen = dict(original_sha256=digest(document), rows_sha256='r', events=dict(loading_frame=1))
            for module in (zoom_zoo_playable, dragster_playable):
                got, rows, events = module.frozen_original(ref, frozen)
                self.assertEqual(got, document)
                self.assertIsNone(rows)                     # the caller compares the rows by digest
                self.assertEqual(events, frozen['events'])
                with self.assertRaises(ValueError):
                    module.frozen_original(ref, dict(frozen, original_sha256='0' * 64))
