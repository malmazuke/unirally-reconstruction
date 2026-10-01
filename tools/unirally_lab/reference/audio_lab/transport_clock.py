"""Semantic timing experiment for the recovered CPU sound queue.

This is a laboratory prototype, not a game CPU interpreter. It reads no ROM or
instruction bytes. The source reading is main's ignored static map bank-82.lst,
$82:8035-806D; bus timing comes from pinned bsnes CPU memory/timing.cpp.
Entry clocks are supplied observations in diagnostics: this does not yet recover
cold producer timing, driver acknowledgments, interrupt or DMA costs.
"""
from dataclasses import dataclass


@dataclass
class PalBusClock:
    """Master clocks, PAL non-interlace, CPU version 2, slow ROM, no H/DMA.

    A CPU bus step checks DRAM refresh after stepping. Refresh lasts 40 clocks;
    its position is 538 minus the scanline-start CPU counter modulo eight.
    Reads split their bus clocks before/after the data access, unlike writes.
    """
    ticks: int

    def __post_init__(self) -> None:
        self._line = self.ticks // 1364
        self._refreshed = self.ticks % 1364 >= self.refresh_position()

    def refresh_position(self) -> int:
        return 538 - (self._line * 1364 % 8)

    def step(self, clocks: int) -> None:
        if clocks not in (2, 4, 6, 8, 10, 12):
            raise ValueError('CPU step must be an even bus interval in 2..12')
        self.ticks += clocks
        line = self.ticks // 1364
        if line != self._line:
            self._line, self._refreshed = line, False
        if not self._refreshed and self.ticks % 1364 >= self.refresh_position():
            self._refreshed = True
            self.ticks += 40

    def rom_bytes(self, count: int) -> None:
        for _ in range(count):
            self.step(4); self.step(4)

    def ram_read(self, count: int = 1) -> None:
        for _ in range(count):
            self.step(4); self.step(4)

    def ram_write(self, count: int = 1) -> None:
        for _ in range(count):
            self.step(8)

    def idle(self, count: int = 1) -> None:
        for _ in range(count):
            self.step(6)

    def port_read(self) -> int:
        self.step(2)
        accessed = self.ticks
        self.step(4)
        return accessed

    def port_write(self) -> int:
        self.step(6)
        return self.ticks


def begin_receiver_poll(clock: PalBusClock, interrupts=None) -> int:
    """Preserve caller/widths and read acknowledgment ($8035-$803F).

    Optional native menu interrupts recognize an edge before each operation's
    last bus cycle, then resume between operations, matching the CPU pipeline.
    """
    def begin():
        if interrupts: interrupts.before_operation(clock)
    def finish(final):
        if interrupts: interrupts.last_cycle(clock)
        return final()
    begin(); clock.rom_bytes(1); clock.idle(); finish(clock.ram_write)
    begin(); clock.rom_bytes(2); finish(clock.idle)
    for _ in range(2):
        begin(); clock.rom_bytes(1); clock.idle(); clock.ram_write()
        finish(clock.ram_write)
    begin(); clock.rom_bytes(2); finish(clock.idle)
    begin(); clock.rom_bytes(3)
    return finish(clock.port_read)


def send_ready_command(clock: PalBusClock) -> tuple[int, int, int]:
    """Ready receiver and nonempty queue: $803F through both $806D writes.

    Readiness and queue contents must be supplied by causal state; this helper
    only predicts elapsed bus time on that path. It returns store-entry, low
    port write and high port write clocks. No observed timestamps are embedded.
    """
    clock.rom_bytes(4); clock.ram_read()                # expected acknowledgment
    clock.rom_bytes(2)                                 # ready branch
    clock.rom_bytes(1); clock.idle(2)                   # preserve header byte
    clock.rom_bytes(4); clock.ram_read()                # queue read cursor
    clock.rom_bytes(4); clock.ram_read()                # queue write cursor
    clock.rom_bytes(2)                                 # nonempty branch
    clock.rom_bytes(1); clock.idle()                    # select queue slot
    clock.rom_bytes(4); clock.ram_read()                # command parameter
    clock.rom_bytes(1); clock.idle(); clock.ram_write()  # parameter saved
    clock.rom_bytes(1); clock.idle(2)                   # header restored
    clock.rom_bytes(2)                                 # toggle handshake bits
    clock.rom_bytes(4); clock.ram_write()               # expected acknowledgment
    clock.rom_bytes(4); clock.ram_read()                # command number
    clock.rom_bytes(1); clock.idle(); clock.ram_write()  # combined header saved
    clock.rom_bytes(1); clock.idle()                    # read cursor arithmetic
    clock.rom_bytes(1); clock.idle()
    clock.rom_bytes(2)
    clock.rom_bytes(4); clock.ram_write()               # publish next cursor
    clock.rom_bytes(2); clock.idle()                    # 16-bit transfer width
    clock.rom_bytes(1); clock.idle(2); clock.ram_read(2) # restore transfer word
    store_entry = clock.ticks
    clock.rom_bytes(3)
    return store_entry, clock.port_write(), clock.port_write()


@dataclass
class MenuPaletteClockState:
    delay: int = 0
    phase: int = 0


