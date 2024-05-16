#!/usr/bin/env python
# prints out a debug symbol for a source location
# Example: ./utilities/source_location.py 0x4008_77774

import sys
import pathlib
import argparse
import functools
import collections

try:
    import colorama
except ImportError:
    colorama = None

from utils import integer

root = pathlib.Path(__file__).parent.parent.resolve()

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
    def __init__(self, lma: int):
        self._tree = _Node(range(lma, 0x8000_0000))

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
    def from_items(cls, lma, items):
        ft = cls(lma)

        items = list(items)
        pairs = zip(items[0:], items[1:])

        for ((addr1, label1), (addr2, _)) in pairs:
            ft.insert(range(addr1, addr2), label1)

        final = items[-1]
        #  dont overshoot, so we spot errors
        ft.insert(range(final[0], final[0] + 0x1_000), final[1])
        return ft


def read_func_addrs(elf: pathlib.Path, lma:int):
    if not elf.exists():
        raise ValueError(f"{elf} does not exist")

    funcs = {}

    with elf.open(mode="r", encoding="utf-8") as f:
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
                funcs[lma + addr] = funcname

    if not funcs:
        raise ValueError("no function listing.  compile with DEBUG=1.")

    return FuncTree.from_items(lma, sorted(funcs.items()))

def main(args):
    elf_out = (root / "bin" / ELF_NAME).with_suffix(".elf.S")
    labels = read_func_addrs(elf_out, args.lma)
    print(f"0x{args.loc:08x}: {labels.find(args.loc)}")
    return 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("loc", metavar="LOC", type=integer)
    parser.add_argument("--color", choices=["auto", "on", "none"], default="auto")
    parser.add_argument("--lma", default=0x4008_0000, type=integer)
    parser.add_argument("--unittests", action="store_true", default=False, help="Process logs from unittest run")
    args = parser.parse_args()

    if args.color == "none":
        colorama = None
    elif args.color == "auto":
        if not sys.stdout.isatty():
            colorama = None

    ELF_NAME = "litmus.elf" if not args.unittests else "unittests.elf"

    sys.exit(main(args))