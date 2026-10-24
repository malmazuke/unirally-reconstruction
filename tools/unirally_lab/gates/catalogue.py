"""The gate steps (D-0012): every section of the last serial `gates.sh` (RECORD-SCREENS', from
PODIUM-AUDIO's), each with the binaries it drives and when it runs.

Commands are the sections' own lines, run from the candidate's checkout with these variables:
  W         the candidate's checkout          L  main's local/evidence
  PACK      the candidate's pack              BASE_PACK  main's pack
  B         a directory holding main's binaries (front_end_runner, zoom_zoo_runner,
            classic_race_presentation_runner, movement_runner)
  G         the step's own output directory   TMPDIR  the step's own temp directory
  CORPUS    the front-end corpus              JOBS  workers a sweep may use

When a step runs:
- `code`: any change outside docs/ and tasks/ (builds, the synthetic suite, tooling tests, rules);
- `drives`: one of the binaries it drives changed (ninja's recorded inputs, the pack, or the build);
- `corpus`: a check against the original that drives `front_end_runner`. It runs when its own
  recorded invocations, replayed through main and the candidate, differ on any output; when it has
  no recorded invocations; or in a full run. Equal on every output on its own inputs means its
  comparison with the original would read exactly what it read on main, where it was accepted.
"""
from __future__ import annotations

from .disk import GB
from .schedule import Step

PRELUDE = '''set -u
A="$L/audio-first-race"; O="$L/split-pause-menu"; C="$L/classic-pause-menu"; R="$L/split-race-end"
BIN="build/app-debug/src/core/zoom_zoo_runner"
'''

# The eleven frozen differential gates: (name, module, capture a/b stem, contract).
_M416 = '$L/m4-16-playable-zoom-zoo/m4-16'
_IDLE = '$L/m4-16-rider-art/m4-16-idle/captures'
_DRG = '$L/dragster-ordinary-controls/dragster-ordinary-controls/originals'
_OPP = '$L/zoom-zoo-opposing-input/zoom-zoo-opposing-input/originals'
FROZEN_GATES = [
    ('m4-16-primary', 'zoom_zoo_playable', f'{_M416}/boundary', 'zoom-zoo-playable-primary-v11.freeze.json'),
    ('m4-16-idle', 'zoom_zoo_playable', f'{_IDLE}/late-start', 'zoom-zoo-playable-idle-late-start-v11.freeze.json'),
] + [(f'opposing-{c}', 'zoom_zoo_playable', f'{_OPP}/{c}', f'zoom-zoo-playable-opposing-{c}-v11.freeze.json')
     for c in ('ride', 'axes', 'edges')] + [
    (f'dragster-{c}', 'dragster_playable', f'{_DRG}/{c}', f'dragster-ordinary-{c}.freeze.json')
    for c in ('primary', 'random-1', 'reversal', 'random-3', 'regression-landing-held-roll',
              'regression-countdown-actions-tie')]
# Tracked files the frozen-digest mode trusts unchanged (the projection and its guards).
FROZEN_TRUSTS = ('tools/unirally_lab/native/zoom_zoo_playable.py', 'tools/unirally_lab/native/dragster_playable.py',
                 'tools/unirally_lab/native/dragster_playable_reference.py', 'tools/unirally_lab/native/zoom_zoo_race_reference.py',
                 'tools/unirally_lab/native/zoom_zoo_trial_reference.py', 'tools/unirally_lab/native/zoom_zoo_race.py',
                 'tests/manifests/native/zoom-zoo-race-guards.reference.json',
                 'tests/manifests/native/dragster-race-guards.reference.json', 'tests/manifests/native/')


def code(run):
    ident = run.context['identity']
    return (True, 'a source change') if run.context['full'] or ident.any_code() else (False, 'only records changed')


def drives(*names, tracked=()):
    def select(run):
        if run.context['full']:
            return True, 'full run'
        ident = run.context['identity']
        hit = [n for n in names if ident.changed(n)]
        if hit:
            return True, f'{hit[0]} changed: {ident.binaries[hit[0]]["reason"]}'
        files = [f for f in ident.changed_files if f.startswith(tracked)] if tracked else []
        if files:
            return True, f'{files[0]} changed'
        return False, f'{", ".join(names)} unchanged from main, so it replays as main did'
    return select


def corpus(step_id, also=()):
    def select(run):
        ctx = run.context
        if ctx['full']:
            return True, 'full run'
        ident = ctx['identity']
        hit = [n for n in also if ident.changed(n)]
        if hit:
            return True, f'{hit[0]} changed'
        if not ident.changed('front_end_runner'):
            return False, "front_end_runner unchanged from main, so it replays as main did"
        keys = ctx['corpus_steps'].get(step_id)
        if not keys:
            return True, 'no recorded invocations: it runs against the original'
        replay = ctx.get('replay')
        if replay is None:
            return True, 'the replay did not finish: it runs against the original'
        rows = [r for r in replay['rows'] if step_id in r['steps']]
        differing = [r for r in rows if not r['equal']]
        if differing:
            return True, f'{len(differing)} of its {len(rows)} recorded runs differ from main'
        return False, f'all {len(rows)} of its recorded runs equal main on every output and frame'
    return select


