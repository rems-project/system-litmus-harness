#!/usr/bin/env python3
#
# list source files
#
# usage: list_src_files.py [-h] [--[no-]gitignore] [--[no-]include-litmus]
#
# options:
#   -h, --help            show this help message and exit
#   --gitignore, --no-gitignore
#                         respect gitignore (default: True)
#   --include-litmus, --no-include-litmus
#                         recurse into the litmus_tests dir (default: False)
#
# output:
#   prints one line per source file

import sys
import pathlib
import argparse
import subprocess


HERE = pathlib.Path(__file__).parent
ROOT = HERE.parent


COLLECT_SUFFIXES = [".c", ".h"]


def should_collect_src_file(f: pathlib.Path, args) -> bool:
    if not args.gitignore:
        return True

    cp = subprocess.run(
        ["git", "check-ignore", "-q", str(f)],
    )

    if cp.returncode == 0:
        return False
    elif cp.returncode == 1:
        return True
    else:
        raise ValueError(cp)


def collect_src_files(dir: pathlib.Path, args):
    for f in dir.iterdir():
        if f.is_dir():
            if (dir != ROOT / "litmus") or args.include_litmus:
                yield from collect_src_files(f, args=args)

        if f.suffix in COLLECT_SUFFIXES:
            if should_collect_src_file(f, args=args):
                yield f


def main(argv):
    args = parser.parse_args(argv)
    for f in collect_src_files(ROOT, args=args):
        print(f)


parser = argparse.ArgumentParser()
parser.add_argument(
    "--gitignore",
    action=argparse.BooleanOptionalAction,
    default=True,
    help="respect gitignore",
)
parser.add_argument(
    "--include-litmus",
    action=argparse.BooleanOptionalAction,
    default=False,
    help="recurse into the litmus_tests dir",
)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