def run_menu_nmi(clock: PalBusClock, palette: MenuPaletteClockState) -> dict[str, int]:
    """Observed NMI-entry boundary through RTI; logo settled at zero.

    Static readings: $00:8587-8598, $80:F622-644, $80:FA60-FAC9,
    $80:B0F0-F9. Palette state is native arithmetic, not a timestamp table.
    The caller still supplies interrupt entry; interrupt recognition is pending.
    """
    markers = {}
    # Preserve interrupted native registers; select zero data/direct-page banks.
    clock.rom_bytes(1); clock.idle(); clock.ram_write()
    clock.rom_bytes(1); clock.idle(); clock.ram_write(2)
    clock.rom_bytes(2); clock.idle()
    for _ in range(3):
        clock.rom_bytes(1); clock.idle(); clock.ram_write(2)
    clock.rom_bytes(2); clock.idle()
    clock.rom_bytes(2)
    clock.rom_bytes(1); clock.idle(); clock.ram_write()
    clock.rom_bytes(1); clock.idle(2); clock.ram_read()
    clock.rom_bytes(3); clock.ram_write(2)
    clock.rom_bytes(1); clock.idle(2); clock.ram_read(2)
    clock.rom_bytes(3); clock.ram_read(3)
    markers['logo'] = clock.ticks
    # Settled downward logo: decrement wraps to FE, so no scroll register write.
    clock.rom_bytes(2); clock.idle()
    clock.rom_bytes(4); clock.ram_read()
    clock.rom_bytes(2)
    clock.rom_bytes(2)
    clock.rom_bytes(2); clock.ram_read()
    for _ in range(2): clock.rom_bytes(1); clock.idle()
    clock.rom_bytes(2); clock.idle()
    clock.rom_bytes(4)
    markers['palette'] = clock.ticks
    clock.rom_bytes(2); clock.idle()
    clock.rom_bytes(2); clock.ram_read(); clock.idle(); clock.ram_write()
    palette.delay -= 1
    clock.rom_bytes(2)
    if palette.delay >= 0:
        clock.idle()
    else:
        palette.delay = 6
        clock.rom_bytes(2)
        clock.rom_bytes(2); clock.ram_write()
        clock.rom_bytes(2); clock.ram_read(); clock.idle(); clock.ram_write()
        palette.phase -= 1
        clock.rom_bytes(2)
        if palette.phase >= 0:
            clock.idle()
        else:
            palette.phase = 3
            clock.rom_bytes(2)
            clock.rom_bytes(2); clock.ram_write()
        # Select the first colour and compute its word index.
        clock.rom_bytes(2)
        clock.rom_bytes(3); clock.port_write()
        clock.rom_bytes(2); clock.idle()
        clock.rom_bytes(2); clock.ram_read(2)
        clock.rom_bytes(3)
        for _ in range(2): clock.rom_bytes(1); clock.idle()
        clock.rom_bytes(2); clock.idle()
        for colour in range(4):
            if colour:
                if colour == 3: clock.rom_bytes(2); clock.idle()
                clock.rom_bytes(2)
                clock.rom_bytes(3); clock.port_write()
                clock.rom_bytes(2); clock.idle()
            clock.rom_bytes(3); clock.idle(); clock.rom_bytes(2)
            clock.rom_bytes(2); clock.idle()
            clock.rom_bytes(3); clock.port_write()
            clock.rom_bytes(1); clock.idle(2)
            clock.rom_bytes(3); clock.port_write()
    markers['palette_leave'] = clock.ticks
    clock.rom_bytes(4)
    markers['restore'] = clock.ticks
    clock.rom_bytes(2); clock.idle()
    for _ in range(4): clock.rom_bytes(1); clock.idle(2); clock.ram_read(2)
    clock.rom_bytes(1); clock.idle(2); clock.ram_read()
    clock.rom_bytes(2); clock.idle()
    markers['rti'] = clock.ticks
    clock.rom_bytes(1); clock.idle(2); clock.ram_read(4)
    markers['resumed'] = clock.ticks
    return markers


@dataclass
class MenuPollInterrupt:
    """Conditional poll interrupt recognition from PAL raster and native state.

    First ordinary NMI is frame 251 (the first enable during frame 250 is a
    separate boot path). Accepted front-end initialization resets palette state
    after frame 377. No observed interrupt timestamp is an input here.
    """
    entry: int
    pending: bool = False
    handled: bool = False

    def __post_init__(self) -> None:
        self.frame = self.entry // 425568
        self.deadline = self.frame * 425568 + 306906
        # The caller is JSL: its last-cycle interrupt test precedes its final
        # eight-clock stack write. An edge in that write can reach this entry
        # before the first operation has performed another interrupt test.
        self.eligible = self.frame >= 250 and self.entry - 8 < self.deadline

    def last_cycle(self, clock: PalBusClock) -> None:
        if self.eligible and not self.handled and clock.ticks >= self.deadline:
            self.pending = True

    def before_operation(self, clock: PalBusClock) -> None:
        if not self.pending: return
        self.pending, self.handled = False, True
        # Native interrupt entry: held PC bus read, idle, saved bank/PC/status,
        # then two vector-byte reads. It runs no original game instructions.
        clock.rom_bytes(1); clock.idle(); clock.ram_write(4); clock.rom_bytes(2)
        frame = self.frame + 1
        before = frame - (378 if frame >= 378 else 250)
        wraps = (before + 6) // 7
        palette = MenuPaletteClockState(6 - (before - 1) % 7 if before else 0,
                                       (-wraps) % 4)
        run_menu_nmi(clock, palette)
