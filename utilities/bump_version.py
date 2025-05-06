#!/usr/bin/env python3
import re
import sys
import enum
import argparse
import dataclasses

VERSION_PATTERN = re.compile(
    r"^(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)(?:-(?P<rem>.+))?$"
)


class VersionField(enum.Enum):
    MAJOR = 0
    MINOR = 1
    PATCH = 2
    REM = 3


@dataclasses.dataclass
class VersionInfo:
    major: int
    minor: int
    patch: int
    rem: "str | None"

    def __iter__(self):
        return iter((self.major, self.minor, self.patch, self.rem))

    def __str__(self):
        major, minor, patch, rem = self
        if rem:
            return f"{major}.{minor}.{patch}-{rem}"
        else:
            return f"{major}.{minor}.{patch}"

    def bump(self, field: VersionField):
        match field:
            case VersionField.MAJOR:
                self.major += 1
            case VersionField.MINOR:
                self.minor += 1
            case VersionField.PATCH:
                self.patch += 1
            case _:
                raise ValueError("cannot bump rem")

    @classmethod
    def from_version_string(cls, version):
        match = VERSION_PATTERN.fullmatch(version)

        major = int(match["major"])
        minor = int(match["minor"])
        patch = int(match["patch"])
        rem = match["rem"]
        return cls(major, minor, patch, rem)


def main(argv):
    args = parser.parse_args(argv)
    version_info = VersionInfo.from_version_string(args.version)
    for field in args.fields:
        version_info.bump(field)
    print(version_info)
    return 0


parser = argparse.ArgumentParser()

parser.add_argument("version", type=str)

parser.add_argument(
    "--major",
    dest="fields",
    default=[],
    action="append_const",
    const=VersionField.MAJOR,
)
parser.add_argument(
    "--minor",
    dest="fields",
    default=[],
    action="append_const",
    const=VersionField.MINOR,
)
parser.add_argument(
    "--patch",
    dest="fields",
    default=[],
    action="append_const",
    const=VersionField.PATCH,
)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