def steps(jobs: int, task_checks: list[tuple[str, str, str]]) -> list[Step]:
    s: list[Step] = []
    add = s.append

    # -- builds and the synthetic suite: exclusive -----------------------------------------------
    add(Step('build-base', "main's binaries", 'python3 -m tools.unirally_lab.gates.base build "$BASE_SHA" "$B"',
             system='build', lane='exclusive', timeout=3600, needs_identity=False, select=lambda run: (True, 'every run')))
    add(Step('build', 'presets: build + ctest (ASan presets unavailable on this host; Linux CI covers them)', '''
for preset in lab-debug lab-release app-debug app-release; do
  python3 tools/project.py build --preset "$preset" > "$G/build-$preset.log" 2>&1
  st=$(grep status= "$G/build-$preset.log" | tail -1)
  ctest --test-dir "build/$preset" > "$G/ctest-$preset.log" 2>&1
  echo "  $preset build $st; ctest $(grep 'tests passed' "$G/ctest-$preset.log")"
  case "$st" in *passed*) ;; *) exit 1;; esac
  grep -q '100% tests passed' "$G/ctest-$preset.log" || exit 1
done''', system='build', lane='exclusive', timeout=5400, needs_identity=False, select=lambda run: (True, 'every run')))
    add(Step('synthetic', 'synthetic suite (app-debug), alone', '''
python3 tools/project.py test --suite synthetic --preset app-debug --artifacts "$G/artifacts" --report "$G/synthetic.json" > "$G/synthetic.log" 2>&1
sync; echo "  $(grep 'status=' "$G/synthetic.log" | tail -1)"
grep -q 'status=passed' "$G/synthetic.log"''', system='build', lane='exclusive', timeout=1800, select=code))

    # -- the app in real time: together, with nothing else -----------------------------------------
    for t in ('dragster', 'zoom-zoo', '13', '5', '40', '43', '25', '28'):
        add(Step(f'app-hidden-{t}', f'hidden app run, track {t}', f'''
python3 tools/project.py frontend run --track {t} --pack "$PACK" --preset app-debug --updates 4000 --hidden --fixed-controller-mask 128 --report "$G/hidden.json" > "$G/hidden.log" 2>&1
echo "  {t} $(grep status= "$G/hidden.log" | tail -1) $(grep -o 'rider-pose fallback frames: [0-9]*' "$G/hidden.log")"
grep -q 'status=passed' "$G/hidden.log"''', system='app', lane='realtime', timeout=900, select=drives('app')))
    add(Step('app-hidden-front-start', 'hidden app run, front end with Start held', '''
python3 tools/project.py frontend run --pack "$PACK" --preset app-debug --updates 2000 --hidden --fixed-controller-mask 8 --report "$G/hidden.json" > "$G/hidden.log" 2>&1
echo "  front end, Start held: $(grep status= "$G/hidden.log" | tail -1) $(grep -o 'Final native state: updates [0-9]*; frame [0-9]*' "$G/hidden.log")"
grep -q 'status=passed' "$G/hidden.log"''', system='app', lane='realtime', timeout=900, select=drives('app')))
    add(Step('app-demo-audio', "DEMO-AUDIO: the app's producer through three idle demos (native audio, real time)", PRELUDE + '''
DA="$L/demo-audio"; APP="build/app-debug/src/app/unirally.app/Contents/MacOS/unirally"
"$APP" --content-pack "$PACK" --hidden --native-title-menu-audio --updates 13000 --front-end-inputs "$DA/demo-long.frontend-inputs.txt" --audio-cue-log "$G/app-demo.cues" > "$G/app-demo.log" 2>&1
echo "  rc=$? $(grep -o 'native_audio_cued_frames=[0-9]* native_audio_cued_stopped=[0-9]*' "$G/app-demo.log") $(grep -o 'native_audio_dropped_late_pairs=[0-9]*' "$G/app-demo.log")"
python3 "$DA/resolve_app_cues.py" "$G/app-demo.cues" "$G/app-demo.resolved.txt"
frames=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['frames'])" "$DA/demo-long.script.json")
build/lab-release/src/core/front_end_runner --content-pack "$PACK" --frames $frames --inputs "$DA/demo-long.frontend-inputs.txt" $(cat "$DA/demo-long.runner-args.txt" 2>/dev/null) --sound-cues "$G/native-demo-long.cues.txt" > /dev/null 2>&1
awk '$1 <= 12999' "$G/native-demo-long.cues.txt" > "$G/runner-13000.cues.txt"
cmp -s "$G/app-demo.resolved.txt" "$G/runner-13000.cues.txt" && echo "  the app's cue log equals front_end_runner's through frame 12999" || { echo "  the app's cue log DIFFERS from front_end_runner's"; exit 1; }''',
             system='audio', lane='exclusive', timeout=1200, select=drives('app')))
    add(Step('app-podium-audio', "PODIUM-AUDIO: the app's producer through a podium (real time)", '''
zsh "$L/podium-audio/app_run.sh" "$G/app" 2>&1 | sed 's/^/  /' ''', shell='bash', system='audio', lane='exclusive',
             timeout=1200, reads_status=False, select=drives('app')))

    # -- the pool -------------------------------------------------------------------------------
    add(Step('v1-contracts', 'v1 presentation contracts (legacy DRAGSTER renderer, dragster.v1 pack)', PRELUDE + '''
V1="artifacts/gates-v1-fixtures"; rm -rf "$V1"; mkdir -p "$V1"; trap 'rm -rf "$V1"' EXIT
cp -R "$L/classic-presentation-unification/unification-baseline/winner-fixtures" "$V1/"
cp -R "$L/classic-presentation-unification/unification-baseline/loser-fixtures" "$V1/"
ok=0
for kind in winner loser; do
  manifest="tests/manifests/presentation/classic-crawler-dragster-v1.json"; [ "$kind" = loser ] && manifest="tests/manifests/presentation/classic-crawler-dragster-loser-v1.json"
  A2="artifacts/gates-$kind-visual"; rm -rf "$A2"
  python3 tools/project.py native presentation-check --manifest "$manifest" --fixtures "$V1/$kind-fixtures" --content-pack local/classic-crawler-dragster.pack --preset lab-debug --artifacts "$A2" --report "$A2/report.json" --timeout 600 > "$G/$kind.log" 2>&1
  echo "  $kind $(grep 'status=' "$G/$kind.log" | tail -1)"; grep -q status=passed "$G/$kind.log" || ok=1
  mv "$A2" "$G/$kind-visual"
done
exit $ok''', system='race', select=drives('presentation_runner')))
    add(Step('fuzz', 'fuzz (lab-release, 40 seeds; aborts on main too, recorded non-pass)', '''
build/lab-release/src/app/dragster_fuzz_runner --content-pack "$PACK" --first-seed 1 --seeds 40 --races 2 --max-updates 30000 --failure-cases "$G/fuzz-failures" > "$G/fuzz.log" 2>&1; echo "  fuzz rc=$? $(tail -1 "$G/fuzz.log")"''',
             system='race', reads_status=False, select=drives('dragster_fuzz_runner')))
    for name, module, stem, contract in FROZEN_GATES:
        add(Step(f'gate-{name}', f'differential gate {name} against the frozen original', PRELUDE + f'''
mode=--frozen-original
[ "${{FROZEN_FULL:-0}}" = 1 ] && mode=
python3 -m tools.unirally_lab.native.{module} compare --reference "{stem}-a" --repeat "{stem}-b" --contract tests/manifests/native/{contract} --binary "$BIN" --pack "$PACK" --out "$G/{name}.json" $mode > "$G/{name}.log" 2>&1
rc=$?
echo "  {name} rc=$rc ${{mode:-full}} $(python3 -c "import json;d=json.load(open('$G/{name}.json'));print(d['status'],'restores',len(d.get('restore_frames',[])),'rows',d.get('rows_sha256','?')[:16])" 2>/dev/null)"
[ $rc -eq 0 ] || tail -3 "$G/{name}.log"
exit $rc''', system='race', timeout=2400, select=drives('zoom_zoo_runner', tracked=FROZEN_TRUSTS)))
    add(Step('race-sweep', "race equivalence sweep: main against the candidate (every scenario, 9 schedules)", '''
python3 -m tools.unirally_lab.gates.race_sweep --base "$B" --candidate build/lab-release/src/core --pack "$BASE_PACK" --candidate-pack "$PACK" \\
  --v1-pack local/classic-crawler-dragster.pack --out "$G/equivalence.json" --jobs "$JOBS" > "$G/equivalence.log" 2>&1
rc=$?; echo "  rc=$rc $(tail -2 "$G/equivalence.log" | tr '\\n' ' ')"; exit $rc''',
             system='race', weight=max(1, jobs), timeout=3600,
             select=drives('zoom_zoo_runner', 'classic_race_presentation_runner', 'movement_runner')))
    add(Step('race-sweep-classify', 'race sweep: every differing picture classified by its frame\'s state', PRELUDE + '''
python3 "$C/classify_sweep.py" "$RUN/race-sweep/equivalence.json" "$B" "$BASE_PACK" | sed 's/^/  /' ''',
             system='race', after=('race-sweep',), reads_status=False,
             select=lambda run: (True, 'the race sweep found differences')
             if run.outcomes.get('race-sweep') and run.outcomes['race-sweep'].status == 'failed'
             else (False, 'the race sweep did not run or found none')))
    add(Step('track-sweeps', "every track's original sweeps (track-breadth-2, locked tours): candidate and main", PRELUDE + '''
st=0
for sw in "$L/track-breadth/track-breadth-2/sweep" "$L/locked-tours/sweep"; do
  n=$(basename "$(dirname "$sw")")
  for side in "candidate:build/lab-debug/src/core/zoom_zoo_runner" "main:$B/zoom_zoo_runner"; do
    s=${side%%:*}; bin=${side#*:}
    python3 -m tools.unirally_lab.native.track_reference recompare --per-track --sweep "$sw" --binary "$bin" --pack "$([ "$s" = main ] && echo "$BASE_PACK" || echo "$PACK")" --out "$G/recompare-$n-$s.json" > "$G/recompare-$n-$s.log" 2>&1; echo "  $n $s rc=$?"
  done
  eq=$(python3 -c "import json,sys;a,b=(json.load(open(p)) for p in sys.argv[1:]);print(a.get('rows')==b.get('rows') and a.get('per_track')==b.get('per_track'))" "$G/recompare-$n-candidate.json" "$G/recompare-$n-main.json" 2>&1 | tail -1)
  echo "  $n candidate rows equal main's: $eq"; [ "$eq" = True ] || st=1
done
exit $st''', system='race', weight=2, select=drives('zoom_zoo_runner')))
    add(Step('fe-replay', 'front-end corpus: every recorded check input through main and the candidate, every output on every frame', '''
python3 -m tools.unirally_lab.gates.replay --corpus "$CORPUS" --base "$B/front_end_runner" --base-pack "$BASE_PACK" \\
  --candidate "$FER" --candidate-pack "$PACK" --jobs "$JOBS" --out "$G/replay.json" > "$G/replay.log" 2>&1
rc=$?; tail -2 "$G/replay.log" | sed 's/^/  /'; exit $rc''', system='front-end', weight=max(1, jobs),
             timeout=3600, temp_cap=1 * GB, select=drives('front_end_runner')))
    add(Step('restores', 'restore checks (candidate only): two-pad races and the idle demo', PRELUDE + '''
st=0
for spec in "twop 2105" "twop 2210" "twop 2216" "twop 2246" "twop 2610" "twop2 2610" "twop2 2810" "twop2 2830" "vs2 2125"; do
  set -- ${spec}
  python3 -c "
import json,sys
B={'b':0x8000,'y':0x4000,'select':0x2000,'start':0x1000,'up':0x0800,'down':0x0400,'left':0x0200,'right':0x0100,'a':0x80,'x':0x40,'l':0x20,'r':0x10}
m=json.load(open('$O/$1.json'));pads={}
for c in m['inputs']['controllers']:
  for e in c['events']:
    for f in range(e['from'],e['to']+1):
      o,t=pads.get(f,(0,0));k=sum(B[b] for b in e['buttons']);pads[f]=(o|k,t) if c['port']==0 else (o,t|k)
print(''.join(f'{f} {x:04x} {y:04x}\\\\n' for f,(x,y) in sorted(pads.items())),end='')" > "$TMPDIR/$1.inputs"
  n=$(python3 -c "import json;print(json.load(open('$O/$1.json'))['run']['frames'])")
  "$FER" --content-pack "$PACK" --frames $n --inputs "$TMPDIR/$1.inputs" --race-initialization 1978 --restore-check $2 > /dev/null 2> "$G/restore-$1-$2.err"; rc=$?; echo "  $1 @$2 rc=$rc"; [ $rc -eq 0 ] || st=1
done
SHORT="--short-demo-title 13 --short-demo-title 16 --short-demo-title 18 --short-demo-title 21 --short-demo-title 33"
for x in 8478 11526 14572 17640 45068 63309 81561 96746 99793; do
  "$FER" --content-pack "$PACK" --frames 102000 --inputs "$L/split-screen-race/title-start-300-305.inputs" $SHORT --restore-check $x > /dev/null 2> "$G/restore-demo-$x.err"; rc=$?; echo "  demo restore @$x rc=$rc $(grep -h 'demo' "$G/restore-demo-$x.err" | tail -1)"; [ $rc -eq 0 ] || st=1
done
exit $st''', system='front-end', select=drives('front_end_runner')))

    # -- checks against the original, driven by front_end_runner: run when the corpus differs ------
    def original(step_id, title, command, writes=(), shell='bash', weight=1, temp_cap=1 * GB, also=(), timeout=5400):
        add(Step(step_id, title, PRELUDE + command if shell == 'bash' else command, system='front-end',
                 shell=shell, weight=weight, temp_cap=temp_cap, writes=writes, timeout=timeout,
                 after=('fe-replay',), reads_status=False, record=True, select=corpus(step_id, also)))

    original('fe-equivalence', "the front-end sweep's manifests through main and the candidate (every 500th picture)", '''
python3 "$C/fe_equivalence.py" "$B/front_end_runner" build/lab-release/src/core/front_end_runner "$BASE_PACK" "$PACK" "$G/fe-equivalence.json" 4 | tr '\\n' ' ' | sed 's/^/  /'; echo''', weight=4)
    original('pause-menu', 'the pause menu against the original (every captured picture)', '''
for c in menu countdown split "$C/review/fadein" "$C/review/zoomlap" "$L/race-pause-exits/decode/quit" "$L/race-pause-exits/decode/restart" "$L/race-pause-exits/decode/lapquit"; do
  echo "  $(python3 "$C/pictures.py" "$c" --binary build/lab-release/src/core/front_end_runner --pack "$PACK" 2>&1 | tail -1 | sed 's#/Users.*decode/##')"
done''', writes=('{L}/classic-pause-menu',))
    original('two-pad', 'two-pad races against the original: state words and picture classes', '''
for c in twop vs twop2 vs2 league league-restart twop-plain "$O/review/drfin" "$O/review/leaguefin" "$O/review/leaguecpu" "$O/review/zz2p" "$L/split-hud-gaps/review/zzap"; do
  echo "  $(python3 "$O/split_compare.py" "$c" --no-pictures --opponent-queue --idle-latch 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print(d['capture'],'race frames',d['race_frames'],'differences',d['word_differences'])")"
  echo "  $(python3 "$O/classify.py" "$c" 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print(d['capture'],d['counts'])")"
done''', writes=('{L}/split-pause-menu',), temp_cap=3 * GB)  # classify keeps two copies of every picture
    original('race-end', "the race's end against the original (R-0081): words, restores, picture classes", '''
S2="$TMPDIR/keep"
for spec in "vs-idle 4588" "zzap-long 8308" "league-idle 99999" "review/vs-opp 4588" "review/vs-late 4263" "review/vs-later 4588"; do
  set -- ${spec}
  echo "  $(python3 "$O/split_compare.py" "$R/$1" --no-pictures --opponent-queue --idle-latch 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print(d['capture'],'race frames',d['race_frames'],'differences',d['word_differences'],d['races'])")"
  [ -d "$R/$1/frames" ] || continue
  rm -rf "$S2"; mkdir -p "$S2" "$(dirname "$G/boxes-$1.txt")"; python3 "$R/boxes.py" "$R/$1" --keep "$S2" > "$G/boxes-$1.txt" 2>&1
  echo "  $1 $(python3 "$R/classes.py" "$R/$1" "$S2" $2 2>&1 | head -1)"
done
rm -rf "$S2"
for f in 3990 4347 4400; do
  echo "  vs-idle restore @$f $(python3 "$O/split_compare.py" "$R/vs-idle" --no-pictures --restore-check $f 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('differences',d['word_differences'])" 2>&1 | tail -1)"
done
for spec in "split-pause-menu/twop-plain 2290" "split-pause-menu/twop-plain 2850" "split-pause-menu/league 7400" "split-hud-gaps/review/zzap 3000" "split-hud-gaps/review/zzap 5600"; do
  set -- ${spec}
  echo "  $1 restore @$2 $(python3 "$O/split_compare.py" "$L/$1" --no-pictures --opponent-queue --idle-latch --restore-check $2 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('differences',d['word_differences'])" 2>&1 | tail -1)"
done''', writes=('{L}/split-pause-menu',))
    original('look', "the riders' look against the original (R-0083)", '''
for c in "$O/twop" "$O/twop-plain" "$O/league" "$O/review/zz2p" "$L/split-hud-gaps/review/zzap" "$R/vs-idle" "$R/league-idle" "$R/review/vs-opp" "$L/split-captions/review/mike1" "$L/split-captions/review/p2pause"; do
  echo "  $(basename $c) $(python3 "$L/split-riders-under-ink/look_compare.py" "$c" 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('look differences',d['differences'])" 2>&1 | tail -1)"
done
for spec in "split-race-end/vs-idle 2800" "split-race-end/league-idle 9000" "split-captions/review/p2pause 2420"; do
  set -- ${spec}
  echo "  $1 restore @$2 $(python3 "$O/split_compare.py" "$L/$1" --no-pictures --opponent-queue --idle-latch --restore-check $2 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('differences',d['word_differences'])" 2>&1 | tail -1)"
done''', writes=('{L}/split-pause-menu',))
    original('restart', 'a two-human restart against the original (R-0084): words, look, restores, picture runs', '''
T="$L/two-human-restart"
for c in vs-restart twop-restart league-restart vs-control; do
  echo "  $(python3 "$O/split_compare.py" "$T/$c" --no-pictures --opponent-queue --idle-latch 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print(d['capture'],'race frames',d['race_frames'],'differences',d['word_differences'],d['races'])")"
  echo "  $c $(python3 "$L/split-riders-under-ink/look_compare.py" "$T/$c" 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('look differences',d['differences'])" 2>&1 | tail -1)"
  python3 "$T/runs.py" "$T/$c" > "$G/runs-$c.txt" 2>&1; echo "  $c picture runs: $(sed -n '2,$p' "$G/runs-$c.txt" | wc -l | tr -d ' ') (runs-$c.txt)"
done
for spec in "vs-restart 2700" "vs-restart 4600" "twop-restart 3200" "twop-restart 4600" "league-restart 8600" "league-restart 9800"; do
  set -- ${spec}
  echo "  $1 restore @$2 $(python3 "$O/split_compare.py" "$T/$1" --no-pictures --opponent-queue --idle-latch --restore-check $2 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('differences',d['word_differences'])" 2>&1 | tail -1)"
done''', writes=('{L}/split-pause-menu',))
    original('idle-demo', 'the idle demo rotation against the original (R-0087): race words and trailers, pictures', '''
D="$L/idle-demo-cycles"; SHORT="--short-demo-title 13 --short-demo-title 16 --short-demo-title 18 --short-demo-title 21 --short-demo-title 33"
build/lab-release/src/core/front_end_runner --content-pack "$PACK" --frames 102000 --inputs "$L/split-screen-race/title-start-300-305.inputs" $SHORT --race-timeline "$TMPDIR/demo-timeline.txt" > /dev/null 2> "$G/demo-run.err"; echo "  lap run rc=$?"
python3 "$D/compare_cycles.py" "$D/cold-102000" "$TMPDIR/demo-timeline.txt" "$G/demo-compare.json" --trailer > "$G/demo-compare.log" 2>&1
rm -f "$TMPDIR/demo-timeline.txt"
echo "  $(python3 -c "import json,sys;d=json.load(open(sys.argv[1]));print('cycles',len(d),'exact',sum(1 for r in d if r['first_difference'] is None))" "$G/demo-compare.json")"
python3 "$D/pictures.py" "$D/cold-102000" build/lab-release/src/core/front_end_runner "$PACK" "$TMPDIR/demo-pictures" $SHORT 2>&1 | tail -1 | python3 -c "import json,sys;d=json.loads(sys.stdin.read());print('  pictures',d['pictures'],'equal',d['equal'])"
rm -rf "$TMPDIR/demo-pictures"''')
    original('league-frozen', "TWO-HUMAN-STUNTS: the league's frozen pictures (pack v38)", '''
python3 "$L/two-human-stunts/league_frozen.py" "$G/league-frozen.json" "local/classic-pal-crawler-tracks-v38.pack" build/lab-release/src/core/front_end_runner > "$G/league-frozen.log" 2>&1; echo "  rc=$? $(tail -1 "$G/league-frozen.log" | cut -c1-200)"''')
    original('checks4', "TWO-HUMAN-STUNTS: the review's captures and two-human restore checks (pass4/checks4.sh, pack v38)", '''
zsh "$L/two-human-stunts/pass4/checks4.sh" "$G/checks4" > "$G/checks4.out" 2>&1; echo "  rc=$?"; grep -E '^  ' "$G/checks4.out" | cut -c1-240''')
    original('credits-name', 'CREDITS-NAME: the capture and replays against the original', '''
CN="$L/credits-name"
echo "  $(python3 "$CN/compare.py" credits --pack "$PACK" --pictures-from 3400 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print('credits frames',d['frames_compared'],'pictures',d['pictures_equal'],'of',d['pictures'],'differing words',d['frames_differing'])")"
for c in faedine faedine-zoom faedine-count warm-hunter-faedine; do
  echo "  $(python3 "$CN/race_compare.py" $c --pack "$PACK" 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print(d['capture'],[(r['rows'],r['equal']) for r in d['races']])")"
done''')
    original('wipe-ram', 'WIPE-RAM: the eight captures against the original, their sound schedules', '''
WR="$L/wipe-ram"; mkdir -p "$G/cues"
for c in confirm cancel main-at-once pad2 start-held confirm-long twice partial; do
  # compare.py keeps its picture directory; give each capture its own TMPDIR and drop it after.
  T1="$TMPDIR/$c"; mkdir -p "$T1"
  echo "  $(TMPDIR="$T1" python3 "$WR/compare.py" $c --pack "$PACK" 2>&1 | tail -1 | python3 -c "import json,sys;d=json.load(sys.stdin);print(d['capture'],'frames',d['frames_compared'],'differing',len(d['frames_differing']),'pictures',d['pictures_equal'],'of',d['pictures'])")"
  rm -rf "$T1"
  W="$W" E="$WR/audio" N="$G/cues" zsh "$WR/audio/cues.sh" $c 2>/dev/null | grep -E 'EQUAL|DIFF' | cut -c1-70 | sed 's/^/  /'
done''')
    original('demo-audio-cues', "DEMO-AUDIO: the idle demos' cue schedules against the original", '''
DA="$L/demo-audio"; mkdir -p "$G/cues"
for n in demo-long demo-pad-early demo-pad-late demo-pad-port2; do
  W="$W" N="$G/cues" zsh "$DA/cues.sh" $n > /dev/null 2>&1
  echo "  $n: $(python3 "$DA/diff_cues.py" "$DA/$n.cues.txt" "$G/cues/native-$n.cues.txt" 620 --waits --limit=0 | head -1) (lines $(wc -l < "$DA/$n.cues.txt") / $(wc -l < "$G/cues/native-$n.cues.txt"))"
done
RV="$L/demo-audio-review"
for n in rv-late-split rv-early-p2 rv-long; do
  W="$W" E="$RV" N="$G/cues" zsh "$DA/cues.sh" $n > /dev/null 2>&1
  echo "  $n (review): $(python3 "$DA/diff_cues.py" "$RV/$n.cues.txt" "$G/cues/native-$n.cues.txt" 620 --waits --limit=0 | head -1) (lines $(wc -l < "$RV/$n.cues.txt"))"
done
echo "  rv-late-split with its way back held and its third title short (late_split_shift.sh): $(TMPDIR="$TMPDIR" zsh "$RV/late_split_shift.sh" "$W" "$PACK" | head -1)"
echo "  demo-cold (demo-long's first 10,000 frames): $(python3 "$DA/diff_cues.py" "$DA/demo-long.cues.txt" "$G/cues/native-demo-long.cues.txt" 620 9999 --waits --limit=0 | head -1)"''')
    original('mode-audio', "MODE-AUDIO's schedules, R-0077's one-player schedules and the cold cartridge image", '''
MA="$L/mode-audio"; mkdir -p "$G/cues"
for n in options-rename options-league twop-next vs-champions league-two; do
  W="$W" N="$G/cues" zsh "$MA/cues.sh" $n > /dev/null 2>&1
  echo "  $n: $(python3 "$MA/diff_cues.py" "$MA/$n.cues.txt" "$G/cues/native-$n.cues.txt" 620 --limit=0 | head -1)"
done
for n in six-quits lap-won bowl-lose bowl-quit hopper-gold fifth-win two-races cont-loss locked-gold hill-win hunter-t41-right hunter-t44-right; do
  echo "  $(W="$W" E="$L/audio-one-player" N="$G/cues" zsh "$MA/cues.sh" $n 2>/dev/null | grep -E 'EQUAL|DIFF' | cut -c1-70)"
done
build/lab-release/src/core/front_end_runner --content-pack "$PACK" --frames 430 --records 429 "$G/cold-429.bin" > /dev/null
cmp -s "$G/cold-429.bin" "$L/save-files/images/cold-429.bin" && echo "  cold cartridge image at 429 equals the original's" || echo "  cold image DIFFERS"''')
    original('split-demo', "the split demo's retained pictures (R-0069)", '''
echo "  $(python3 "$L/split-hud-gaps/demo_recheck.py" build/lab-release/src/core/front_end_runner "$PACK")"''')
    original('audio-r0076', "audio: R-0076's seven cue schedules, the candidate's cues against main's", '''
for v in primary quit loss variety continue back nav; do
  d="$A"; [ -f "$A/$v.script.json" ] || d="$A/review"
  n=$(python3 -c "import json;print(json.load(open('$d/$v.script.json'))['frames'])")
  build/lab-release/src/core/front_end_runner --content-pack "$PACK" --frames $n --inputs "$d/$v.frontend-inputs.txt" --sound-cues "$G/native-$v.cues.txt" > /dev/null 2> "$G/cues-$v.err"
  "$B/front_end_runner" --content-pack "$BASE_PACK" --frames $n --inputs "$d/$v.frontend-inputs.txt" --sound-cues "$G/main-$v.cues.txt" > /dev/null 2>&1
  cmp -s "$G/native-$v.cues.txt" "$G/main-$v.cues.txt" && echo "  $v: identical to main ($(wc -l < "$G/main-$v.cues.txt") lines)" || echo "  $v: DIFFERS from main"
done''')
    # MENU-INPUT's and LEAGUE-TOURS' checks.sh (35 and 57 minutes serially), one step per capture,
    # so that a difference re-runs only its own capture and the full run spreads them over the pool.
    mi_summary = '''summary() { python3 -c "
import json,sys; d=json.load(open(sys.argv[1]))
print(f\\"    {sys.argv[2]:>9}: frames {d['frames_compared']} differing {d['frames_differing']} pictures {d['pictures_equal']}/{d['pictures']} ranges {d['differing_ranges'][:6]}\\")" "$1" "$2"; }
'''
    for spec in ('w5:400 kb-input:400 league-pick:400 league-name:400 opt-backs:400 warnings:400 records-input:400 '
                 'scores-input:400 scores-carry:400 vs-challenger:6600 twop-quit:6600 awards-press:10500').split():
        c, first = spec.split(':')
        original(f'mi-{c}', f"MENU-INPUT's capture {c} against the original, candidate and main", temp_cap=GB * 3 // 2, command=mi_summary + f'''
E=local/evidence/menu-input; B0=$E/base-e304868/front_end_runner; C0=build/lab-release/src/core/front_end_runner
echo "  {c}:"
python3 $E/compare.py {c} --probe --binary $C0 --pack local/classic-pal-crawler-tracks-v39.pack --pictures-from {first} > "$G/{c}-candidate.json" 2>&1 && summary "$G/{c}-candidate.json" candidate
python3 $E/compare.py {c} --probe --binary $B0 --pack local/classic-pal-crawler-tracks-v39.pack --pictures-from {first} > "$G/{c}-main.json" 2>&1 && summary "$G/{c}-main.json" main''')
    for c in ('rv-ca', 'rv-scores', 'rv-bars', 'rv-records', 'rv-pick', 'rv-warn', 'rv-kb'):
        # rv-warn keeps 6,700 pictures a side.
        original(f'mi-{c}', f"MENU-INPUT's review capture {c} against the original, candidate and main",
                 temp_cap=(3 if c == 'rv-warn' else 2) * GB, command=mi_summary + f'''
R=local/evidence/menu-input-review; B0=local/evidence/menu-input/base-e304868/front_end_runner; C0=build/lab-release/src/core/front_end_runner
echo "  {c}:"
python3 $R/compare.py {c} --probe --binary $C0 --pack local/classic-pal-crawler-tracks-v39.pack --pictures-from 400 > "$G/{c}-candidate.json" 2>&1 && summary "$G/{c}-candidate.json" candidate
python3 $R/compare.py {c} --probe --binary $B0 --pack local/classic-pal-crawler-tracks-v39.pack --pictures-from 400 > "$G/{c}-main.json" 2>&1 && summary "$G/{c}-main.json" main''')
    original('mi-cues', "MENU-INPUT's sound schedules against the original's (frame waits included)", '''
E=local/evidence/menu-input; R=local/evidence/menu-input-review; mkdir -p "$G/cues"
for n in kb-input league-pick league-name opt-backs warnings records-input scores-input scores-carry w5 vs-challenger awards-press; do
  [ -f $E/$n.cues.txt ] || { echo "  $n: no schedule"; continue; }
  W="$PWD" N="$G/cues" zsh $E/cues.sh $n > /dev/null 2>&1
  echo "  $n: $(python3 $E/diff_cues.py $E/$n.cues.txt "$G/cues/native-$n.cues.txt" 620 --waits --limit=0 | head -1) (lines $(wc -l < $E/$n.cues.txt | tr -d ' ') / $(wc -l < "$G/cues/native-$n.cues.txt" | tr -d ' '))"
done
for n in rv-pick rv-scores; do
  W="$PWD" E="$PWD/$R" N="$G/cues" zsh $E/cues.sh $n > /dev/null 2>&1
  echo "  $n (review): $(python3 $E/diff_cues.py $R/$n.cues.txt "$G/cues/native-$n.cues.txt" 620 --waits --limit=0 | head -1)"
done
RV=local/evidence/mode-audio-review
W="$PWD" E="$PWD/$RV" N="$G/cues" zsh $E/cues.sh rv-twop-quit > /dev/null 2>&1
echo "  rv-twop-quit (MODE-AUDIO review): $(python3 $E/diff_cues.py $RV/rv-twop-quit.cues.txt "$G/cues/native-rv-twop-quit.cues.txt" 620 --waits --limit=0 | head -1)"''')
    lt_main = {'tours-a-w0', 'tours-a-w2', 'tours-b-w0', 'odd-tours-w0', 'picks-l0', 'podium-back', 'backs'}
    for c in ('tours-a-w0 tours-a-w1 tours-a-w2 tours-a-w3 tours-a-w4 tours-a-w5 tours-a-w6 tours-a-w7 tours-a-w8 tours-b-w0 '
              'tours-b-w1 odd-tours-w0 odd-tours-w1 picks-l0 picks-l1 picks-l2 podium-back t-crawler five-w0 five-w1 backs').split():
        main_side = (f'''
echo "  {c} (main):"; PACK=local/classic-pal-crawler-tracks-v39.pack bash $E/compare_capture.sh {c} "$G/main" $E/base-ceb50cf/front_end_runner | sed 's/^/  /'
'''
                     if c in lt_main else '')
        original(f'lt-{c}', f"LEAGUE-TOURS' capture {c} against the original", temp_cap=GB * 3 // 2, command=f'''
E=local/evidence/league-tours
echo "  {c} (candidate):"; PACK=local/classic-pal-crawler-tracks-v40.pack bash $E/compare_capture.sh {c} "$G/candidate" build/lab-release/src/core/front_end_runner | sed 's/^/  /'
''' + main_side)
    original('lt-cues', "LEAGUE-TOURS' sound schedules against the original's (frame waits included)", '''
E=local/evidence/league-tours
for n in $(ls $E/cues/*.cues.txt 2>/dev/null | grep -v /native- | xargs -n1 basename | sed 's/.cues.txt//'); do
  PACK="$PWD/local/classic-pal-crawler-tracks-v40.pack" W="$PWD" E="$PWD/$E/cues" N="$G" zsh local/evidence/menu-input/cues.sh $n > /dev/null 2>&1
  echo "  $n: $(python3 local/evidence/menu-input/diff_cues.py $E/cues/$n.cues.txt "$G/native-$n.cues.txt" 620 --waits --limit=0 | head -1) (lines $(wc -l < $E/cues/$n.cues.txt | tr -d ' ') / $(wc -l < "$G/native-$n.cues.txt" 2>/dev/null | tr -d ' '))"
done''')
    # PODIUM-AUDIO's checks.sh, one step per section; its section 5, the app's producer in real
    # time, is the exclusive step app-podium-audio above.
    pa = '''P="$L/podium-audio"; E="$P/cues"; RV="$L/podium-audio-review/cues"
PACK40="$W/local/classic-pal-crawler-tracks-v40.pack"; C=build/lab-release/src/core/front_end_runner; B0="$P/base-6c50ac1/front_end_runner"
'''
    original('pa-cues', "PODIUM-AUDIO: cue schedules against the original (frame waits included), candidate and main", pa + '''
O="$G"; mkdir -p "$O/cand" "$O/main"
for n in tour-cues full-tour pa-t-crawler pa-tours-b pa-tours-a pa-odd-tours rv-press1 rv-press2 rv-up-pad2; do
  D="$E"; case $n in rv-*) D="$RV";; esac
  [ -f "$D/$n.cues.txt" ] || { echo "  $n: not captured"; continue; }
  frames=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['frames'])" "$D/$n.script.json")
  rw=$(cat "$D/$n.record-writes.txt")
  for side in cand main; do
    bin="$C"; [ $side = main ] && bin="$B0"
    "$bin" --content-pack "$PACK40" --frames "$frames" --inputs "$D/$n.frontend-inputs.txt" $rw --sound-cues "$O/$side/native-$n.cues.txt" > /dev/null 2>&1
    echo "  $n ($side): $(python3 "$P/podium_windows.py" "$D/$n.cues.txt" "$O/$side/native-$n.cues.txt" 620 | tr '\\n' ' ' | sed 's/  */ /g')"
  done
done
python3 - "$O/cand/native-tour-cues.cues.txt" "$O/cand/native-tour-cues.old-view.txt" <<'PY'
import sys
rows, skip = [], 0
for line in open(sys.argv[1]):
    r = line.split()
    if r[1:2] == ['D'] and r[2].startswith('podium'): continue
    if r[1:3] == ['L', 'podium']: r[2] = 'ddcd'; skip = 3
    elif skip and r[1] == 'E' and (r[2], r[3]) in (('8', '255'), ('7', '127'), ('1', '2')): skip -= 1; continue
    rows.append(' '.join(r))
open(sys.argv[2], 'w').write('\\n'.join(rows) + '\\n')
PY
echo "  tour-cues against LEAGUE-TOURS' derivation (its conventions applied to native's log): $(python3 "$E/diff_cues.py" "$L/league-tours/cues/tour-cues.cues.txt" "$O/cand/native-tour-cues.old-view.txt" 620 --waits --limit=5 | tr '\\n' ' ')"
echo "  main against LEAGUE-TOURS' derivation: $(python3 "$E/diff_cues.py" "$L/league-tours/cues/tour-cues.cues.txt" "$O/main/native-tour-cues.cues.txt" 620 --waits --limit=0 | head -1)"''')
    add(Step('pa-anchors', "PODIUM-AUDIO: the podium's anchors (D-0010 calibration of the original; no native binary)", pa + '''
caps=("$L/league-tours/cues/tour-cues"); for n in full-tour pa-t-crawler pa-tours-b pa-tours-a pa-odd-tours; do [ -d "$E/$n-disp2" ] && caps+=("$E/$n"); done
(cd "$E" && python3 podium_anchors.py "$G/anchors.json" "${caps[@]}") | sed 's/^/  /' ''', system='audio', reads_status=False,
             select=lambda run: (True, 'full run') if run.context['full'] else (False, 'reads only the original\'s captures')))
    add(Step('pa-cond', "PODIUM-AUDIO: the native driver on the original's port writes (tour-cues-full)", pa + '''
R="$PWD/build/lab-release/tests/native/audio_driver_runner" zsh "$E/cond.sh" tour-cues-full title race-62 title award title race-63 > "$G/cond.log" 2>&1
echo "  $(python3 -c "import json,sys;d=json.load(open(sys.argv[1]));print('rows',d['expected_rows'],d['native_rows'],'first row difference',d['first_row_difference'],'pairs',d['original_pairs'],d['native_pairs'],'pcm equal',d['pcm_equal'])" "$E/cond-tour-cues-full.json" 2>&1 | tail -1)"''',
             system='audio', reads_status=False, writes=('{L}/podium-audio/cues',), select=drives('audio_driver_runner')))
    original('pa-pcm', "PODIUM-AUDIO: anchored PCM of the candidate's cues and the pass that delivers the music start", pa + '''
zsh "$E/podium_pcm.sh" "$RUN/pa-cues/cand" "$G/pcm" 2>&1
zsh "$E/native_commands.sh" "$RUN/pa-cues/cand" 2>&1''', also=('race_audio_runner',), writes=('{L}/podium-audio',))
    s[-1].after = ('fe-replay', 'pa-cues')
    original('pa-league-frozen', "PODIUM-AUDIO: LEAGUE's frozen gate (pack v40)", '''
python3 "$L/two-human-stunts/league_frozen.py" "$G/league-frozen.json" local/classic-pal-crawler-tracks-v40.pack build/lab-release/src/core/front_end_runner > "$G/league-frozen.log" 2>&1
echo "  league frozen: $(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['status'])" "$G/league-frozen.json" 2>&1)"''')
    # RECORD-SCREENS' checks join here once it is on main (until then they are its own task checks).

    add(Step('demo-audio-cond', "DEMO-AUDIO: the native driver on the demo's sessions (demo-cold-full)", PRELUDE + '''
DA="$L/demo-audio"
R="$W/build/lab-release/tests/native/audio_driver_runner" zsh "$DA/cond.sh" demo-cold-full title demo title demo title demo title > "$G/cond.log" 2>&1
echo "  $(python3 -c "import json;d=json.load(open('$DA/cond-demo-cold-full.json'));print('rows',d['expected_rows'],d['native_rows'],'first row difference',d['first_row_difference'],'pairs',d['original_pairs'],d['native_pairs'],'pcm equal',d['pcm_equal'])" 2>&1 | tail -1)"''',
             system='audio', reads_status=False, writes=('{L}/demo-audio',), select=drives('audio_driver_runner')))

    # -- the task's own checks: always ------------------------------------------------------------
    for step_id, command, shell in task_checks:
        add(Step(step_id, f"the task's own checks: {step_id}", command, shell=shell, system='task',
                 reads_status=False, record=True, timeout=5400, select=lambda run: (True, "the task's own acceptance")))

    # -- static checks ---------------------------------------------------------------------------
    add(Step('tooling', 'tooling tests', '''
python3 -m unittest discover -s tests/tooling > "$G/tooling.log" 2>&1; rc=$?; echo "  $(tail -1 "$G/tooling.log") $(grep '^Ran' "$G/tooling.log")"; exit $rc''',
             system='rules', select=code))
    add(Step('rules', 'rules: function size and the address index', '''
/opt/homebrew/opt/llvm/bin/clang-tidy -p build/app-debug src/core/*.cpp src/app/*.cpp > "$G/function-size.log" 2>/dev/null
n=$(grep -c 'warning: function' "$G/function-size.log"); echo "  functions over 80 lines: $n"
python3 tools/project.py coverage native-symbols --check > "$G/native-symbols.log" 2>&1; echo "  native-symbols $(grep status= "$G/native-symbols.log" | tail -1)"
[ "$n" = 0 ] && grep -q status=passed "$G/native-symbols.log"''', system='rules', weight=4, select=code))
    # Pool order is start order: the long and wide steps first, the corpus checks once the replay
    # has said which of them must run.
    first = [x for x in s if x.lane != 'pool']
    pool = [x for x in s if x.lane == 'pool']
    rank = {'task': 0, 'fe-replay': 1, 'race-sweep': 2}
    pool.sort(key=lambda x: rank.get(x.system if x.system == 'task' else x.id, 3))
    return first + pool
