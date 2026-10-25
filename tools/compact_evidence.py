#!/usr/bin/env python3
"""Shrink local evidence on disk without changing a byte any reader sees (D-0013).

Two passes over each root (default `local/evidence` and `artifacts` of this checkout), over files of
at least --min-size:
1. Compression. Each file not already compressed gets macOS's transparent filesystem compression
   (decmpfs), by extracting it from a `ditto` archive with --hfsCompression. Readers, mmap
   included, see the original bytes. macOS does not compress files above about 500 MB this way, so
   files over --max-compress are left as they are without trying.
2. Identical files. Files with the same size and SHA-256 are stored once: each later copy (in
   sorted path order) becomes an APFS clone (`cp -c`) of the first, keeping its own mode, times and
   extended attributes. A copy whose mode or extended attributes differ from the first's is left
   as it is.

A process that keeps a file open for writing across the swap writes to the old copy, so the tool
skips files modified in the last --skip-recent minutes; evidence is written once and then only read.
A copy left uncloned because its mode or attributes differ is not compressed either. Each run
re-reads and re-clones identical copies (cheap, but not free) and compresses only what is new.

3. Large captures. A file over --max-compress whose name is a capture kind the readers open as raw
   or NAME.zst (tools/unirally_lab/capture_files.py: memory.wram, memory.sram, series.bin,
   wram-series.bin, sram-series.bin) is stored as NAME.zst (zstd -10) instead, checked by
   decompressing it again; identical .zst files are then cloned (EVIDENCE-LARGE-FILES).

Every replacement is written to a scratch directory `.compact-scratch` at the root (same volume, so
the final rename is atomic), checked byte for byte (SHA-256), and swapped in only if the original
still has the size, mtime and inode it had when it was read. A reader with the file open keeps the
old copy. A scratch directory left by an interrupted run is removed at the start. Files modified in
the last --skip-recent minutes, hard-linked files, symlinks and already-packed formats are skipped,
and each compression needs --min-free GB free besides the file's own size.
macOS (APFS) only. Run it through local/locks/heavy-run.sh.
usage: python3 tools/compact_evidence.py [ROOT...] [--dry-run] [--jobs N]
"""
import argparse
import hashlib
import os
import plistlib
import shutil
import stat
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

CHECKOUT = Path(__file__).resolve().parents[1]
COMPRESSED_FLAG = 0x20  # UF_COMPRESSED, set on a decmpfs-compressed file
ALREADY_PACKED = {'.png', '.gz', '.zst', '.zip', '.xz', '.bz2', '.jpg', '.dylib', '.pack'}
SCRATCH = '.compact-scratch'
# Attributes the filesystem keeps for its own bookkeeping, not part of a file's identity.
CAPTURE_KINDS = {'memory.wram', 'memory.sram', 'series.bin', 'wram-series.bin', 'sram-series.bin'}
SYSTEM_XATTRS = {'com.apple.decmpfs', 'com.apple.ResourceFork', 'com.apple.provenance'}


def disk_bytes(path):
    return os.lstat(path).st_blocks * 512


