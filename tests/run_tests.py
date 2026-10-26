"""Compile the engine and game logic (no SDL window, no GPU) into one test
binary and run each group in its own process."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

GROUPS = (
    "level", "physics", "player", "enemy", "combat", "book_reader", "score_popups", "bot",
    "scenery", "sprites", "audio",
)


# The engine pieces with no SDL in them; the rest is exercised by the engine's
# own tests and by the snapshots.
ENGINE_PURE_SOURCES = (
    "core/math/rng.c",
    "core/math/geometry.c",
    "core/time/stepper.c",
    "core/input/controls.c",
    "core/audio/mixer.c",
    "core/events/event_system.c",
    "core/memory/object_pool.c",
)


def source_directories():
    directories = sorted((ROOT / "engine/core").iterdir())
    directories += sorted((ROOT / "game/src").iterdir())
    return [p for p in directories if p.is_dir()]


def sources():
    found = [ROOT / "engine" / name for name in ENGINE_PURE_SOURCES]
    for directory in sorted((ROOT / "game/src").iterdir()):
        found += sorted(directory.glob("*.c"))
    return [p for p in found if p.name != "main.c" and not p.name.startswith("sdl_")]


def build(destination, sanitize):
    compiler = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-D_DEFAULT_SOURCE"]
    if sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    includes = ["-I" + str(d) for d in source_directories()] + ["-I" + str(ROOT / "tests")]
    objects = []
    for index, source in enumerate(sources() + sorted((ROOT / "tests").glob("test_*.c"))):
        target = destination.parent / (str(index) + ".o")
        subprocess.run(compiler + flags + includes + ["-c", str(source), "-o", str(target)], check=True)
        objects.append(str(target))
    subprocess.run(compiler + flags + objects + ["-lm", "-o", str(destination)], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("groups", nargs="*", default=list(GROUPS))
    args = parser.parse_args()
    environment = dict(os.environ)
    environment.setdefault("UBSAN_OPTIONS", "halt_on_error=1")
    environment.setdefault("ASAN_OPTIONS", "detect_leaks=0")
    failures = 0
    with tempfile.TemporaryDirectory(prefix="super-fabio-tests-") as directory:
        binary = Path(directory) / "tests"
        build(binary, args.sanitize)
        for name in args.groups:
            try:
                result = subprocess.run([str(binary), name], text=True, capture_output=True,
                                        timeout=120, cwd=str(ROOT), env=environment)
                passed, output = result.returncode == 0, result.stdout + result.stderr
            except subprocess.TimeoutExpired:
                passed, output = False, "Test exceeded 120 seconds"
            print(("PASS " if passed else "FAIL ") + name)
            if not passed:
                failures += 1
                print(output)
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
