#!/usr/bin/env python
""" process_logs.py

Usage:
    cat log | ./process_logs.py
or  ./qemu_litmus -d | ./process_logs.py
"""

import re
import sys
import bisect
import pathlib
import argparse
import functools
import collections

from utils import integer
from source_location import read_func_addrs

try:
    import colorama
except ImportError:
    colorama = None

parser = argparse.ArgumentParser()
parser.add_argument("--color", choices=["auto", "on", "none"], default="auto")
parser.add_argument("--lma", default=0x4008_0000, type=integer)
parser.add_argument("--hide-header", action="store_true", default=False)
parser.add_argument("--unittests", action="store_true", default=False, help="Process logs from unittest run")
args = parser.parse_args()

if args.color == "none":
    colorama = None
elif args.color == "auto":
    if not sys.stdout.isatty():
        colorama = None

ELF_NAME = "litmus.elf" if not args.unittests else "unittests.elf"

def build_msg(m, labels):
    time = m.group("time")
    cpu = m.group("cpu")
    level = m.group("level")
    stack = m.group("stack")
    loc = m.group("loc")
    msg = m.group("msg")

    bold = colorama.Style.BRIGHT if colorama is not None else ""
    style = colorama.Fore.MAGENTA if colorama is not None else ""
    grey = colorama.Fore.LIGHTBLACK_EX if colorama is not None else ""
    reset = colorama.Style.RESET_ALL if colorama is not None else ""

    new_stack = build_new_stack(stack, labels)
    if not args.hide_header:
        new_msg = f"{bold}{grey}[{time}]{reset} {style}CPU{cpu}:{level} [{new_stack}:{bold}{loc}{reset}{style}]\n\t{msg}{reset}\n\n"
    else:
        new_msg = f"{msg}\n"
    return new_msg

def build_new_stack(stack, labels):
    addrs = stack.split(":")
    addrs = [addr for addr in addrs if addr.strip()]
    labels = [f"<{addr}: {labels.find(int(addr, 16))}>" for addr in addrs]
    return "->".join(labels)

def build_strace(m, labels):
    indent = m.group("indent")
    el = m.group("el")
    old_stack = m.group("addrs")
    new_stack = build_new_stack(old_stack, labels)
    return f"{indent}[ STRACE {el}] {new_stack}\n"


def forever(root):
    elf_out = (root / "bin" / ELF_NAME).with_suffix(".elf.S")
    labels = read_func_addrs(elf_out, args.lma)

    data = iter(sys.stdin.buffer.readline, b"")

    while True:
        try:
            line = next(data)
            line = line.decode("utf-8", errors="replace")
        except StopIteration:
            break

        if not line:
            break

        #  might be a debug line
        m = re.fullmatch(
            r"#?!?"
            r"\s*"
            r"(?P<time>\(\d+:\d+:\d+(:\d+)?)\)\s*"
            r"CPU(?P<cpu>\d)"
            r":"
            r"(?P<level>[^:]+?)"
            r":"
            r"\["
            r"(?P<stack>(0x([^:]+?):?)+\s)"
            r"(?P<loc>.+?)"
            r"\]"
            r" "
            r"(?P<msg>.+?)"
            r"\s*",
            line,
        )

        if m:
            try:
                msg = build_msg(m, labels)
            except ValueError as e:
                print(e)
                print(repr(line))
                raise
            sys.stdout.write(msg)
            continue


        m = re.fullmatch(
            r"(?:\#!?)?"
            r"(?P<indent>\s*)\[ STRACE (?P<el>.+?)\] (?P<addrs>.+)\s*", line
        )

        if m:
            msg = build_strace(m, labels)
            sys.stdout.write(msg)
            continue

        sys.stdout.write(line)


if __name__ == "__main__":
    root = pathlib.Path(__file__).parent.parent
    forever(root)
