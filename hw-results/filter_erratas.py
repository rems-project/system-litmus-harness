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

    errata = collections.defaultdict(list)

    for item in content:
        date = parse_date(item["date"])

        for it in item["errata"]:
            if not it["test"]:
                print("skipping entry for", date)
                continue

            errata[date].append(it)

    return errata


def subs(fpath, tests):
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()

    for test in tests:
        if test.startswith("@"):
            for (tname, grps) in group_list:
                if test[1:] in grps:
                    content = content.replace(
                        f"Observation {tname}:", f"Observation {tname}.errata:"
                    )
        else:
            content = content.replace(
                f"Observation {test}:", f"Observation {test}.errata:"
            )

    with open(fpath, "w", encoding="utf-8") as f:
       f.write(content)


def filter_erratas(errata, dir):
    for f in dir.iterdir():
        results_date = parse_file_date(f.name)

        for errata_fixed_date, tests in errata.items():
            for test in tests:
                since_date = parse_date(test.get("since", "0001-1"))
                if since_date is None:
                    print('filter_erratas.py does not yet support "since" key with commit hash')
                    sys.exit(1)
                if results_date is None or since_date <= results_date <= errata_fixed_date:
                    tests = test['test'].split()
                    subs(f, tests)


# get absolute path to this directory
root = pathlib.Path(__file__).parent.resolve()

with open(root.parent / "litmus" / "test_list.txt", "r") as f:
    group_list = []
    for line in f:
        (_, _, _, _, test_name, *groups,) = line.split()
        group_list.append((test_name, groups))


errata = read_all_errata()
filter_erratas(errata, root / "raw" / "rpi4b")
filter_erratas(errata, root / "raw" / "rpi3bp")
filter_erratas(errata, root / "raw" / "graviton2")
