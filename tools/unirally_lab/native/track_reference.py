"""TRACK-BREADTH laboratory: capture any Crawler track through the original menu with the
controller released, and compare the original's race projection (742 bytes, and for any track but
DRAGSTER and ZOOM ZOO the 34 special-tile bytes of R-0047 and the 60 checkpoint bytes of
R-0048) with native.

Original only on the capture side; never a native runtime input. The menu path is the
accepted ZOOM ZOO prefix of `tests/manifests/replay/race-crawler-zoom-zoo-3300.json`
(1P, MIKE, CRAWLER by Start presses) with one Down on PICK TRACK per position after
DRAGSTER, 20 frames apart, then Start on PICK TRACK and Start (Race) on NOW PLAYING. The
controller is released from then on. For ZOOM ZOO (one Down) the inputs equal the
M4-16 idle original's up to its first riding input at frame 2650.

The race initialization boundary is found, not assumed: the end of the frame before
fade `$0FF1` first advances from 0, with the countdown `$11C5` at 270 (1,328 on
DRAGSTER and 1,376 on ZOOM ZOO; see `dragster_playable_reference`). The countdown
reads 270 for some 85 frames before that, so the first such frame is not the boundary. The comparison
aligns updates from that boundary; native labels its rows from its own scenario's
initialization frame, so the magic and frame label (row bytes 0-11) are excluded
from the comparison and reported separately.

Raw WRAM/SRAM series stay under ignored `artifacts/`.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
from .zoom_zoo_trial_reference import ROOT, ROM_SHA, CORE_SHA, sha, digest
from .zoom_zoo_race_reference import project
from .zoom_zoo_playable import ROLL_WORDS
from .classic_race_layout import describe, special_tile_bytes, checkpoint_tail_bytes, hunter_bytes, hud_captions
from ..content.commands import write_track_override
from ..reference.bsnes import BsnesCore, BUTTONS, frame_png
from .zoom_zoo_trial import BUTTONS as RUNNER_BUTTONS  # the runner's controller-row bit order

FIRST_RECORDED_FRAME = 1100
MENU_STARTS = ((300, 305), (620, 625), (750, 755), (900, 905))
FIRST_DOWN = 1000
FIRST_TOUR_DOWN = 850
DOWN_SPACING = 20
GUARDS_PATH = 'tests/manifests/native/zoom-zoo-race-guards.reference.json'
# ZOOM ZOO applies its constant-domain guards from end-1649, 273 updates after its boundary.
GUARD_OFFSET = 1649 - 1376


def menu_events(position, tour_row=0, tour_column=0):
    """(from, to, button) for the track at ``position`` (0-4) on the PICK TRACK screen of the
    tour at ``tour_row`` and ``tour_column`` of PICK TOUR (row 0, column 0 = CRAWLER, the default
    highlight). Down walks the left column (CRAWLER, SHUFFLER, WALKER, HOPPER, then HUNTER once
    the locked tours are open); Right takes the right column (JUMPER, BOUNDER, RUNNER, SPRINTER)."""
    if not 0 <= position <= 4 or not 0 <= tour_row <= 4 or tour_column not in (0, 1) or \
            (tour_column == 1 and tour_row == 4):
        raise ValueError('a track position is 0-4, a PICK TOUR row 0-4 and a column 0-1 (row 4 has no column 1)')
    events = [(a, b, 'start') for a, b in MENU_STARTS[:3]]
    shift = 0
    for k in range(tour_row + tour_column):
        at = FIRST_TOUR_DOWN + DOWN_SPACING * k
        events.append((at, at + 5, 'down' if k < tour_row else 'right'))
        shift = max(shift, at + 5 + DOWN_SPACING - MENU_STARTS[3][0])
    events.append((MENU_STARTS[3][0] + shift, MENU_STARTS[3][1] + shift, 'start'))
    last = FIRST_DOWN + shift - DOWN_SPACING
    for k in range(position):
        last = FIRST_DOWN + shift + DOWN_SPACING * k
        events.append((last, last + 5, 'down'))
    pick = max(1050 + shift, last + 5 + DOWN_SPACING)
    events += [(pick, pick + 5, 'start'), (pick + 150, pick + 155, 'start')]
    return events


def segments(hold):
    """``hold`` as a list of (first frame, buttons): one pair, or a list of them."""
    if hold is None:
        return []
    if isinstance(hold[0], int):
        hold = [hold]
    return sorted((int(first), list(buttons)) for first, buttons in hold)


def timeline(position, horizon, tour_row=0, hold=None, tour_column=0):
    """The menu inputs, then a released controller, or ``hold``: (first frame, buttons) held
    from that frame to the horizon, or several such segments, each held until the next
    begins (an empty button list releases the controller)."""
    rows = [[[], []] for _ in range(horizon + 1)]
    events = menu_events(position, tour_row, tour_column)
    for first, last, button in events:
        for frame in range(first, last + 1):
            rows[frame][0] = [button]
    held = segments(hold)
    for i, (first, buttons) in enumerate(held):
        if first <= max(e[1] for e in events):
            raise ValueError('a held input must start after the menu')
        end = held[i+1][0] if i+1 < len(held) else horizon + 1
        for frame in range(first, end):
            rows[frame][0] = sorted(buttons)
    return rows


# LOCKED-TOURS: a nonzero $77:1000 lists all nine tours on PICK TOUR, but choosing a locked
# tour also needs bytes in both $77:10C0-$10DF and $10E0-$10FF; with $1000 and $10C0-$10FF all
# $FF every tour opens. (Filling the whole $1000-$1FFF also reaches the SRAM graph extrema at
# $106F and the tutorial hints, so the preload is kept to those bytes.) A capture of a locked
# tour preloads them (original side only, recorded in the reference).
UNLOCK_TOURS_SRAM = [0x1000, *range(0x10c0, 0x1100)]


def unlocked_sram(core_path, rom):
    """Cartridge RAM for a locked-tour capture: a fresh power-on's RAM (all $FF) is formatted by
    the menu (frames 403-405, after the first Start), which would clear the unlock, so boot once
    through the cold menu to frame 600, after the format and before any choice, take the
    formatted RAM, and set $77:1000 and $10C0-$10FF to $FF."""
    with tempfile.TemporaryDirectory() as directory:
        core = BsnesCore(core_path, Path(directory), {})
        try:
            core.load(rom)
            core.set_serialization_method('Strict')
            rows = timeline(0, 1400, 0)
            for frame in range(601):
                core.set_inputs(0, set(rows[frame][0])); core.set_inputs(1, set())
                core.run_frame()
            image = bytearray(core.cartridge_ram())
        finally:
            core.unload()
    if image[UNLOCK_TOURS_SRAM[0]] != 0:
        raise ValueError('the cold menu left cartridge RAM unformatted')
    for address in UNLOCK_TOURS_SRAM:
        image[address] = 0xff
    return bytes(image)


def capture(core_path, out, track, horizon, frame_images=(), tour_row=0, hold=None, tour_column=0, unlock_tours=False):
    if out.exists():
        raise ValueError('fresh output directory required')
    if any(b not in BUTTONS for _, buttons in segments(hold) for b in buttons):
        raise ValueError(f'a held input names buttons from {BUTTONS}')
    rom = Path((ROOT/'local/rom-location.txt').read_text().strip())
    if (sha(core_path.read_bytes()), sha(rom.read_bytes())) != (CORE_SHA, ROM_SHA):
        raise ValueError('original identities differ')
    if (tour_column or tour_row == 4) and not unlock_tours:
        raise ValueError('a locked tour needs --unlock-tours')
    preload = unlocked_sram(core_path, rom) if unlock_tours else None
    inputs = timeline(track, horizon, tour_row, hold, tour_column)
    out.mkdir(parents=True)
    hashes, cartridge_hashes, video = [], [], []
    boundary = None
    held = None  # last recorded frame with $0FF1 = 0 and $11C5 = 270
    with tempfile.TemporaryDirectory(dir=out) as directory:
        if preload is not None:
            # The core loads cartridge RAM from its save directory at power-on, as a
            # console loads its battery RAM; a write into memory after load is lost to the
            # reset the Strict serialization method performs.
            (Path(directory)/(rom.stem+'.srm')).write_bytes(preload)
        core = BsnesCore(core_path, Path(directory), {})
        try:
            core.load(rom)
            core.set_serialization_method('Strict')
            with (out/'memory.wram').open('xb') as ws, (out/'memory.sram').open('xb') as ss:
                for frame, ports in enumerate(inputs):
                    for port, buttons in enumerate(ports):
                        core.set_inputs(port, set(buttons))
                    core.keep_frame = frame in frame_images
                    result = core.run_frame()
                    if core.keep_frame and core.frame_raw:
                        (out/f'frame-{frame}.png').write_bytes(frame_png(*core.frame_raw))
                    if frame < FIRST_RECORDED_FRAME:
                        continue
                    w, s = core.wram(), core.cartridge_ram()
                    fade, countdown = int.from_bytes(w[0xff1:0xff3], 'little'), int.from_bytes(w[0x11c5:0x11c7], 'little')
                    if boundary is None:
                        if fade == 0 and countdown == 270:
                            held = frame
                        elif fade == 1 and held == frame - 1:
                            boundary = held
                    ws.write(w); ss.write(s)
                    hashes.append(sha(w)); cartridge_hashes.append(sha(s))
                    video.append(result.video)
        finally:
            core.unload()
    report = dict(kind='track_breadth_original', position=track, frames=[FIRST_RECORDED_FRAME, horizon],
                  initialization_frame=boundary, tour_row=tour_row, tour_column=tour_column, unlock_tours=unlock_tours,
                  preload_sram_sha256=sha(preload) if preload is not None else None,
                  hold=hold, menu_events=menu_events(track, tour_row, tour_column), rom_sha256=ROM_SHA, core_sha256=CORE_SHA,
                  timeline_sha256=digest(inputs), timeline=inputs, wram_sha256=hashes, sram_sha256=cartridge_hashes, video=video)
    (out/'reference.json').write_text(json.dumps(report, separators=(',', ':'))+'\n')
    return dict(position=track, tour_row=tour_row, initialization_frame=boundary, wram=digest(hashes), sram=digest(cartridge_hashes), video=digest(video))


# Result-loading updates until the result screen is stable, by race mode $77:074B and outcome:
# a lap race's (ZOOM ZOO, M4-16) and a one-run race's (DRAGSTER, R-0012/R-0019), as native's
# scenario table uses them.
STABLE_RESULT = {1: dict(player_won=115, player_lost=115), 0: dict(player_won=226, player_lost=242)}


def original_rows(directory):
    """Projections from the boundary through both finishes and the result load, with guard
    violations; stops at the first projection error. Once result loading begins the original
    reuses race WRAM, so the last race row is archived and only the result clock, the graph
    extrema and published totals (SRAM) advance, as in `zoom_zoo_playable` (R-0049)."""
    document = json.loads((directory/'reference.json').read_text())
    if (document['rom_sha256'], document['core_sha256']) != (ROM_SHA, CORE_SHA):
        raise ValueError('original identity differs')
    first, last = document['frames']
    boundary = document['initialization_frame']
    if boundary is None:
        raise ValueError('the capture never reached a race initialization boundary')
    guards = json.loads((ROOT/GUARDS_PATH).read_text())['items']
    rows, violations, error = [], {}, None
    paused_updates = countdown_paused = 0
    previous = None
    finish, loading, archive, extras_archive, tail_archive = [None, None], None, None, None, b''
    captions = caption_rows = None
    mode = None
    with (directory/'memory.wram').open('rb') as ws, (directory/'memory.sram').open('rb') as ss:
        for frame in range(first, last+1):
            w, s = ws.read(131072), ss.read(8192)
            if sha(w) != document['wram_sha256'][frame-first] or sha(s) != document['sram_sha256'][frame-first]:
                raise ValueError(f'original memory differs at {frame}')
            if frame < boundary:
                continue
            if mode is None:
                mode = s[0x74b]
            if loading is None:
                for r in (0, 1):
                    if finish[r] is None and int.from_bytes(w[0xeff+2*r:0xf01+2*r], 'little'):
                        finish[r] = frame
                if archive is not None and int.from_bytes(archive[515:517], 'little') == 240:
                    loading = frame
            if loading is not None:
                # $83:904A-90F0 and $80:F88D publish the graph extrema and totals to SRAM;
                # the laps and totals of both riders survive there unchanged.
                row = bytearray(archive); row[8:12] = frame.to_bytes(4, 'little')
                stable = STABLE_RESULT[mode]['player_won' if finish[0] <= finish[1] else 'player_lost']
                row[-2:] = min(stable, frame-loading+1).to_bytes(2, 'little')
                if row[467:511] != s[0x755:0x769]+s[0x7bf:0x7d3]+s[0x769:0x76b]+s[0x7d3:0x7d5]:
                    error = dict(frame=frame, error='result lap/total archive differs from original (left the result screen?)')
                    break
                rows.append((row+s[0x106f:0x1073]+s[0x618:0x61c]+extras_archive+tail_archive).hex())
                continue
            # $82:AAA4-AAB4 publish the player's A, X and Start; the timeline must agree
            # (the accepted original() checks the same, from its guard frame on).
            for at, button in ((0x31d, 'a'), (0x321, 'x'), (0x339, 'start')):
                if frame >= boundary + GUARD_OFFSET and int.from_bytes(w[at:at+2], 'little') != int(button in document['timeline'][frame][0]):
                    raise ValueError(f'player {button} publication differs from the controller timeline at {frame}')
            if frame >= boundary + GUARD_OFFSET:
                for item in guards:
                    at = item['address']
                    if at in (0xd53, 0xd55, 0x31d, 0x321, 0x339) or at in {a+2*r for a in ROLL_WORDS for r in (0, 1)}:
                        continue
                    value = int.from_bytes(w[at:at+item['width']], 'little')
                    if value != item['value']:
                        violations.setdefault(f'{at:04x}', dict(frame=frame, value=value, guarded=item['value']))
                # R-0052: the HUNTER opponent's voices 232-247 index the learned bank past its end
                # at $7E21E8-$7E21F7, as the other tours' 200-215 do at the manifest's $7E21C9-$7E21D8;
                # native takes the original's zero-weight exit, which these bytes being zero justifies.
                for at in range(0x21e8, 0x21f8):
                    if w[at]:
                        violations.setdefault(f'{at:04x}', dict(frame=frame, value=w[at], guarded=0))
            try:
                projected = project(w, s, frame)
            except (ValueError, AssertionError) as exc:
                error = dict(frame=frame, error=str(exc))
                break
            row = bytearray(projected+w[0xff1:0xff3]+w[0x1261:0x1265]+b'\0\0')
            archive = bytearray(row)
            charge = w[0xd53:0xd57]
            announcements = (w[0xcc1:0xce1]+w[0xce7:0xce8]+w[0xce9:0xcea]+w[0xca5:0xca7]+s[0x7bb:0x7bd]+w[0x20e8:0x20e9]
                             +w[0x12e3:0x12e5]+w[0x12eb:0x12ed]+w[0x12ef:0x12f1]+w[0x3ed:0x3ef])
            roll = b''.join(w[a+2*r:a+2*r+2] for r in (0, 1) for a in ROLL_WORDS)
            weights = w[0x20e9:0x2102]+w[0x2103:0x211c]
            if previous is not None and (int.from_bytes(previous[0xef3:0xef5], 'little') or int.from_bytes(w[0x339:0x33b], 'little')):
                paused_updates += 1
                if int.from_bytes(w[0xff1:0xff3], 'little') >= 5 and int.from_bytes(previous[0x11c5:0x11c7], 'little'):
                    countdown_paused += 1
            pause = w[0xef3:0xef7]+paused_updates.to_bytes(4, 'little')+countdown_paused.to_bytes(4, 'little')
            extras_archive = charge+announcements+roll+weights+pause
            row += s[0x106f:0x1073]+s[0x618:0x61c]+extras_archive
            # Any track but DRAGSTER and ZOOM ZOO: its state (URTRnn06) appends
            # the special-tile words (R-0047) and the last checkpoint flags (R-0048).
            if s[0x74a] not in (0, 1):
                if captions is None:
                    rom = Path((ROOT/'local/rom-location.txt').read_text().strip()).read_bytes()
                    captions, caption_rows = hud_captions(rom), hud_captions(rom, range(1, 256))
                tail_archive = special_tile_bytes(w)+checkpoint_tail_bytes(w)+hunter_bytes(w, captions, caption_rows)
                row += tail_archive
            rows.append(row.hex())
            previous = w
    scenario = {'laps_0744': None, 'race_mode_074b': None, 'track_074a': None}
    with (directory/'memory.sram').open('rb') as ss:
        ss.seek((boundary-first)*8192)
        s = ss.read(8192)
        scenario = {'track_074a': s[0x74a], 'race_mode_074b': s[0x74b], 'laps_0744': s[0x744]}
    events = dict(finish_frames=finish, loading_frame=loading)
    if loading is not None:
        # As zoom_zoo_playable: the result is black through loading + 75, and the accepted
        # tracks' first visible picture is loading + 108.
        video = document['video']
        if loading+75 <= last:
            black = video[loading+75-first]
            events['first_visible_result'] = next((f for f in range(loading+76, last+1) if video[f-first] != black), None)
    return document, rows, dict(guard_violations=violations, projection_stop=error, scenario=scenario, **events)


def native_rows(binary, pack, track, count, scenario, hold=None, controller=None):
    rom = Path((ROOT/'local/rom-location.txt').read_text().strip()).read_bytes()
    with tempfile.TemporaryDirectory(prefix='track-native-') as directory:
        root = Path(directory)
        write_track_override(rom, track, root/'content')
        empty = root/'none.txt'; empty.write_text('')
        base = [str(binary), '--start', scenario, '--content-pack', str(pack)]
        start = int(subprocess.run(base+['--inputs', str(empty)], capture_output=True, text=True, timeout=60).stdout.split()[0])
        inputs = root/'inputs.txt'
        # The capture's held input, if any, by update (it starts that many updates
        # after the original's boundary, whatever native's frame label).
        mask, first = 0, None
        if hold is not None:
            first, buttons = hold
            mask = sum(1 << RUNNER_BUTTONS.index(b) for b in buttons)
        if controller is not None:
            # The original's own controller, update k being frame boundary + k.
            masks = [sum(1 << RUNNER_BUTTONS.index(b) for b in buttons if b in RUNNER_BUTTONS) for buttons in controller]
            inputs.write_text(''.join(f'{start+k} {masks[k-1]} 0\n' for k in range(1, count)))
        else:
            inputs.write_text(''.join(f'{start+k} {mask if first is not None and k >= first else 0} 0\n' for k in range(1, count)))
        # A per-track scenario (classic.track.NN, pack profile v10) takes the
        # track's content from the pack itself; a race-mode scenario takes it
        # from the laboratory override.
        override = [] if scenario.startswith('classic.track.') else ['--track-override', str(root/'content')]
        run = subprocess.run(base+override+['--inputs', str(inputs)], capture_output=True, text=True, timeout=600)
        return [line.split()[1] for line in run.stdout.splitlines()], run.returncode, run.stderr.strip(), start


def explore(reference, binary, pack, scenario):
    document, rows, events = original_rows(reference)
    # The track the original loaded (SRAM $77:074A at the boundary), not the menu position.
    track = events['scenario']['track_074a']
    boundary = document['initialization_frame']
    controller = [document['timeline'][boundary+k][0] for k in range(1, len(rows))]
    actual, code, error, native_start = native_rows(binary.resolve(), pack.resolve(), track, len(rows), scenario,
                                                    controller=controller)
    divergence = None
    for i, (x, y) in enumerate(zip(actual, rows)):
        a, b = bytes.fromhex(x), bytes.fromhex(y)
        if a[12:] != b[12:]:
            divergence = dict(update=i, original_frame=document['initialization_frame']+i,
                              fields=describe(a[:8]+b[8:12]+a[12:], b))
            break
    exact = divergence['update'] if divergence else min(len(actual), len(rows))
    return dict(track=track, initialization_frame=document['initialization_frame'], native_scenario=scenario,
                native_initialization_frame=native_start, original_rows=len(rows), native_rows=len(actual),
                native_exit=code, native_error=error, exact_updates_from_boundary=exact, first_divergence=divergence, **events)


# Race mode `$77:074B` at the boundary -> the native scenario whose mode it is
# (0 DRAGSTER's one-way race, 1 ZOOM ZOO's lap race); mode 2 is the stunt event,
# which no native scenario models.
SCENARIO_FOR_MODE = {0: 'classic.crawler.dragster', 1: 'classic.crawler.zoom-zoo'}


def sweep(core_path, out, binary, pack, horizon, tour_rows=range(4), positions=range(5)):
    """Capture every reachable (tour row, position) twice, then compare the first with native."""
    import shutil
    rows = []
    for tour_row in tour_rows:
        for position in positions:
            name = f'row{tour_row}-pos{position}'
            pick = [e for e in menu_events(position, tour_row) if e[2] == 'start'][-2][0]
            a = capture(core_path, out/name, position, horizon, {pick + 140}, tour_row)
            b = capture(core_path, out/f'{name}-repeat', position, horizon, (), tour_row)
            shutil.rmtree(out/f'{name}-repeat')
            repeat = {k: a[k] == b[k] for k in ('wram', 'sram', 'video', 'initialization_frame')}
            document, original, events = original_rows(out/name)
            mode = events['scenario']['race_mode_074b']
            scenario = SCENARIO_FOR_MODE.get(mode)
            row = dict(tour_row=tour_row, position=position, capture=name, now_playing_frame=pick + 140,
                       repeat_identical=all(repeat.values()), repeat=repeat, **events['scenario'],
                       initialization_frame=document['initialization_frame'], native_scenario=scenario)
            if scenario is not None:
                result = explore(out/name, binary, pack, scenario)
                row.update({k: result[k] for k in ('original_rows', 'native_rows', 'native_exit', 'native_error',
                                                   'exact_updates_from_boundary', 'first_divergence', 'projection_stop')})
                row['guard_violations'] = sorted(result['guard_violations'])
            rows.append(row)
            print(json.dumps({k: row.get(k) for k in ('capture', 'track_074a', 'race_mode_074b', 'laps_0744', 'repeat_identical',
                                                     'exact_updates_from_boundary', 'native_rows', 'original_rows', 'native_error')}), flush=True)
    (out/'sweep.json').write_text(json.dumps(dict(kind='track_breadth_sweep', horizon=horizon, rows=rows), indent=1, default=list)+'\n')
    return rows


def native_scenario(track, mode, per_track):
    """The native scenario to compare a captured race with: the track's own
    (TRACK-BREADTH part 3) or the accepted track of the same race mode."""
    if mode not in SCENARIO_FOR_MODE:
        return None
    if per_track and track not in (0, 1):
        return f'classic.track.{track:02d}'
    return SCENARIO_FOR_MODE[mode]


def recompare(sweep_dir, binary, pack, per_track, out):
    """Compare an earlier sweep's captures with native again, without recapturing."""
    rows = []
    for row in json.loads((sweep_dir/'sweep.json').read_text())['rows']:
        scenario = native_scenario(row['track_074a'], row['race_mode_074b'], per_track)
        result = dict(track=row['track_074a'], race_mode=row['race_mode_074b'], laps=row['laps_0744'], native_scenario=scenario)
        if scenario is not None:
            r = explore(sweep_dir/row['capture'], binary, pack, scenario)
            result.update({k: r[k] for k in ('original_rows', 'native_rows', 'native_exit', 'native_error',
                                             'exact_updates_from_boundary', 'first_divergence', 'native_initialization_frame',
                                             'initialization_frame')})
        rows.append(result)
        print(json.dumps({k: result.get(k) for k in ('track', 'native_scenario', 'exact_updates_from_boundary', 'native_rows',
                                                    'original_rows', 'native_error')}), flush=True)
    out.write_text(json.dumps(dict(kind='track_breadth_recompare', sweep=str(sweep_dir), per_track=per_track, rows=rows),
                              indent=1, default=list)+'\n')
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='command', required=True)
    c = sub.add_parser('capture')
    c.add_argument('--core', type=Path, required=True)
    c.add_argument('--track', type=int, required=True, help='position on the PICK TRACK screen, 0-4')
    c.add_argument('--tour-row', type=int, default=0, help='row on the PICK TOUR screen, 0-4')
    c.add_argument('--tour-column', type=int, default=0, help='column on the PICK TOUR screen, 0-1')
    c.add_argument('--unlock-tours', action='store_true', help='preload cartridge RAM $77:1000 and $10C0-$10FF with $FF (all nine tours)')
    c.add_argument('--out', type=Path, required=True)
    c.add_argument('--horizon', type=int, required=True)
    c.add_argument('--frame-image', type=int, action='append', default=[])
    c.add_argument('--hold', nargs='+', action='append', metavar=('FRAME', 'BUTTON'),
                   help='hold BUTTONs from FRAME until the next --hold or the horizon; repeatable, '
                        'and a FRAME alone releases the controller')
    e = sub.add_parser('explore')
    e.add_argument('--reference', type=Path, required=True)
    e.add_argument('--binary', type=Path, required=True)
    e.add_argument('--pack', type=Path, required=True)
    e.add_argument('--scenario', default='classic.crawler.zoom-zoo')
    e.add_argument('--out', type=Path, required=True)
    w = sub.add_parser('sweep')
    w.add_argument('--core', type=Path, required=True)
    w.add_argument('--out', type=Path, required=True)
    w.add_argument('--binary', type=Path, required=True)
    w.add_argument('--pack', type=Path, required=True)
    w.add_argument('--horizon', type=int, required=True)
    r = sub.add_parser('recompare')
    r.add_argument('--sweep', type=Path, required=True, help='directory holding an earlier sweep.json and its captures')
    r.add_argument('--binary', type=Path, required=True)
    r.add_argument('--pack', type=Path, required=True)
    r.add_argument('--per-track', action='store_true', help="compare on each track's own scenario (classic.track.NN)")
    r.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    if a.command == 'recompare':
        recompare(a.sweep, a.binary, a.pack, a.per_track, a.out)
    elif a.command == 'sweep':
        a.out.mkdir(parents=True)
        sweep(a.core.resolve(), a.out, a.binary, a.pack, a.horizon)
    elif a.command == 'capture':
        hold = [(int(h[0]), h[1:]) for h in a.hold] if a.hold else None
        if hold is not None and len(hold) == 1:
            hold = hold[0]
        print(json.dumps(capture(a.core.resolve(), a.out, a.track, a.horizon, set(a.frame_image), a.tour_row, hold,
                                 a.tour_column, a.unlock_tours)))
    else:
        result = explore(a.reference, a.binary, a.pack, a.scenario)
        a.out.write_text(json.dumps(result, indent=1, default=list)+'\n')
        print(json.dumps({k: result[k] for k in ('track', 'initialization_frame', 'original_rows', 'native_rows', 'native_exit',
                                                 'exact_updates_from_boundary', 'projection_stop', 'scenario')}, default=list))


if __name__ == '__main__':
    main()
