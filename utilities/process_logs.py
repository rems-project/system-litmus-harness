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

try:
    import colorama
except ImportError:
    colorama = None

parser = argparse.ArgumentParser()
parser.add_argument("--color", choices=["auto", "on", "none"], default="auto")
parser.add_argument("--lma", default=0x4008_0000, type=integer)
args = parser.parse_args()

if args.color == "none":
    colorama = None
elif args.color == "auto":
    if not sys.stdout.isatty():
        colorama = None


def overlaps(r1, r2):
    return (r1.start <= r2.start < r1.stop) or (r2.start <= r1.start < r2.stop)


class _Node:
    def __init__(self, r, label=None):
        #  invariants:
        self.range = r if r is not None else range(0)
        self.label = label
        self.children = []

    def insert(self, new: "_Node"):
        if self.children == [] and self.label is not None:
            #  we are a leaf, so turn into non-leaf and insert new Leaf
            r, l = self.range, self.label
            self.label = None
            self.insert(_Node(r, l))
            self.insert(new)
        else:
            for child in self.children:
                if overlaps(child.range, new.range):
                    return child.insert(new)

            #  no child overlapped
            self.children.append(new)
            self.range = range(
                min(self.range.start, new.range.start),
                max(self.range.stop, new.range.stop),
            )

    def search(self, addr):
        if self.children == [] and addr not in self.range:
            raise ValueError(f"cannot find {addr} in tree")

        elif self.children == [] and addr in self.range:
            return self.label

        else:
            for child in self.children:
                if addr in child.range:
                    return child.search(addr)

        raise ValueError(f"no child contained 0x{addr:x}")


class FuncTree:
    def __init__(self):
        self._tree = _Node(range(args.lma, 0x8000_0000))

    def insert(self, r, label):
        self._tree.insert(_Node(r, label))

    @functools.lru_cache()
    def find(self, addr):
        if addr == 0:
            return "NULL"

        try:
            return self._tree.search(addr)
        except ValueError as e:
            print(e)
            print()
            self.print_tree()
            raise

    def __iter__(self):
        d = collections.deque([self._tree])

        while d:
            n = d.popleft()
            for c in n.children[::-1]:
                d.appendleft(c)
            yield n

    def print_tree(self):
        for c in self:
            if c.label is not None:
                print(f"range(0x{c.range.start:x}, 0x{c.range.stop:x}) : {c.label}")

    @classmethod
    def from_items(cls, items):
        ft = cls()

        items = list(items)
        pairs = zip(items[0:], items[1:])

        for ((addr1, label1), (addr2, _)) in pairs:
            ft.insert(range(addr1, addr2), label1)

        final = items[-1]
        #  dont overshoot, so we spot errors
        ft.insert(range(final[0], final[0] + 0x1_000), final[1])
        return ft


def read_func_addrs(root: pathlib.Path):
    elf_out = root / "bin" / "litmus.elf.S"

    if not elf_out.exists():
        raise ValueError(f"{elf_out} does not exist")

    funcs = {}

    with elf_out.open(mode="r", encoding="utf-8") as f:
        for line in f:
            # only read .text section
            #  other labels can be ignored
            if line.startswith("Disassembly of section "):
                if ".text" not in line:
                    break
            if line.endswith(">:\n"):
                addr, _, funcname = line.partition("<")
                funcname, _, _ = funcname.partition(">")
                addr = int(addr.strip(), 16)
                funcs[args.lma + addr] = funcname

    if not funcs:
        raise ValueError("no function listing.  compile with DEBUG=1.")

    return FuncTree.from_items(sorted(funcs.items()))


def build_new_stack(stack, labels):
    addrs = stack.split(":")
    addrs = [addr for addr in addrs if addr.strip()]
    labels = [f"<{addr}: {labels.find(int(addr, 16))}>" for addr in addrs]
    return "->".join(labels)


def build_msg(m, labels):
    cpu = m.group("cpu")
    level = m.group("level")
    stack = m.group("stack")
    loc = m.group("loc")
    msg = m.group("msg")

    bold = colorama.Style.BRIGHT if colorama is not None else ""
    style = colorama.Fore.MAGENTA if colorama is not None else ""
    reset = colorama.Style.RESET_ALL if colorama is not None else ""

    new_stack = build_new_stack(stack, labels)
    new_msg = f"{style}CPU{cpu}:{level} [{new_stack}:{bold}{loc}{reset}{style}]\n\t{msg}{reset}\n\n"
    return new_msg


def build_strace(m, labels):
    indent = m.group("indent")
    el = m.group("el")
    old_stack = m.group("addrs")
    new_stack = build_new_stack(old_stack, labels)
    return f"{indent}[ STRACE {el}] {new_stack}\n"


def forever(root):
    labels = read_func_addrs(root)

    data = sys.stdin.buffer.read()
    data = data.decode("utf-8", errors="replace")
    lines = iter(data.splitlines(keepends=True))

    while True:
        try:
            line = next(lines)
        except StopIteration:
            break

        if not line:
            break

        if line.startswith("CPU"):
            #  might be a debug line
            m = re.fullmatch(
                r"CPU(?P<cpu>\d)"
                r":"
                r"(?P<level>.+?)"
                r":"
                r"\["
                r"(?P<stack>(0x(.+?):)+)"
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
            else:
                sys.stdout.write(line)
        elif "[ STRACE" in line:
            m = re.fullmatch(
                r"(?P<indent>\s*)\[ STRACE (?P<el>.+?)\] (?P<addrs>.+)\s*", line
            )

            if m:
                msg = build_strace(m, labels)
                sys.stdout.write(msg)
            else:
                sys.stdout.write(line)
        else:
            sys.stdout.write(line)


if __name__ == "__main__":
    root = pathlib.Path(__file__).parent.parent
    forever(root)
