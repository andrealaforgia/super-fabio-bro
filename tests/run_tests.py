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
    "core", "level", "physics", "player", "enemy", "combat", "bot",
    "scenery", "sprites", "audio", "demo",
)


def source_directories():
    directories = [p for p in (ROOT / "engine").iterdir() if p.is_dir()]
    directories += [p for p in (ROOT / "game/src").iterdir() if p.is_dir()]
    return sorted(directories)


def sources():
    found = []
    for directory in source_directories():
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
