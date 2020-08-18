import re
import sys
import pathlib
import datetime as dt
import collections

import yaml

def read_date(date):
    formats = [
        "%Y-%B-%d",
        "%Y-%b-%d",
        "%Y-%j",
        "%Y-"
    ]

    for fmt in formats:
        try:
            return dt.datetime.strptime(date, fmt).date()
        except ValueError:
            pass

    return None

def subs(fpath, tests):
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    for test in tests:
        if test.startswith("@"):
            if test == "@all":
                content = re.sub(r"Observation (.+):", r"Observation \g<1>.errata:", content)
            else:
                sys.exit("unsupported errata for group {}".format(test))
        else:
            content = content.replace(f"Observation {test}:", f"Observation {test}.errata:")

    with open(fpath, "w", encoding="utf-8") as f:
        f.write(content)

def read_file_date(fname):
    m = re.search(r"(\d{4})-overnight-(\d{1,3})", fname)
    if m:
        return read_date("{}-{}".format(m.group(1), m.group(2)))

    m = re.search(r"(\d{4})-(.+)-(\d+)-.+", fname)
    if m:
        return read_date("{}-{}-{}".format(m.group(1), m.group(2), m.group(3)))


def read_erratas() -> yaml.YAMLObject:
    with open('../errata.yaml', 'r', encoding='utf-8') as f:
        content = yaml.load(f, Loader=yaml.SafeLoader)

    errata = collections.defaultdict(set)
    for item in content:
        date = read_date(item['date'])
        tests = [it['test'] for it in item['errata']]
        for test in tests:
            errata[date].add(test)

    return errata

def filter_erratas(dir):
    for f in dir.iterdir():
        date = read_file_date(f.name)

        for d in errata.keys():
            if date is None or date <= d:
                subs(f, errata[d])

cwd = pathlib.Path(".")

errata = read_erratas()

filter_erratas(cwd/"rpi4b")
filter_erratas(cwd/"rpi3bp")
filter_erratas(cwd/"graviton2")