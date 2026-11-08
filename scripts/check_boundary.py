"""Fails if the game names the library the engine runs on.

The engine hides its platform behind its own API. Everything in game/ and
tests/ must stay on the engine's side of that line: no platform headers, types
or calls, and no engine header whose name ends in _internal.h. The compiler
enforces the same rule from the other direction (the game is built with no
platform include flags, and with stub headers in front that refuse to compile),
so this is the readable half of the check."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = re.compile(r"\b(SDL\w*|IMG_\w*|TTF_\w*|Uint(8|16|32|64)|Sint(8|16|32|64))\b|_internal\.h")
SKIPPED = ("scripts/boundary_stubs",)


def main():
    failures = []
    for folder in ("game", "tests"):
        for path in sorted((ROOT / folder).rglob("*")):
            if path.suffix not in (".c", ".h", ".py"):
                continue
            for number, line in enumerate(path.read_text().splitlines(), 1):
                match = FORBIDDEN.search(line)
                if match:
                    failures.append("%s:%d: %s" % (path.relative_to(ROOT), number, match.group(0)))
    if failures:
        print("The game names the platform library behind the engine:")
        print("\n".join(failures))
        return 1
    print("PASS the game names no platform library")
    return 0


if __name__ == "__main__":
    sys.exit(main())
