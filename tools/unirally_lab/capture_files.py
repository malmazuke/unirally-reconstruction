"""Open a stored capture whether it is kept raw (`NAME`) or zstd-compressed (`NAME.zst`).

Captures larger than macOS's transparent compression accepts (about 500 MB) are stored as
`NAME.zst` (EVIDENCE-LARGE-FILES, D-0013). Readers open them through this module, which returns the
same bytes either way. The raw file wins when both exist. Decompression uses the standard library's
`compression.zstd` (Python 3.14) or, failing that, the `zstd` command.
"""
import io
import shutil
import subprocess
from pathlib import Path

SUFFIX = '.zst'


def stored_path(path):
    """The file that holds `path`'s bytes: the raw file, else its `.zst`. Raises FileNotFoundError."""
    path = Path(path)
    if path.exists():
        return path
    packed = path.with_name(path.name + SUFFIX)
    if packed.exists():
        return packed
    raise FileNotFoundError(f'{path} (nor {packed.name})')


def open_capture(path):
    """A binary stream of the capture's bytes."""
    stored = stored_path(path)
    if stored.suffix != SUFFIX:
        return stored.open('rb')
    try:
        from compression import zstd
    except ImportError:
        if not shutil.which('zstd'):
            raise RuntimeError(f'{stored} needs Python 3.14 or the zstd command')
        return io.BytesIO(subprocess.run(['zstd', '-dcq', str(stored)], check=True, capture_output=True).stdout)
    return zstd.open(stored, 'rb')


def read_capture(path):
    """The capture's bytes."""
    with open_capture(path) as stream:
        return stream.read()
