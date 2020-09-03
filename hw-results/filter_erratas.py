import re
import sys
import pathlib
import datetime as dt
import collections

import yaml


def parse_date(date):
    formats = ["%Y-%B-%d", "%Y-%b-%d", "%Y-%j", "%Y-"]

    for fmt in formats:
        try:
            return dt.datetime.strptime(date, fmt).date()
        except ValueError:
            pass

    return None


def parse_file_date(fname):
    m = re.search(r"(\d{4})-overnight-(\d{1,3})", fname)
    if m:
        return parse_date("{}-{}".format(m.group(1), m.group(2)))

    m = re.search(r"(\d{4})-(.+)-(\d+)-.+", fname)
    if m:
        return parse_date("{}-{}-{}".format(m.group(1), m.group(2), m.group(3)))


def read_all_errata():
    with open("../errata.yaml", "r", encoding="utf-8") as f:
        content = yaml.load(f, Loader=yaml.SafeLoader)

    errata = collections.defaultdict(set)
    for item in content:
        date = parse_date(item["date"])
        tests = [it["test"] for it in item["errata"]]
        for test_list in tests:
            if not test_list:
                continue

            for test in test_list.split():
                errata[date].add(test)

    return errata


def subs(fpath, tests):
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    for test in tests:
        if test.startswith("@"):
            if test == "@all":
                content = re.sub(
                    r"Observation (.+):", r"Observation \g<1>.errata:", content
                )
            else:
                sys.exit("unsupported errata for group {}".format(test))
        else:
            content = content.replace(
                f"Observation {test}:", f"Observation {test}.errata:"
            )

    with open(fpath, "w", encoding="utf-8") as f:
        f.write(content)


def filter_erratas(errata, dir):
    for f in dir.iterdir():
        date = parse_file_date(f.name)

        for d in errata.keys():
            if date is None or date <= d:
                subs(f, errata[d])


cwd = pathlib.Path(".")

errata = read_all_errata()
filter_erratas(errata, cwd / "raw" / "rpi4b")
filter_erratas(errata, cwd / "raw" / "rpi3bp")
filter_erratas(errata, cwd / "raw" / "graviton2")
