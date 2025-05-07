#!/usr/bin/env python3

import sys
import shlex
import pathlib
import argparse
import subprocess

HERE = pathlib.Path(__file__).parent
ROOT = HERE.parent

LINUX_AARCH64_DEFAULTS = {
    "name": "LINUX_AARCH64_DEFAULTS",
    "cross_prefix": None,
    "tools": {
        "cc": "gcc",
        "ld": "ld",
        "objcopy": "objcopy",
        "objdump": "objdump",
        "gdb": "gdb",
        "qemu": "qemu-system-aarch64",
        "clang_format": "clang-format",
    },
}

LINUX_CROSS_DEFAULTS = {
    "name": "LINUX_CROSS_DEFAULTS",
    "cross_prefix": "aarch64-linux-gnu-",
    "tools": {
        "cross_cc": "gcc",
        "cross_ld": "ld",
        "cross_objcopy": "objcopy",
        "cross_objdump": "objdump",
        "cross_gdb": "gdb",
        "qemu": "qemu-system-aarch64",
        "clang_format": "clang_format",
    },
}

PLATFORM_DEFAULTS = {
    "Linux": {
        "aarch64": LINUX_AARCH64_DEFAULTS,
    },
}

GENERIC_DEFAULT = LINUX_CROSS_DEFAULTS


CONFIG = """\
# auto-generated file, do not edit!

CONFIGURE_CMD = {cmd}
CROSS-PREFIX = {cross_prefix}

# fully-qualified toolnames
CC = {cc}
LD = {ld}
OBJCOPY = {objcopy}
OBJDUMP = {objdump}
QEMU = {qemu}
GDB = {gdb}
CLANG-FORMAT = {clang_format}
"""


def get_uname(k):
    return subprocess.check_output(["uname", k], text=True).strip()


def cross_tool(tool):
    return "cross_" + tool


class CommandAction(argparse.Action):
    def __init__(
        self, option_strings, dest, default=None, required=False, help=None, cross=False
    ):

        self._options = option_strings
        self.cross = cross

        _option_strings = []
        for option_string in option_strings:
            _option_strings.append(option_string)

            if cross and option_string.startswith("--"):
                option_string = "--cross-" + option_string[2:]
                _option_strings.append(option_string)

        super().__init__(
            _option_strings,
            dest,
            default=default,
            required=required,
            help=help,
        )

    def __call__(self, _parser, namespace, values, option_string=None):
        if option_string is None:
            raise ValueError("unexpected positional")

        if option_string.startswith("--cross-"):
            dest = cross_tool(self.dest)
        else:
            dest = self.dest

        setattr(namespace, dest, values)

    def format_usage(self):
        # format_usage is only used for options which take no value
        # but all Command actions should take a value
        raise ValueError("unexpected value")

    def normalise(self, namespace):
        """turn --cross-prefix=P --cross-tool=X into --tool=XP"""
        prefix = namespace.cross_prefix
        cross_tool_suffix = getattr(namespace, cross_tool(self.dest), None)
        full_tool = getattr(namespace, self.dest, None)

        if self.cross and cross_tool_suffix is not None and full_tool is None:
            setattr(namespace, self.dest, prefix + cross_tool_suffix)

    def try_fill(self, namespace, defaults):
        dest = self.dest
        tool = getattr(namespace, dest, None)
        default_tool = defaults["tools"].get(dest, None)

        if self.cross:
            dest_cross_suffix = cross_tool(dest)
            tool_cross_suffix = getattr(namespace, dest_cross_suffix, None)
            default_tool_cross_suffix = defaults["tools"].get(dest_cross_suffix, None)

            if not tool and not tool_cross_suffix:
                if default_tool:
                    setattr(namespace, dest, default_tool)
                elif default_tool_cross_suffix:
                    setattr(
                        namespace, dest_cross_suffix, default_tool_cross_suffix
                    )
        else:
            if not tool and default_tool:
                setattr(namespace, dest, default_tool)


def normalise_args(args):
    if args.cross_prefix is None:
        args.cross_prefix = ""

    for action in parser._actions:
        if isinstance(action, CommandAction):
            action.normalise(args)


def fill_defaults(args):
    # fill in any already-provided prefixes etc
    normalise_args(args)

    plat_defaults = PLATFORM_DEFAULTS.get(args.platform, {}).get(args.arch, None)

    if plat_defaults is not None:
        defaults = plat_defaults
    else:
        defaults = GENERIC_DEFAULT

    if not args.cross_prefix and defaults["cross_prefix"]:
        args.cross_prefix = defaults["cross_prefix"]

    for action in parser._actions:
        if isinstance(action, CommandAction):
            action.try_fill(args, defaults)

    # now try fill in any given the defaults
    normalise_args(args)


def write_config(args):
    cmd = (
        " ".join(
            [
                sys.argv[0],
                *[shlex.quote(arg) for arg in sys.argv[1:]]
            ]
        )
    )

    with open(ROOT / "config.mk", "w") as f:
        f.write(CONFIG.format(**vars(args), cmd=cmd))


def main(argv):
    args = parser.parse_args(argv)
    fill_defaults(args)
    write_config(args)


parser = argparse.ArgumentParser()
parser.add_argument("--cross-prefix", default=None)
#
parser.add_argument("--cc", action=CommandAction, cross=True)
parser.add_argument("--ld", action=CommandAction, cross=True)
parser.add_argument("--objcopy", action=CommandAction, cross=True)
parser.add_argument("--objdump", action=CommandAction, cross=True)
parser.add_argument("--gdb", action=CommandAction, cross=True)
#
parser.add_argument("--qemu", action=CommandAction)
parser.add_argument("--clang-format", dest="clang_format", action=CommandAction)
#
parser.add_argument("--platform", default=get_uname("-s"))
parser.add_argument("--arch", default=get_uname("-m"))

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
