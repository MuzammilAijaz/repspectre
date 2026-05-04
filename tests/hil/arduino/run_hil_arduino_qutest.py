#!/usr/bin/env python3

""" Arduino HIL runner for QUTest/QSPY.

This script enables a tight TDD loop for integration tests on real hardware:
1) Stage a temporary Arduino sketch that includes the QUTest fixture + QP/C sources.
2) Build + upload the sketch using arduino-cli.
3) Start QSPY connected to the target over serial.
4) Run QUTest scripts against the running firmware.

Requirements (installed on your host):
- arduino-cli
- QTools: qspy + qutest.py
- A connected board + known FQBN + serial port

The QP/C sources are expected to exist locally. By default, the runner looks for
them in the main build directory (../build/_deps/qpc-src). If you haven't run
the repo's CMake build yet, set --qpc explicitly.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Optional

port_dir = Path(__file__).resolve().parent / "ports" / "arduino-qutest"

_STAMP_VERSION = 1


def _resolve_exe(path_or_dir: str, default_name: str | None = None) -> str:
    p = Path(path_or_dir)
    if p.is_dir():
        if default_name is None:
            raise ValueError(f"Expected a file, got directory: {path_or_dir}")
        p = p / default_name
    if not p.exists():
        raise FileNotFoundError(f"Not found: {p}")
    if not os.access(str(p), os.X_OK):
        raise PermissionError(f"Not executable: {p}")
    return str(p)


def _terminate_process(process: subprocess.Popen, timeout_s: float) -> None:
    if process.poll() is not None:
        return
    try:
        process.terminate()
    except Exception:
        pass

    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if process.poll() is not None:
            return
        time.sleep(0.05)

    try:
        process.kill()
    except Exception:
        pass


def _release_serial_reset_lines(port: str) -> None:
    if os.name != "posix" or not port.startswith("/dev/"):
        return

    try:
        import array
        import fcntl
        import termios

        fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        try:
            mask = array.array("i", [termios.TIOCM_DTR | termios.TIOCM_RTS])
            fcntl.ioctl(fd, termios.TIOCMBIC, mask, True)
        finally:
            os.close(fd)
    except Exception:
        # Some USB CDC drivers do not expose modem control ioctls. In that
        # case QSPY/target communication can still work without this helper.
        return


def _repo_root_from_this_file() -> Path:
    # tests/hil/arduino/run_hil_arduino_qutest.py -> tests -> repo root
    return Path(__file__).resolve().parents[2].parent


def _default_qpc_dir() -> Path:
    return _repo_root_from_this_file() / "build" / "_deps" / "qpc-src"


def _default_work_dir() -> Optional[Path]:
    # Prefer a stable location under $HOME (some installs of arduino-cli are
    # sandboxed and might not be able to see /tmp).
    env = os.environ.get("REPSPECTRE_HIL_WORKDIR", "").strip()
    if env:
        return Path(env).expanduser()

    xdg_cache = os.environ.get("XDG_CACHE_HOME", "").strip()
    if xdg_cache:
        return Path(xdg_cache) / "repspectre" / "hil"

    home = os.environ.get("HOME", "").strip()
    if home:
        return Path(home) / ".cache" / "repspectre" / "hil"

    return None


QPC_SOURCES = [
    # QEP/QF
    "src/qf/qep_hsm.c",
    "src/qf/qep_msm.c",
    "src/qf/qf_act.c",
    "src/qf/qf_actq.c",
    "src/qf/qf_defer.c",
    "src/qf/qf_dyn.c",
    "src/qf/qf_mem.c",
    "src/qf/qf_ps.c",
    "src/qf/qf_qact.c",
    "src/qf/qf_qeq.c",
    "src/qf/qf_qmact.c",
    "src/qf/qf_time.c",
    # QS + QUTest
    "src/qs/qs.c",
    "src/qs/qs_64bit.c",
    "src/qs/qs_rx.c",
    "src/qs/qs_fp.c",
    "src/qs/qutest.c",
]


def _workspace_key(fqbn: str, sketch_dir: Path, qpc_dir: Path) -> str:
    # Deterministic and short; unique per sketch/QP/C/FQBN.
    h = hashlib.sha256()
    h.update(fqbn.encode("utf-8"))
    h.update(b"\n")
    h.update(str(sketch_dir).encode("utf-8"))
    h.update(b"\n")
    h.update(str(qpc_dir).encode("utf-8"))
    return h.hexdigest()[:12]


def _should_skip_name(name: str) -> bool:
    return name.startswith(".")


def _safe_unlink(path: Path) -> None:
    try:
        path.unlink()
    except FileNotFoundError:
        return


def _rm_tree(path: Path) -> None:
    if not path.exists() and not path.is_symlink():
        return
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path, ignore_errors=True)
    else:
        _safe_unlink(path)


def _link_or_copy_file(src: Path, dst: Path, mode: str) -> None:
    if dst.exists() or dst.is_symlink():
        if mode == "symlink" and dst.is_symlink():
            try:
                if dst.resolve() == src.resolve():
                    return
            except FileNotFoundError:
                pass
        elif mode == "copy":
            # Avoid rewriting unchanged files (mtime churn defeats incremental builds).
            try:
                src_stat = src.stat()
                dst_stat = dst.stat()
                if (src_stat.st_size == dst_stat.st_size) and (
                    src_stat.st_mtime_ns == dst_stat.st_mtime_ns
                ):
                    return
            except FileNotFoundError:
                pass
        _rm_tree(dst)

    dst.parent.mkdir(parents=True, exist_ok=True)

    if mode == "symlink":
        os.symlink(src, dst)
    else:
        shutil.copy2(src, dst)


def _sync_copy_dir(src_dir: Path, dst_dir: Path) -> None:
    dst_dir.mkdir(parents=True, exist_ok=True)
    desired: set[str] = set()

    for item in src_dir.iterdir():
        if _should_skip_name(item.name):
            continue
        desired.add(item.name)
        dst = dst_dir / item.name
        if item.is_dir():
            if dst.exists() and dst.is_dir() and not dst.is_symlink():
                _sync_copy_dir(item, dst)
            else:
                _rm_tree(dst)
                shutil.copytree(item, dst)
        else:
            _link_or_copy_file(item, dst, mode="copy")

    for item in dst_dir.iterdir():
        if item.name in desired:
            continue
        _rm_tree(item)


def _iter_nonhidden_files(root: Path) -> list[Path]:
    files: list[Path] = []
    if not root.exists():
        return files

    for p in root.rglob("*"):
        if not p.is_file():
            continue
        try:
            rel = p.relative_to(root)
        except Exception:
            rel = p
        if any(part.startswith(".") for part in rel.parts):
            continue
        files.append(p)

    files.sort()
    return files


_FIRMWARE_EXTS = {
    ".ino",
    ".pde",
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".s",
    ".S",
}


def _iter_firmware_files(sketch_dir: Path) -> list[Path]:
    """
    Return sketch-tree files that can affect the Arduino build.
    This intentionally excludes host-side test scripts like *_test.py
    when they live alongside the sketch sources.
    """
    files: list[Path] = []
    for p in _iter_nonhidden_files(sketch_dir):
        if p.suffix in _FIRMWARE_EXTS:
            files.append(p)
            continue
        if p.suffix == ".py":
            continue
    files.sort()
    return files


def _compute_inputs_digest(
    fqbn: str,
    sketch_dir: Path,
    qpc_dir: Path,
    port_dir: Path,
    extra_defines: str,
    stage_mode: str,
    extra_includes: list[str],
) -> str:
    """
    Cheap-ish fingerprint of build inputs (uses mtime+size, not file contents).
    """
    h = hashlib.sha256()
    h.update(f"v={_STAMP_VERSION}\n".encode("utf-8"))
    h.update(f"fqbn={fqbn}\n".encode("utf-8"))
    h.update(f"sketch={sketch_dir}\n".encode("utf-8"))
    h.update(f"qpc={qpc_dir}\n".encode("utf-8"))
    h.update(f"port={port_dir}\n".encode("utf-8"))
    h.update(f"extra_defines={extra_defines}\n".encode("utf-8"))
    h.update(f"stage_mode={stage_mode}\n".encode("utf-8"))

    for p in _iter_firmware_files(sketch_dir):
        st = p.stat()
        h.update(str(p).encode("utf-8"))
        h.update(b"\0")
        h.update(str(st.st_size).encode("utf-8"))
        h.update(b"\0")
        h.update(str(st.st_mtime_ns).encode("utf-8"))
        h.update(b"\n")

    for rel in QPC_SOURCES:
        p = qpc_dir / rel
        if p.exists():
            st = p.stat()
            h.update(str(p).encode("utf-8"))
            h.update(b"\0")
            h.update(str(st.st_size).encode("utf-8"))
            h.update(b"\0")
            h.update(str(st.st_mtime_ns).encode("utf-8"))
            h.update(b"\n")

    for p in _iter_nonhidden_files(port_dir):
        st = p.stat()
        h.update(str(p).encode("utf-8"))
        h.update(b"\0")
        h.update(str(st.st_size).encode("utf-8"))
        h.update(b"\0")
        h.update(str(st.st_mtime_ns).encode("utf-8"))
        h.update(b"\n")

    for p in _iter_nonhidden_files(qpc_dir / "include"):
        st = p.stat()
        h.update(str(p).encode("utf-8"))
        h.update(b"\0")
        h.update(str(st.st_size).encode("utf-8"))
        h.update(b"\0")
        h.update(str(st.st_mtime_ns).encode("utf-8"))
        h.update(b"\n")

    for inc_dir in extra_includes:
        p_dir = Path(inc_dir)
        if p_dir.exists() and p_dir.is_dir():
            for p in _iter_nonhidden_files(p_dir):
                if p.suffix in {".h", ".hpp", ".hh", ".hxx"}:
                    st = p.stat()
                    h.update(str(p).encode("utf-8"))
                    h.update(b"\0")
                    h.update(str(st.st_size).encode("utf-8"))
                    h.update(b"\0")
                    h.update(str(st.st_mtime_ns).encode("utf-8"))
                    h.update(b"\n")

    return h.hexdigest()


def _read_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return {}
    except json.JSONDecodeError:
        return {}


def _write_json(path: Path, obj: dict) -> None:
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _output_has_artifacts(output_dir: Path) -> bool:
    if not output_dir.exists():
        return False
    for p in output_dir.iterdir():
        if p.is_file():
            return True
    return False

def _stage_sketch(
    sketch_dir: Path,
    qpc_dir: Path,
    port_dir: Path,
    out_root: Path,
    stage_mode: str,
    extra_libs: list[str],
    extra_includes: list[str],
) -> Path:
    if not sketch_dir.exists():
        raise FileNotFoundError(f"Sketch dir not found: {sketch_dir}")
    if not (sketch_dir / f"{sketch_dir.name}.ino").exists():
        raise FileNotFoundError(
            f"Sketch must contain {sketch_dir.name}.ino inside {sketch_dir}"
        )
    if not qpc_dir.exists():
        raise FileNotFoundError(
            f"QP/C source tree not found: {qpc_dir} (set --qpc or build the repo first)"
        )

    staged_sketch_dir = out_root / sketch_dir.name
    staged_sketch_dir.mkdir(parents=True, exist_ok=True)

    desired_names: set[str] = set()

    # Stage sketch sources.
    for item in sketch_dir.iterdir():
        if _should_skip_name(item.name):
            continue

        # Host-side QUTest scripts often live next to the sketch. Do not stage
        # them into the Arduino workspace (and do not let them trigger rebuilds).
        if item.is_file() and item.suffix == ".py":
            continue

        desired_names.add(item.name)
        dst = staged_sketch_dir / item.name

        if item.is_dir():
            if dst.exists() or dst.is_symlink():
                if stage_mode == "symlink" and dst.is_symlink():
                    try:
                        if dst.resolve() == item.resolve():
                            continue
                    except FileNotFoundError:
                        pass
                if stage_mode == "symlink":
                    _rm_tree(dst)
                elif stage_mode == "copy" and not (
                    dst.exists() and dst.is_dir() and not dst.is_symlink()
                ):
                    _rm_tree(dst)

            if stage_mode == "symlink":
                if not dst.exists() and not dst.is_symlink():
                    os.symlink(item, dst)
            else:
                _sync_copy_dir(item, dst)
        else:
            _link_or_copy_file(item, dst, mode=stage_mode)

    # Provide Arduino QUTest port headers + serial port implementation.
    for name in ("qp_port.h", "qs_port.h", "qutest_port_arduino.cpp"):
        desired_names.add(name)
        _link_or_copy_file(port_dir / name, staged_sketch_dir / name, mode=stage_mode)

    # Stage required QP/C sources into the sketch directory so arduino-cli compiles them.
    for rel in QPC_SOURCES:
        src = qpc_dir / rel
        if not src.exists():
            continue
        desired_names.add(src.name)
        _link_or_copy_file(src, staged_sketch_dir / src.name, mode=stage_mode)

    # Stage external library sources
    for lib_path in extra_libs:
        src = Path(lib_path)
        if not src.exists():
            continue

        if src.is_file():
            # For files, copy directly to sketch root to be included in the build
            dst = staged_sketch_dir / src.name
            desired_names.add(dst.name)
            _link_or_copy_file(src, dst, mode="copy")
        elif src.is_dir():
            # For directories, stage files into the sketch root as well
            for p in src.rglob("*"):
                if p.is_dir():
                    continue
                if p.suffix in {".c", ".cpp", ".h", ".hpp"}:
                    dst = staged_sketch_dir / p.name
                    desired_names.add(dst.name)
                    _link_or_copy_file(p, dst, mode="copy")

    # Stage headers from extra include directories into the sketch root
    # This ensures they are visible in the work-dir as requested by the user
    # and makes the sketch more self-contained.
    for inc_dir in extra_includes:
        src_dir = Path(inc_dir)
        if src_dir.exists() and src_dir.is_dir():
            for p in src_dir.iterdir():
                if p.is_file() and p.suffix in {".h", ".hpp", ".hh", ".hxx"}:
                    dst = staged_sketch_dir / p.name
                    if dst.name not in desired_names: # avoid overwriting sources/ports
                        desired_names.add(dst.name)
                        _link_or_copy_file(p, dst, mode="copy")

    # Remove stale files from the staged sketch root.
    for item in staged_sketch_dir.iterdir():
        if item.name in desired_names:
            continue
        _rm_tree(item)

    return staged_sketch_dir


def _arduino_cli_compile(
    arduino_cli: str,
    fqbn: str,
    staged_sketch_dir: Path,
    qpc_include_dir: Path,
    extra_defines: str,
    extra_includes: str,
    build_path: Path,
    build_cache_path: Path,
    output_dir: Path,
) -> None:
    print(f"DEBUG: Compiling sketch in {staged_sketch_dir}...")
    # Collect all directories containing header files in the staged workspace
    include_dirs = []
    for p in staged_sketch_dir.rglob("*.h"):
        if p.parent not in include_dirs:
            include_dirs.append(p.parent)

    extra_includes_flags = " ".join(f"-I{inc}" for inc in extra_includes)
    staged_includes_flags = " ".join(f"-I{inc}" for inc in include_dirs)
    all_extra_includes = f"-I{qpc_include_dir} -I{port_dir} {extra_includes_flags} {staged_includes_flags}"
    defs = f"-DQ_SPY -DQ_UTEST=1 {extra_defines}".strip()

    # NOTE: build properties are core-specific, but these work for common cores.
    cmd = [
        arduino_cli,
        "compile",
        "--fqbn",
        fqbn,
        "--build-path",
        str(build_path),
        "--output-dir",
        str(output_dir),
        "--build-property",
        f"compiler.cpp.extra_flags={defs} {all_extra_includes}",
        "--build-property",
        f"compiler.c.extra_flags={defs} {all_extra_includes}",
        str(staged_sketch_dir),
    ]
    print(f"DEBUG: Running compile command: {' '.join(cmd)}")
    subprocess.run(cmd, check=True, timeout=300)
    print("DEBUG: Compilation successful.")


def _arduino_cli_upload(
    arduino_cli: str,
    fqbn: str,
    port: str,
    sketch_dir: Path,
    input_dir: Path,
) -> None:
    print(f"DEBUG: Uploading sketch to {port}...")
    # Try to upload the compiled artifacts, fall back to compile+upload if needed.
    cmd = [
        arduino_cli,
        "upload",
        "--fqbn",
        fqbn,
        "--port",
        port,
        "--input-dir",
        str(input_dir),
        str(sketch_dir),
    ]
    print(f"DEBUG: Running upload command: {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True, timeout=60)
        print("DEBUG: Upload successful.")
        return
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print(f"DEBUG: Upload with --input-dir failed or timed out: {e}. Falling back to normal upload.")
        pass

    cmd = [
        arduino_cli,
        "upload",
        "--fqbn",
        fqbn,
        "--port",
        port,
        str(sketch_dir),
    ]
    print(f"DEBUG: Running fallback upload command: {' '.join(cmd)}")
    subprocess.run(cmd, check=True, timeout=90)
    print("DEBUG: Fallback upload successful.")


def _run_qutest(
    qutest_py: str,
    scripts: list[str],
    qutest_args: list[str],
    detach_sleep_s: float,
    timeout_s: float,
) -> int:
    spec = importlib.util.spec_from_file_location("_repspectre_qutest", qutest_py)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load qutest.py module from: {qutest_py}")

    qutest_mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(qutest_mod)

    # Optional speed-up: lower QUTest TIMEOUT (affects socket timeouts and some waits).
    qutest_mod.QUTest.TIMEOUT = timeout_s

    # Speed-up: QUTest sleeps for 1.0s inside QSpy._detach() by default.
    def _fast_detach():
        if qutest_mod.QSpy._sock is None:
            return
        qutest_mod.QSpy.send_to(
            qutest_mod.struct.pack("<B", qutest_mod.QSpy._QSPY_DETACH)
        )
        time.sleep(detach_sleep_s)
        qutest_mod.QSpy._sock.close()
        qutest_mod.QSpy._sock = None

    qutest_mod.QSpy._detach = staticmethod(_fast_detach)

    saved_argv = sys.argv
    sys.argv = [qutest_py, *qutest_args, *scripts]

    try:
        print(f"DEBUG: Calling qutest_mod.main() with scripts={scripts}")
        qutest_mod.main()
        print("DEBUG: qutest_mod.main() returned")
        return 0
    except SystemExit as exc:
        code = exc.code
        print(f"DEBUG: qutest_mod.main() exited with SystemExit({code})")
        if code is None:
            return 0
        if isinstance(code, int):
            return code
        return 1
    except Exception as e:
        print(f"DEBUG: qutest_mod.main() raised exception: {e}")
        return 1
    finally:
        sys.argv = saved_argv


def main() -> int:
    parser = argparse.ArgumentParser(description="Arduino HIL runner for QUTest/QSPY")

    parser.add_argument("--arduino-cli", default="arduino-cli", help="Path to arduino-cli")
    parser.add_argument("--fqbn", required=True, help="Arduino FQBN, e.g. esp32:esp32:esp32dev")
    parser.add_argument("--port", required=True, help="Upload serial port, e.g. /dev/ttyUSB0")
    parser.add_argument("--qspy", required=True, help="Path to qspy executable (QTools)")
    parser.add_argument("--qutest", required=True, help="Path to qutest.py (QTools)")
    parser.add_argument("--qspy-port", default="", help="Serial port for qspy (defaults to --port)")
    parser.add_argument("--qspy-baud", type=int, default=115200, help="Baud rate for qspy/Serial")
    parser.add_argument("--config", help="Path to HIL config JSON")

    parser.add_argument(
        "--qspy-extra",
        nargs="*",
        default=[],
        help="Extra args appended to qspy command line",
    )
    parser.add_argument(
        "--qutest-arg",
        action="append",
        default=[],
        help="Extra argument passed to qutest.py (repeatable)",
    )
    parser.add_argument("--qpc", default="", help="Path to QP/C source tree")
    parser.add_argument(
        "--sketch",
        required=True,
        help="Path to sketch directory (must contain <dir>/<dir>.ino)",
    )
    parser.add_argument(
        "--work-dir",
        default="",
        help="Parent directory for a cached workspace (defaults to a cache dir under $HOME; set to use a repo-local folder for CTest)",
    )
    parser.add_argument(
        "--ephemeral-work-dir",
        action="store_true",
        help="Use a fresh temp work dir per run (slower; disables build caching)",
    )
    parser.add_argument(
        "--keep-work-dir",
        action="store_true",
        help="Do not delete the staged directory (only meaningful with --ephemeral-work-dir)",
    )
    parser.add_argument(
        "--clean-work-dir",
        action="store_true",
        help="Delete cached workspace before running (forces full rebuild)",
    )
    parser.add_argument(
        "--clean-build",
        action="store_true",
        help="Delete cached build/output artifacts before running (forces rebuild+upload)",
    )
    parser.add_argument(
        "--stage-mode",
        choices=["symlink", "copy"],
        default="symlink" if os.name != "nt" else "copy",
        help="How to stage files into the workspace (symlink is fastest on Linux/macOS)",
    )

    parser.add_argument("scripts", nargs="*", help="One or more QUTest Python scripts")

    parser.add_argument("--no-build", action="store_true", help="Skip arduino-cli compile")
    parser.add_argument("--no-upload", action="store_true", help="Skip arduino-cli upload")
    parser.add_argument(
        "--force-build",
        action="store_true",
        help="Always run arduino-cli compile (even if inputs are unchanged)",
    )
    parser.add_argument(
        "--force-upload",
        action="store_true",
        help="Always run arduino-cli upload (even if binary is unchanged)",
    )
    parser.add_argument(
        "--after-upload-sleep",
        type=float,
        default=0.2,
        help="Seconds to wait after upload before starting qspy/qutest",
    )

    args = parser.parse_args()

    config = {}
    if args.config:
        config_path = Path(args.config).expanduser().resolve()
        if not config_path.exists():
            raise FileNotFoundError(f"Config file not found: {config_path}")

        with open(config_path, "r") as f:
            config = json.load(f)

    arduino_cfg = config.get("arduino", {})
    extra_includes = arduino_cfg.get("includes", [])
    print("INCLUDES:", extra_includes)
    extra_libs = arduino_cfg.get("libraries", [])
    print("LIBRARIES:", extra_libs)

    qpc_dir = Path(args.qpc).expanduser().resolve() if args.qpc else _default_qpc_dir()
    sketch_dir = Path(args.sketch).expanduser().resolve()

    qspy_path = _resolve_exe(args.qspy)
    qutest_path = str(Path(args.qutest).expanduser().resolve())
    if not Path(qutest_path).exists():
        raise FileNotFoundError(f"qutest.py not found: {qutest_path}")

    qspy_serial = args.qspy_port or args.port

    parent = Path(args.work_dir).expanduser().resolve() if args.work_dir else _default_work_dir()
    if parent is not None:
        parent.mkdir(parents=True, exist_ok=True)

    if args.ephemeral_work_dir:
        stage_root = Path(
            tempfile.mkdtemp(
                prefix=f"repspectre-hil-{sketch_dir.name}-",
                dir=str(parent) if parent is not None else None,
            )
        )
    else:
        ws = _workspace_key(args.fqbn, sketch_dir=sketch_dir, qpc_dir=qpc_dir)
        stage_root = (parent if parent is not None else Path(tempfile.gettempdir())) / (
            f"repspectre-hil-{sketch_dir.name}-{ws}"
        )

    if args.clean_work_dir and stage_root.exists():
        _rm_tree(stage_root)

    stage_root.mkdir(parents=True, exist_ok=True)

    try:
        staged_sketch_dir = _stage_sketch(
            sketch_dir=sketch_dir,
            qpc_dir=qpc_dir,
            port_dir=port_dir,
            out_root=stage_root,
            stage_mode=args.stage_mode,
            extra_libs=extra_libs,
            extra_includes=extra_includes,
        )

        build_path = stage_root / "build"
        cache_path = stage_root / "cache"
        out_dir = stage_root / "out"

        build_path.mkdir(parents=True, exist_ok=True)
        cache_path.mkdir(parents=True, exist_ok=True)
        out_dir.mkdir(parents=True, exist_ok=True)

        if args.clean_build:
            _rm_tree(build_path)
            _rm_tree(out_dir)
            build_path.mkdir(parents=True, exist_ok=True)
            out_dir.mkdir(parents=True, exist_ok=True)

        extra_defines = ""

        # ESP32-S3 commonly uses the built-in USB-Serial/JTAG (CDC) interface,
        # which the Arduino-ESP32 core exposes as HWCDCSerial and selects
        # via compile-time defines.
        if ("esp32s3" in args.fqbn) and ("/dev/ttyACM" in args.port):
            extra_defines = "-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1"

        stamp_path = stage_root / ".repspectre_hil_stamp.json"
        upload_stamp_path = stage_root / ".repspectre_hil_upload_stamp.json"

        inputs_digest = _compute_inputs_digest(
            fqbn=args.fqbn,
            sketch_dir=sketch_dir,
            qpc_dir=qpc_dir,
            port_dir=port_dir,
            extra_defines=extra_defines,
            stage_mode=args.stage_mode,
            extra_includes=extra_includes,
        )

        prev_stamp = _read_json(stamp_path)
        prev_digest = prev_stamp.get("digest", "")

        can_skip_build = (
            (not args.force_build)
            and (prev_stamp.get("version") == _STAMP_VERSION)
            and (prev_digest == inputs_digest)
            and _output_has_artifacts(out_dir)
        )

        did_build = False

        if not args.no_build and not can_skip_build:
            _arduino_cli_compile(
                arduino_cli=args.arduino_cli,
                fqbn=args.fqbn,
                staged_sketch_dir=staged_sketch_dir,
                qpc_include_dir=qpc_dir / "include",
                extra_defines=extra_defines,
                extra_includes=extra_includes,
                build_path=build_path,
                build_cache_path=cache_path,
                output_dir=out_dir,
            )
            did_build = True

            _write_json(
                stamp_path,
                {
                    "version": _STAMP_VERSION,
                    "digest": inputs_digest,
                    "fqbn": args.fqbn,
                    "sketch": str(sketch_dir),
                    "qpc": str(qpc_dir),
                    "port": str(port_dir),
                    "stage_mode": args.stage_mode,
                    "extra_defines": extra_defines,
                },
            )

        elif args.no_build:
            if not _output_has_artifacts(out_dir):
                raise RuntimeError(
                    f"--no-build was set but no prior build artifacts exist in: {out_dir}"
                )

        prev_upload_digest = _read_json(upload_stamp_path).get("digest", "")
        need_upload = args.force_upload or did_build or (prev_upload_digest != inputs_digest)

        if not args.no_upload and need_upload:
            _arduino_cli_upload(
                arduino_cli=args.arduino_cli,
                fqbn=args.fqbn,
                port=args.port,
                sketch_dir=staged_sketch_dir,
                input_dir=out_dir,
            )
            _write_json(upload_stamp_path, {"version": _STAMP_VERSION, "digest": inputs_digest})

        time.sleep(args.after_upload_sleep)

        # Start QSPY and connect it to the target serial port.
        # QTools docs for QUTest typically use: qspy -u -c <port> [-b<baud>]
        qspy_cmd = [
            qspy_path,
            "-k",
            "-v",
            "7.3",
            "-c",
            qspy_serial,
            f"-b{args.qspy_baud}",
            *args.qspy_extra,
        ]

        qspy_log = os.environ.get("QSPY_LOG", "")
        qspy_out = None
        qspy = None

        qutest_detach_sleep_s = float(os.environ.get("QUTEST_DETACH_SLEEP", "0.01"))

        # HIL is slower than host-based QUTest: uploads, USB/serial resets and
        # reboot time all add latency.
        qutest_timeout_s = float(os.environ.get("QUTEST_TIMEOUT", "3.0"))
        qspy_startup_delay_s = float(os.environ.get("QSPY_STARTUP_DELAY", "0.5"))

        try:
            if qspy_log:
                print(f"DEBUG: Starting QSPY, logging to {qspy_log}")
                qspy_out = open(qspy_log, "wb")
                qspy = subprocess.Popen(
                    qspy_cmd,
                    stdin=subprocess.DEVNULL,
                    stdout=qspy_out,
                    stderr=subprocess.STDOUT,
                )
            else:
                print("DEBUG: Starting QSPY")
                qspy = subprocess.Popen(
                    qspy_cmd,
                    stdin=subprocess.DEVNULL,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.STDOUT,
                )

            print(f"DEBUG: Waiting {qspy_startup_delay_s}s for QSPY to start...")
            time.sleep(qspy_startup_delay_s)
            print(f"DEBUG: Releasing serial reset lines on {qspy_serial}")
            _release_serial_reset_lines(qspy_serial)

            if not args.scripts:
                raise ValueError("No QUTest scripts provided")

            print("DEBUG: Initiating QUTest run...")
            return _run_qutest(
                qutest_py=qutest_path,
                scripts=[str(Path(s).expanduser().resolve()) for s in args.scripts],
                qutest_args=list(args.qutest_arg),
                detach_sleep_s=qutest_detach_sleep_s,
                timeout_s=qutest_timeout_s,
            )

        finally:
            if qspy is not None:
                print("DEBUG: Terminating QSPY...")
                _terminate_process(qspy, timeout_s=0.3)
            if qspy_out is not None:
                try:
                    qspy_out.close()
                except Exception:
                    pass

    finally:
        if args.ephemeral_work_dir:
            if args.keep_work_dir:
                print(f"[repspectre-hil] kept work dir: {stage_root}", file=sys.stderr)
            else:
                shutil.rmtree(stage_root, ignore_errors=True)
        else:
            # Cached workspaces are kept by default for incremental builds.
            if args.keep_work_dir:
                print(f"[repspectre-hil] workspace dir: {stage_root}", file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())

