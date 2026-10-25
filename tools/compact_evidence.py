#!/usr/bin/env python3
"""Shrink local evidence on disk without changing a byte any reader sees (D-0013).

Two passes over each root (default `local/evidence` and `artifacts`), files of at least --min-size:
1. Identical files (same size and SHA-256) are stored once: each later copy is replaced by an APFS
   clone (`cp -c`) of the first, so the paths stay separate files that share their blocks.
2. Every other file is given macOS's transparent filesystem compression (decmpfs), by extracting it
   from a `ditto` archive with --hfsCompression. Readers, mmap included, see the original bytes.
   macOS does not compress files above about 500 MB this way; those are reported and left as they are.

Each replacement is written beside the original, checked byte for byte (SHA-256) and swapped in with
an atomic rename, so a reader that has the file open keeps the old copy. Files modified in the last
--skip-recent minutes, hard-linked files, symlinks and already-compressed formats are skipped.
macOS only. usage: python3 tools/compact_evidence.py [ROOT...] [--dry-run] [--jobs N]
"""
import argparse
import hashlib
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

COMPRESSED_FLAG = 0x20  # UF_COMPRESSED, set on a decmpfs-compressed file
ALREADY_PACKED = {'.png', '.gz', '.zst', '.zip', '.xz', '.bz2', '.jpg', '.dylib', '.pack'}


def disk_bytes(path):
    return os.lstat(path).st_blocks * 512


def sha256(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        while chunk := stream.read(1 << 24):
            digest.update(chunk)
    return digest.hexdigest()


def candidates(roots, min_size, recent_seconds):
    now = time.time()
    for root in roots:
        for path in Path(root).rglob('*'):
            try:
                info = path.lstat()
            except FileNotFoundError:
                continue
            if not stat.S_ISREG(info.st_mode) or info.st_size < min_size or info.st_nlink > 1:
                continue
            if now - info.st_mtime < recent_seconds or path.suffix.lower() in ALREADY_PACKED:
                continue
            yield path, info


def replace_checked(original, replacement, expected_digest):
    """Swap in `replacement` if it holds the same bytes as `original`; otherwise discard it."""
    if sha256(replacement) != expected_digest:
        replacement.unlink()
        raise RuntimeError(f'{original}: replacement differs, left unchanged')
    os.replace(replacement, original)


def compress(path):
    """Give one file transparent compression. Returns (bytes before, bytes after, note)."""
    before = disk_bytes(path)
    digest = sha256(path)
    with tempfile.TemporaryDirectory(dir=path.parent, prefix='.compact-') as scratch:
        archive = subprocess.Popen(['ditto', '-c', str(path), '-'], stdout=subprocess.PIPE)
        subprocess.run(['ditto', '-x', '--hfsCompression', '-', scratch], stdin=archive.stdout, check=True)
        archive.stdout.close()
        if archive.wait() != 0:
            raise RuntimeError(f'{path}: ditto -c failed')
        packed = Path(scratch) / path.name
        if not os.lstat(packed).st_flags & COMPRESSED_FLAG:
            return before, before, 'not compressible by the filesystem (size limit)'
        if disk_bytes(packed) >= before * 0.9:
            return before, before, 'saving under 10%'
        replace_checked(path, packed, digest)
    return before, disk_bytes(path), 'compressed'


def clone(source, duplicate):
    """Replace `duplicate` (same bytes as `source`) with an APFS clone of `source`."""
    before = disk_bytes(duplicate)
    digest = sha256(duplicate)
    with tempfile.TemporaryDirectory(dir=duplicate.parent, prefix='.compact-') as scratch:
        copy = Path(scratch) / duplicate.name
        subprocess.run(['cp', '-c', '-p', str(source), str(copy)], check=True)
        replace_checked(duplicate, copy, digest)
    return before


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('roots', nargs='*', default=['local/evidence', 'artifacts'])
    parser.add_argument('--min-size', type=int, default=1 << 20, help='bytes (default 1 MiB)')
    parser.add_argument('--skip-recent', type=float, default=60, help='minutes (default 60)')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        sys.exit('compact_evidence: macOS only (APFS clones and decmpfs compression)')

    files = list(candidates(args.roots, args.min_size, args.skip_recent * 60))
    total_before = sum(disk_bytes(p) for p, _ in files)
    print(f'{len(files)} files, {total_before / 1e9:.2f} GB on disk', flush=True)

    by_size = {}
    for path, info in files:
        by_size.setdefault(info.st_size, []).append(path)
    groups = {}
    with ThreadPoolExecutor(args.jobs) as pool:
        same_size = [p for paths in by_size.values() if len(paths) > 1 for p in paths]
        for path, digest in zip(same_size, pool.map(sha256, same_size)):
            groups.setdefault(digest, []).append(path)
    duplicates = [(paths[0], d) for paths in groups.values() for d in sorted(paths)[1:]]
    print(f'identical copies: {len(duplicates)}', flush=True)

    duplicate_paths = {d for _, d in duplicates}
    to_compress = [p for p, _ in files if p not in duplicate_paths]
    if args.dry_run:
        print(f'would compress {len(to_compress)} files and clone {len(duplicates)}')
        return 0
    free_before = shutil.disk_usage(args.roots[0]).free
    notes, before_sum, after_sum, failures = {}, 0, 0, []

    def one(path):
        try:
            return path, compress(path)
        except Exception as exc:  # report and keep going; the original is untouched
            return path, exc

    # Compress first, so each clone below copies the compressed source.
    with ThreadPoolExecutor(args.jobs) as pool:
        for path, result in pool.map(one, to_compress):
            if isinstance(result, Exception):
                failures.append(f'{path}: {result}')
                continue
            before, after, note = result
            before_sum += before
            after_sum += after
            notes[note] = notes.get(note, 0) + 1
    cloned = 0
    for source, duplicate in duplicates:
        try:
            clone(source, duplicate)
            cloned += 1
        except Exception as exc:
            failures.append(f'{duplicate}: {exc}')
    for note, count in sorted(notes.items()):
        print(f'{note}: {count} files')
    for failure in failures:
        print(f'FAILED {failure}')
    print(f'compression: {before_sum / 1e9:.2f} GB -> {after_sum / 1e9:.2f} GB; clones: {cloned}')
    freed = shutil.disk_usage(args.roots[0]).free - free_before
    print(f'status={"failed" if failures else "passed"} freed={freed / 1e9:.2f}GB '
          f'(the volume\'s free space before and after, so other writers count too)')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