def sha256(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        while chunk := stream.read(1 << 24):
            digest.update(chunk)
    return digest.hexdigest()


def identity(info):
    return info.st_size, info.st_mtime_ns, info.st_ino


def xattrs(path):
    """Extended attributes by name (names may hold spaces); a value that cannot be read is unique."""
    names = subprocess.run(['xattr', str(path)], capture_output=True, text=True).stdout.splitlines()
    values = {}
    for name in names:
        if name in SYSTEM_XATTRS:
            continue
        read = subprocess.run(['xattr', '-px', name, str(path)], capture_output=True, text=True)
        values[name] = read.stdout if read.returncode == 0 else object()
    return values


def is_apfs(path):
    fields = subprocess.run(['df', '-P', str(path)], capture_output=True, text=True).stdout.splitlines()[-1].split()
    mount = ' '.join(fields[5:])  # the mount point is the sixth field and may hold spaces
    info = plistlib.loads(subprocess.run(['diskutil', 'info', '-plist', mount], capture_output=True).stdout)
    return info.get('FilesystemType') == 'apfs'


def candidates(roots, min_size, recent_seconds):
    now = time.time()
    for root in roots:
        for path in Path(root).rglob('*'):
            if SCRATCH in path.parts:
                continue
            try:
                info = path.lstat()
            except FileNotFoundError:
                continue
            if not stat.S_ISREG(info.st_mode) or info.st_size < min_size or info.st_nlink > 1:
                continue
            if now - info.st_mtime < recent_seconds or path.suffix.lower() in ALREADY_PACKED:
                continue
            yield path, info


class Compactor:
    def __init__(self, root, min_free):
        self.scratch = Path(root) / SCRATCH
        self.min_free = min_free

    def swap(self, original, replacement, expected_digest, expected_identity):
        """Rename `replacement` over `original` if it holds the same bytes and the original is unchanged."""
        if sha256(replacement) != expected_digest:
            raise RuntimeError('replacement differs from the original')
        if identity(original.lstat()) != expected_identity:
            raise RuntimeError('the original changed while it was being compacted')
        os.replace(replacement, original)

    def compress(self, path, info):
        """Returns (bytes before, bytes after, note)."""
        before = disk_bytes(path)
        if shutil.disk_usage(path.parent).free < self.min_free + info.st_size:
            return before, before, 'skipped: too little free disk'
        digest = sha256(path)
        work = self.scratch / f'{os.getpid()}-{info.st_ino}'
        work.mkdir(parents=True)
        try:
            archive = subprocess.Popen(['ditto', '-c', str(path), '-'], stdout=subprocess.PIPE)
            try:
                extract = subprocess.run(['ditto', '-x', '--hfsCompression', '-', str(work)], stdin=archive.stdout)
            finally:
                archive.stdout.close()
                archived = archive.wait()
            if archived != 0 or extract.returncode != 0:
                raise RuntimeError('ditto failed')
            packed = work / path.name
            if not os.lstat(packed).st_flags & COMPRESSED_FLAG:
                return before, before, 'not compressible by the filesystem'
            if disk_bytes(packed) >= before * 0.9:
                return before, before, 'saving under 10%'
            self.swap(path, packed, digest, identity(info))
        finally:
            shutil.rmtree(work, ignore_errors=True)
        return before, disk_bytes(path), 'compressed'

    def pack_zst(self, path, info):
        """Store a large capture as NAME.zst. Returns (bytes before, bytes after, note)."""
        before = disk_bytes(path)
        if shutil.disk_usage(path.parent).free < self.min_free + info.st_size:
            return before, before, 'skipped: too little free disk'
        digest = sha256(path)
        work = self.scratch / f'{os.getpid()}-{info.st_ino}'
        work.mkdir(parents=True)
        try:
            packed = work / (path.name + '.zst')
            subprocess.run(['zstd', '-q', '-T0', '-10', str(path), '-o', str(packed)], check=True)
            unpacked = hashlib.sha256()
            reader = subprocess.Popen(['zstd', '-dcq', str(packed)], stdout=subprocess.PIPE)
            try:
                while chunk := reader.stdout.read(1 << 24):
                    unpacked.update(chunk)
            finally:
                reader.stdout.close()
                status = reader.wait()
            if status != 0 or unpacked.hexdigest() != digest:
                raise RuntimeError('the .zst does not decompress to the original')
            if identity(path.lstat()) != identity(info):
                raise RuntimeError('the original changed while it was being compacted')
            target = path.with_name(path.name + '.zst')
            os.replace(packed, target)
            path.unlink()
        finally:
            shutil.rmtree(work, ignore_errors=True)
        return before, disk_bytes(target), 'stored as .zst'

    def clone(self, source, duplicate):
        """Replace `duplicate` (same bytes as `source`) with a clone of `source` that keeps its own metadata."""
        info = duplicate.lstat()
        if stat.S_IMODE(info.st_mode) != stat.S_IMODE(source.lstat().st_mode) or xattrs(duplicate) != xattrs(source):
            return 'different mode or attributes'
        digest = sha256(duplicate)
        work = self.scratch / f'{os.getpid()}-{info.st_ino}'
        work.mkdir(parents=True)
        try:
            copy = work / duplicate.name
            subprocess.run(['cp', '-c', str(source), str(copy)], check=True)
            os.chmod(copy, stat.S_IMODE(info.st_mode))
            os.utime(copy, ns=(info.st_atime_ns, info.st_mtime_ns))
            self.swap(duplicate, copy, digest, identity(info))
        finally:
            shutil.rmtree(work, ignore_errors=True)
        return 'cloned'


def run_pass(items, work, jobs, failures):
    def one(item):
        try:
            return item, work(*item)
        except Exception as exc:  # report and keep going; the original is untouched
            return item, exc
    with ThreadPoolExecutor(jobs) as pool:
        for item, result in pool.map(one, items):
            if isinstance(result, Exception):
                failures.append(f'{item[0]}: {result}')
            else:
                yield result


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('roots', nargs='*', default=[str(CHECKOUT / 'local/evidence'), str(CHECKOUT / 'artifacts')])
    parser.add_argument('--min-size', type=int, default=1 << 20, help='bytes (default 1 MiB)')
    parser.add_argument('--max-compress', type=int, default=480 << 20, help='bytes (default 480 MiB)')
    parser.add_argument('--skip-recent', type=float, default=60, help='minutes (default 60)')
    parser.add_argument('--min-free', type=float, default=15, help='GB to leave free (default 15)')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        sys.exit('compact_evidence: macOS only (APFS clones and decmpfs compression)')
    roots = [Path(r) for r in args.roots if Path(r).is_dir()]
    if not roots:
        sys.exit(f'compact_evidence: no such directory: {" ".join(args.roots)}')
    for root in roots:
        if not is_apfs(root):
            sys.exit(f'compact_evidence: {root} is not on APFS; clones would be full copies')
        stale = root / SCRATCH
        if stale.exists() and not args.dry_run:
            print(f'removing {stale} left by an interrupted run')
            shutil.rmtree(stale)

    files = list(candidates(roots, args.min_size, args.skip_recent * 60))
    large = [(p, i) for p, i in files if p.name in CAPTURE_KINDS and i.st_size > args.max_compress]
    if large and not shutil.which('zstd'):
        sys.exit('compact_evidence: large captures need the zstd command')
    files = [(p, i) for p, i in files if (p, i) not in large]
    print(f'{len(files)} files, {sum(disk_bytes(p) for p, _ in files) / 1e9:.2f} GB on disk', flush=True)

    # Identical files first, so only the first of each group is compressed and the rest clone it.
    by_size = {}
    for path, info in files:
        by_size.setdefault(info.st_size, []).append(path)
    same_size = sorted(p for paths in by_size.values() if len(paths) > 1 for p in paths)
    groups = {}
    with ThreadPoolExecutor(args.jobs) as pool:
        for path, digest in zip(same_size, pool.map(sha256, same_size)):
            groups.setdefault(digest, []).append(path)
    pairs = [(paths[0], d) for paths in groups.values() for d in paths[1:]]  # paths are in sorted order
    later_copies = {d for _, d in pairs}
    to_compress = [(p, i) for p, i in files if p not in later_copies
                   and not i.st_flags & COMPRESSED_FLAG and i.st_size <= args.max_compress]
    print(f'identical copies: {len(pairs)}; to compress: {len(to_compress)} (the rest are already '
          f'compressed, over {args.max_compress >> 20} MiB or later copies); large captures to .zst: '
          f'{len(large)}', flush=True)
    if args.dry_run:
        return 0

    root_of = lambda p: next(r for r in roots if r in p.parents)
    compactors = {r: Compactor(r, args.min_free * 1e9) for r in roots}
    free_before = shutil.disk_usage(roots[0]).free
    failures, notes, saved = [], {}, 0
    for before, after, note in run_pass(to_compress, lambda p, i: compactors[root_of(p)].compress(p, i),
                                         args.jobs, failures):
        notes[note] = notes.get(note, 0) + 1
        saved += before - after
    for note in run_pass(pairs, lambda s, d: compactors[root_of(d)].clone(s, d), args.jobs, failures):
        notes[note] = notes.get(note, 0) + 1
    packed = []
    for path, info in large:  # one at a time: each uses every core
        try:
            before, after, note = compactors[root_of(path)].pack_zst(path, info)
        except Exception as exc:
            failures.append(f'{path}: {exc}')
            continue
        notes[note] = notes.get(note, 0) + 1
        saved += before - after
        if note == 'stored as .zst':
            packed.append(path.with_name(path.name + '.zst'))
    zst_groups = {}
    for path in sorted(packed):
        zst_groups.setdefault((path.stat().st_size, sha256(path)), []).append(path)
    zst_pairs = [(paths[0], d) for paths in zst_groups.values() for d in paths[1:]]
    for note in run_pass(zst_pairs, lambda s, d: compactors[root_of(d)].clone(s, d), args.jobs, failures):
        notes[note] = notes.get(note, 0) + 1

    for root in roots:
        shutil.rmtree(root / SCRATCH, ignore_errors=True)
    for note, count in sorted(notes.items()):
        print(f'{note}: {count} files')
    for failure in failures:
        print(f'FAILED {failure}')
    freed = shutil.disk_usage(roots[0]).free - free_before
    print(f'compression saved {saved / 1e9:.2f} GB')
    short = notes.get('skipped: too little free disk', 0)
    status = 'failed' if failures else 'incomplete' if short else 'passed'
    print(f'status={status} freed={freed / 1e9:.2f}GB '
          f'(the volume\'s free space before and after, so other writers count too)')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
