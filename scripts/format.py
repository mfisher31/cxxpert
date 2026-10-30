#!/usr/bin/env python3
"""Run clang-format over all C++ sources in src/.

Usage:
    python3 scripts/format.py           # format files in place
    python3 scripts/format.py --check   # report unformatted files, exit 1 if any
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
EXTENSIONS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="don't modify files; fail if any need formatting")
    parser.add_argument("--clang-format", default="clang-format",
                        help="clang-format executable to use")
    args = parser.parse_args()

    exe = shutil.which(args.clang_format)
    if exe is None:
        print(f"error: '{args.clang_format}' not found on PATH", file=sys.stderr)
        return 2

    files = sorted(p for p in SRC.rglob("*") if p.suffix in EXTENSIONS)
    if not files:
        print("no source files found")
        return 0

    cmd = [exe, "--style=file"]
    cmd += ["--dry-run", "--Werror"] if args.check else ["-i"]
    result = subprocess.run(cmd + [str(f) for f in files], cwd=ROOT)

    if result.returncode == 0:
        verb = "checked" if args.check else "formatted"
        print(f"{verb} {len(files)} file(s)")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
