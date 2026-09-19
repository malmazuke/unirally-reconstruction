"""doctor, bootstrap, build and test subcommands."""

from __future__ import annotations

import argparse
import json
import math
import os
import platform
import re
import shutil
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

from . import (
    EXIT_FAILURE,
    EXIT_INVALID_INPUT,
    EXIT_MISSING_PREREQUISITE,
    EXIT_OK,
    EXIT_TIMEOUT,
)
from . import report as reportmod
from . import judgment
from . import toolchain
from .procs import run_bounded

ROOT = reportmod.repo_root()
DEFAULT_LOCK = ROOT / "tools" / "locks" / "toolchain.json"
MIN_PYTHON = (3, 11)
NESTED_ENV = "UNIRALLY_LAB_NESTED_TEST"


def _status_from_checks(rep: reportmod.Report) -> int:
    outcomes = {c["outcome"] for c in rep.required_failures()}
    if "timeout" in outcomes:
        return EXIT_TIMEOUT
    if "missing" in outcomes:
        return EXIT_MISSING_PREREQUISITE
    if outcomes:
        return EXIT_FAILURE
    return EXIT_OK


def _finish(rep: reportmod.Report, args: argparse.Namespace, status: int) -> int:
    rep.finish("passed" if status == EXIT_OK else "failed")
    rep.write(Path(args.report) if getattr(args, "report", None) else None)
    reportmod.print_summary(rep)
    return status


def _version_line(exe: str | None, *flags: str, timeout: float = 20) -> tuple[str, str | None]:
    """Returns (outcome, first output line)."""
    if not exe:
        return "missing", None
    result = run_bounded([exe, *flags], timeout=timeout)
    if result.outcome != "passed":
        return result.outcome, result.tail(200) or None
    text = (result.stdout or result.stderr).strip()
    return "passed", text.splitlines()[0] if text else ""


def _manifest_path(root: Path, lock: dict[str, Any]) -> Path:
    return root / lock["install_dir"] / "manifest.json"


def _load_toolchain(root: Path, lock_path: Path) -> tuple[dict[str, Any] | None, dict[str, Any] | None, str | None]:
    try:
        lock = toolchain.load_lock(lock_path)
    except (toolchain.ToolchainError, json.JSONDecodeError) as exc:
        return None, None, str(exc)
    return lock, toolchain.load_manifest(_manifest_path(root, lock)), None


# ---------------------------------------------------------------- doctor


def cmd_doctor(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    rep.data["environment"] = {
        "platform_key": toolchain.platform_key(),
        "os": platform.platform(),
        "python_executable": sys.executable,
        "cwd": os.getcwd(),
        "root": str(root),
    }

    py_ok = sys.version_info[:2] >= MIN_PYTHON
    rep.add_check("python_version", "passed" if py_ok else "failed",
                  detail=f"{platform.python_version()} (minimum {MIN_PYTHON[0]}.{MIN_PYTHON[1]})")

    outcome, line = _version_line(shutil.which("git"), "--version")
    rep.add_check("git", outcome, detail=line)

    compiler = None
    for candidate in ("c++", "clang++", "g++"):
        exe = shutil.which(candidate)
        outcome, line = _version_line(exe, "--version")
        if outcome == "passed":
            compiler = (candidate, exe, line)
            break
    rep.add_check("cxx_compiler", "passed" if compiler else "missing",
                  detail=f"{compiler[1]}: {compiler[2]}" if compiler else "no c++, clang++ or g++ on PATH")

    curl = shutil.which("curl")
    outcome, line = _version_line(curl, "--version")
    rep.add_check("downloader", outcome, detail=f"{curl}: {line}" if curl else "curl not found (urllib fallback needs a CA bundle)")

    for name in ("cmake", "ninja"):
        outcome, line = _version_line(shutil.which(name), "--version")
        rep.add_check(f"system_{name}", outcome, required=False, detail=line or "not on PATH; the isolated toolchain is used instead")

    lock, manifest, error = _load_toolchain(root, Path(args.lock))
    if error:
        rep.add_check("toolchain_lock", "failed", detail=error)
    else:
        rep.add_check("toolchain_lock", "passed", detail=str(args.lock))
        if manifest is None:
            rep.add_check("isolated_toolchain", "missing", required=False,
                          detail="run `python3 tools/project.py bootstrap`")
        else:
            for name, entry in manifest["tools"].items():
                outcome, line = _version_line(entry["binaries"][name], "--version")
                rep.add_check(f"isolated_{name}", outcome, required=False, detail=line)
                rep.data["tools"][name] = entry["reported"]

    outcome, line = _version_line(shutil.which("docker"), "--version")
    rep.add_check("docker_cli", outcome, required=False, detail=line or "not installed; not required")

    # Optional: only `judge` commands use it, and it is never required (D-0007).
    key, source = judgment.find_api_key(root)
    rep.add_check("typesafe_api_key", "passed" if key else "missing", required=False,
                  detail=f"from {source}" if key else f"not set; {judgment.KEY_ENV} in the environment or .env enables `judge` (see .env.example)")

    return _finish(rep, args, _status_from_checks(rep))


# ------------------------------------------------------------- bootstrap


def cmd_bootstrap(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    lock_path = Path(args.lock)
    try:
        lock = toolchain.load_lock(lock_path)
    except (toolchain.ToolchainError, json.JSONDecodeError) as exc:
        rep.add_check("toolchain_lock", "failed", detail=str(exc))
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_input("toolchain_lock", lock_path, toolchain.sha256_of(lock_path))

    checks: list[dict[str, Any]] = []
    manifest = toolchain.bootstrap(lock, root, timeout=args.timeout, checks=checks)
    for c in checks:
        rep.add_check(c["name"], c["outcome"], c["required"], detail=c.get("detail"))
    if manifest is not None:
        manifest["lock_sha256"] = rep.data["inputs"]["toolchain_lock"]["sha256"]
        out = _manifest_path(root, lock)
        toolchain.write_manifest(manifest, out)
        rep.add_artifact("toolchain_manifest", out)
        for name, entry in manifest["tools"].items():
            rep.data["tools"][name] = entry["reported"]
    return _finish(rep, args, _status_from_checks(rep))


# ----------------------------------------------------------------- build


def _read_compiler_info(build_dir: Path) -> dict[str, Any]:
    info: dict[str, Any] = {}
    cache = build_dir / "CMakeCache.txt"
    if cache.is_file():
        for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
            m = re.match(r"^(CMAKE_CXX_COMPILER|CMAKE_BUILD_TYPE|CMAKE_GENERATOR|CMAKE_MAKE_PROGRAM|LAB_SANITIZERS|LAB_WARNINGS_AS_ERRORS):[A-Z]+=(.*)$", line)
            if m:
                info[m.group(1)] = m.group(2)
    for comp in build_dir.glob("CMakeFiles/*/CMakeCXXCompiler.cmake"):
        for line in comp.read_text(encoding="utf-8", errors="replace").splitlines():
            m = re.match(r'^set\((CMAKE_CXX_COMPILER_ID|CMAKE_CXX_COMPILER_VERSION|CMAKE_CXX_STANDARD_COMPUTED_DEFAULT) "(.*)"\)$', line)
            if m:
                info[m.group(1)] = m.group(2)
    return info


def _check_preset(rep: reportmod.Report, root: Path, preset: str) -> int | None:
    """Validate a preset name against CMakePresets.json; returns an exit code on failure."""
    presets = root / "CMakePresets.json"
    if not presets.is_file():
        rep.add_check("cmake_presets", "missing", detail=f"{presets} not found")
        return EXIT_MISSING_PREREQUISITE
    try:
        names = {p["name"] for p in json.loads(presets.read_text())["configurePresets"] if not p.get("hidden")}
    except (ValueError, KeyError, TypeError) as exc:
        rep.add_check("cmake_presets", "failed", detail=f"unreadable presets: {exc}")
        return EXIT_INVALID_INPUT
    if preset not in names:
        rep.add_check("cmake_presets", "failed", detail=f"unknown preset {preset!r}; known: {sorted(names)}")
        return EXIT_INVALID_INPUT
    rep.add_input("cmake_presets", presets, toolchain.sha256_of(presets))
    return None


def _write_build_info(rep: reportmod.Report, build_dir: Path, preset: str, manifest: dict[str, Any], outcome: str) -> dict[str, Any]:
    """Record what was built and from which source state; ``outcome`` is the build outcome."""
    info = {
        "preset": preset,
        "build_dir": str(build_dir),
        "outcome": outcome,
        "compiler": _read_compiler_info(build_dir),
        "toolchain": {k: v["reported"] for k, v in manifest["tools"].items()},
        "source": reportmod.source_state(),
    }
    if build_dir.is_dir():
        info_path = build_dir / "lab-build-info.json"
        info_path.write_text(json.dumps(info, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        rep.add_artifact("build_info", info_path)
    rep.data["build"] = info
    return info


def _require_toolchain(rep: reportmod.Report, root: Path, lock_path: Path) -> tuple[dict[str, Any] | None, int | None]:
    lock, manifest, error = _load_toolchain(root, lock_path)
    if error:
        rep.add_check("toolchain_lock", "failed", detail=error)
        return None, EXIT_INVALID_INPUT
    cmake = toolchain.tool_path(manifest, "cmake")
    ninja = toolchain.tool_path(manifest, "ninja")
    if cmake is None or ninja is None:
        rep.add_check("isolated_toolchain", "missing", detail="run `python3 tools/project.py bootstrap` first")
        return None, EXIT_MISSING_PREREQUISITE
    rep.add_check("isolated_toolchain", "passed", detail=f"{cmake}; {ninja}")
    for name, entry in manifest["tools"].items():
        rep.data["tools"][name] = entry["reported"]
    return manifest, None


def cmd_build(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if args.timeout <= 0:
        rep.add_check("arguments", "failed", detail=f"--timeout must be positive, got {args.timeout}")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    manifest, code = _require_toolchain(rep, root, Path(args.lock))
    if code is not None:
        return _finish(rep, args, code)
    code = _check_preset(rep, root, args.preset)
    if code is not None:
        return _finish(rep, args, code)
    cmake = toolchain.tool_path(manifest, "cmake")
    ninja = toolchain.tool_path(manifest, "ninja")

    build_dir = root / "build" / args.preset
    if args.clean and build_dir.exists():
        shutil.rmtree(build_dir)

    defines = []
    for item in args.define or []:
        if "=" not in item:
            rep.add_check("arguments", "failed", detail=f"--define expects NAME=VALUE, got {item!r}")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        defines.append(f"-D{item}")
    configure = run_bounded(
        [cmake, "--preset", args.preset, f"-DCMAKE_MAKE_PROGRAM={ninja}", *defines],
        timeout=args.timeout, cwd=root,
    )
    rep.add_check("configure", configure.outcome, detail=configure.tail(1500) if configure.outcome != "passed" else f"{configure.elapsed:.1f}s")
    if configure.outcome != "passed":
        _write_build_info(rep, build_dir, args.preset, manifest, configure.outcome)
        return _finish(rep, args, _status_from_checks(rep))

    build = run_bounded([cmake, "--build", "--preset", args.preset], timeout=args.timeout, cwd=root)
    rep.add_check("build", build.outcome, detail=build.tail(3000) if build.outcome != "passed" else f"{build.elapsed:.1f}s")
    _write_build_info(rep, build_dir, args.preset, manifest, build.outcome)
    return _finish(rep, args, _status_from_checks(rep))


# ------------------------------------------------------------------ test


def _parse_junit(path: Path) -> list[dict[str, Any]]:
    records = []
    tree = ET.parse(path)
    for case in tree.getroot().iter("testcase"):
        name = case.get("name", "?")
        status = case.get("status", "")
        failure = case.find("failure")
        skipped = case.find("skipped")
        detail = ""
        if failure is not None:
            detail = (failure.get("message") or failure.text or "")[:1500]
        if skipped is not None:
            outcome = "skipped"
        elif status in ("run", "passed") and failure is None:
            outcome = "passed"
        elif "timeout" in status.lower() or "timeout" in detail.lower():
            outcome = "timeout"
        elif status in ("notrun", "disabled"):
            outcome = "skipped"
        else:
            outcome = "failed"
        records.append({"name": name, "outcome": outcome, "status": status,
                        "elapsed": float(case.get("time") or 0.0), "detail": detail})
    return records


def cmd_test(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if args.suite != "synthetic":
        rep.add_check("suite", "failed", detail=f"unknown suite {args.suite!r}; only 'synthetic' exists")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if args.timeout <= 0 or args.test_timeout <= 0:
        rep.add_check("arguments", "failed", detail="--timeout and --test-timeout must be positive")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    code = _check_preset(rep, root, args.preset)
    if code is not None:
        return _finish(rep, args, code)
    # Absolute: ctest --test-dir resolves relative output paths against the build tree.
    artifacts = (Path(args.artifacts) if args.artifacts else root / "artifacts" / rep.data["run_id"]).resolve()
    artifacts.mkdir(parents=True, exist_ok=True)

    # 1. Python tooling tests. A tooling test may itself invoke this command
    # for the native part only; a nested run with Python tests enabled would
    # recurse without bound, so refuse it.
    nested = os.environ.get(NESTED_ENV) == "1"
    if args.python_tests and nested:
        rep.add_check("python_tooling_tests", "failed",
                      detail=f"recursive invocation: {NESTED_ENV} is set; pass --no-python-tests in nested runs")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if not args.python_tests:
        rep.add_check("python_tooling_tests", "skipped", required=False, detail="disabled by --no-python-tests")
        py = None
    else:
        py = run_bounded(
            [sys.executable, str(root / "tools" / "unirally_lab" / "pytests.py"), str(root / "tests" / "tooling")],
            timeout=args.timeout, cwd=root, env={**os.environ, NESTED_ENV: "1"},
        )
    if py is None:
        pass
    elif py.outcome == "timeout" or py.missing:
        rep.add_check("python_tooling_tests", py.outcome, detail=py.tail(500))
    else:
        try:
            summary = json.loads(py.stdout.strip().splitlines()[-1])
            records = summary["records"]
            tests_run = summary["tests_run"]
        except (ValueError, IndexError, KeyError, TypeError):
            rep.add_check("python_tooling_tests", "failed", detail=f"unparseable runner output: {py.tail(800)}")
            records, tests_run = [], 0
        else:
            for r in records:
                rep.add_check(f"py:{r['name']}", r["outcome"], detail=r.get("detail") or None, elapsed=r.get("elapsed"))
            # The runner's own verdict is authoritative: a test that unittest
            # counted as failed must fail this check even if no record says so
            # (M0-04 review 1, finding M2: subtest failures were dropped).
            failed = sum(1 for r in records if r["outcome"] == "failed")
            ok = py.returncode == 0 and records and failed == 0
            rep.add_check("python_tooling_tests", "passed" if ok else "failed",
                          detail=f"{tests_run} tests run, {len(records)} records, {failed} failed; runner exit {py.returncode}"
                                 + ("" if records else "; no tests discovered"))

    # 2. Native synthetic tests via ctest. The build is first brought up to
    # date incrementally so that every native outcome below belongs to the
    # source state recorded in this report, not to whatever was built last.
    lock, manifest, error = _load_toolchain(root, Path(args.lock))
    cmake = toolchain.tool_path(manifest, "cmake") if not error else None
    ctest = toolchain.tool_path(manifest, "cmake", "ctest") if not error else None
    build_dir = root / "build" / args.preset
    native_ready = False
    if error:
        rep.add_check("toolchain_lock", "failed", detail=error)
    elif ctest is None or cmake is None:
        rep.add_check("native_build_available", "missing", detail="isolated toolchain absent; run bootstrap and build")
    elif not (build_dir / "CTestTestfile.cmake").is_file():
        rep.add_check("native_build_available", "missing", detail=f"no build at {build_dir}; run `build --preset {args.preset}`")
    else:
        for name, entry in manifest["tools"].items():
            rep.data["tools"][name] = entry["reported"]
        # Re-run configure so preset changes (cache variables) take effect, then build.
        ninja = toolchain.tool_path(manifest, "ninja")
        reconfigure = run_bounded([cmake, "--preset", args.preset, f"-DCMAKE_MAKE_PROGRAM={ninja}"], timeout=args.timeout, cwd=root)
        rebuild = reconfigure
        if reconfigure.outcome == "passed":
            rebuild = run_bounded([cmake, "--build", "--preset", args.preset], timeout=args.timeout, cwd=root)
        info = _write_build_info(rep, build_dir, args.preset, manifest, rebuild.outcome)
        if rebuild.outcome == "passed":
            rep.add_check("native_build_available", "passed",
                          detail=f"{build_dir}; reconfigure {reconfigure.elapsed:.1f}s, incremental build {rebuild.elapsed:.1f}s "
                                 f"at {info['source']['commit']}{' (dirty)' if info['source']['dirty'] else ''}")
            native_ready = True
        else:
            stage = "configure" if reconfigure.outcome != "passed" else "incremental build"
            rep.add_check("native_build_available", rebuild.outcome, detail=f"{stage}: {rebuild.tail(2000)}")
    if native_ready:
        junit = artifacts / f"ctest-{args.preset}.xml"
        ct = run_bounded(
            [ctest, "--test-dir", str(build_dir), "--output-on-failure", "--timeout", str(math.ceil(args.test_timeout)),
             "--output-junit", str(junit)],
            timeout=args.timeout, cwd=root,
        )
        if ct.outcome == "timeout":
            rep.add_check("ctest", "timeout", detail=f"ctest exceeded {args.timeout}s")
        elif junit.is_file():
            records = _parse_junit(junit)
            rep.add_artifact("ctest_junit", junit)
            for r in records:
                rep.add_check(f"ctest:{r['name']}", r["outcome"], detail=r.get("detail") or None, elapsed=r.get("elapsed"))
            if not records:
                rep.add_check("ctest", "failed", detail="no tests were listed")
        else:
            rep.add_check("ctest", ct.outcome, detail=ct.tail(1500))

        # 3. Fresh-process repeatability of the probe.
        runner = build_dir / "src" / "lab" / "lab_runner"
        if not runner.is_file():
            rep.add_check("native_fresh_process_repeatability", "missing", detail=f"{runner} not built")
        else:
            hashes = []
            outcome = "passed"
            for _ in range(3):
                r = run_bounded([runner, "--steps", "1000"], timeout=args.test_timeout, cwd=root)
                if r.outcome != "passed":
                    outcome = r.outcome
                    break
                try:
                    hashes.append(json.loads(r.stdout.strip())["hash"])
                except (ValueError, KeyError):
                    outcome = "failed"
                    break
            if outcome == "passed" and len(set(hashes)) != 1:
                outcome = "failed"
            rep.add_check("native_fresh_process_repeatability", outcome,
                          detail=f"3 fresh processes, hashes {sorted(set(hashes))}")
            rep.data["probe_hash_1000_steps"] = hashes[0] if hashes else None

    return _finish(rep, args, _status_from_checks(rep))


# ------------------------------------------------------------- argparse


def _common(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--report", help="write the JSON run report here")
    parser.add_argument("--task", help="task ID to record in the report")
    parser.add_argument("--root", default=str(ROOT), help=argparse.SUPPRESS)
    parser.add_argument("--lock", default=str(DEFAULT_LOCK), help="toolchain lock file")


def register(sub: argparse._SubParsersAction) -> None:
    doctor = sub.add_parser("doctor", help="report platform, tool versions and missing capabilities")
    _common(doctor)
    doctor.set_defaults(func=cmd_doctor)

    boot = sub.add_parser("bootstrap", help="fetch and verify the pinned, isolated CMake and Ninja")
    _common(boot)
    boot.add_argument("--timeout", type=float, default=600, help="seconds per download")
    boot.set_defaults(func=cmd_bootstrap)

    build = sub.add_parser("build", help="configure and build a CMake preset with the isolated toolchain")
    _common(build)
    build.add_argument("--preset", required=True)
    build.add_argument("--clean", action="store_true", help="remove the preset's build directory first")
    build.add_argument("--define", action="append", metavar="NAME=VALUE",
                       help="extra CMake cache entry for configure (testing hooks; not for acceptance runs)")
    build.add_argument("--timeout", type=float, default=900, help="seconds for configure and again for build")
    build.set_defaults(func=cmd_build)

    test = sub.add_parser("test", help="run a test suite and report per-check outcomes")
    _common(test)
    test.add_argument("--suite", required=True, help="only 'synthetic' exists")
    test.add_argument("--preset", default="lab-debug", help="built preset to test")
    test.add_argument("--artifacts", help="directory for junit and other outputs (default artifacts/<run-id>)")
    test.add_argument("--timeout", type=float, default=600, help="seconds for each test runner process")
    test.add_argument("--test-timeout", type=float, default=60,
                      help="seconds per lab_runner repeatability run, and ctest's default for tests without a TIMEOUT property "
                           "(every test in tests/synthetic sets its own TIMEOUT)")
    test.add_argument("--no-python-tests", dest="python_tests", action="store_false",
                      help="run only the native part (recorded as a skipped optional check)")
    test.set_defaults(func=cmd_test)
