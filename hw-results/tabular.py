""" tabular.py
Given 1 or more files containing the output of litmus.exe cat them together and build a LaTeX tabular
environment to embed into documents.

Usage:
    python tabular.py -d device1/  -d device2/

Example:
> ./litmus.exe @all -p -n1000 > out.log
> python3 tabular.py -f out.log --standalone
> cat top.tex
"""

import io
import re
import sys
import math
import pathlib
import argparse
import operator
import functools
import contextlib
import collections
import dataclasses

root = pathlib.Path(__file__).parent.absolute()

parser = argparse.ArgumentParser()

parser.add_argument("--standalone", action="store_true")
parser.add_argument("--standalone-file", "-o", default="top.tex")
parser.add_argument("--all-file", default="results-all.tex")
parser.add_argument("--macros", action="store_true")

group = parser.add_mutually_exclusive_group()
group.add_argument("--file", "-f", action="append")
group.add_argument("--device", "-d", action="append")

parser.add_argument(
    "--excludes",
    action='append',
    help="Comma-separated list of excluded groups, e.g. --excludes=@grp1,@grp2",
)
parser.add_argument(
    "--includes",
    action='append',
    help="Comma-separated list of included groups, e.g. --includes=@grp1,@grp2",
)

# the results are a set of tests, each with a set of results
# where each result is a multiset of outcomes, where some outcomes
# are specially marked to be recorded

@dataclasses.dataclass(frozen=True)
class Device:
    name: "str"
    dir: "str"

@dataclasses.dataclass
class RunResult:
    regvals: str
    count: int
    is_marked: "bool" = False

@dataclasses.dataclass
class Hist:
    test_name: str
    results: "List[RunResult]"

    def merge(self) -> "Mapping[str, int]":
        rs = collections.defaultdict(int)

        for r in self.results:
            rs[r.regvals] += r.count

        return rs

    def update(self, other: "Hist") -> None:
        if other.test_name != self.test_name:
            raise ValueError(f"not same test name, self={self.test_name}, other={other.test_name}")

        self.results.extend(other.results)

TestName = str

@dataclasses.dataclass
class LogFileResult:
    """we collect all of the results of a .log file into a LogFileResult
    """
    device: "Device"
    results: "List[Hist]"
    total: "Mapping[TestName, Hist]"


@dataclasses.dataclass
class FilteredLog:
    """the result for a particular test on a particular device
    after filtering
    """
    device: Device
    test_name: TestName
    results: "List[Hist]" = dataclasses.field(default_factory=list)
    total: (int, int) = (0, 0)   # (count_observations, total)
    running: "List[int]" = dataclasses.field(default_factory=list) # [obs_in_500k, obs_in_500k, ...]
    distribution: (int, int) = (0,0) # (avg_in_500k, variance_in_500k)
    batch_size: int = 500_000

    def merge(self):
        """take all of the Hists and merge them into one Hist
        """
        return Hist(self.test_name, [rr for r in self.results for rr in r.results]).merge()

@dataclasses.dataclass
class FilteredTest:
    """the results for a test over many devices
    """
    test_name: TestName
    groups: "List[str]"
    results: "Mapping[Device, FilteredLog]"


def read_test_file(device: Device, fname: str) -> LogFileResult:
    with open(fname, "r") as f:
        total = {}
        lfr = LogFileResult(device, [], total)

        recorded_test = []
        current_test = None

        # Assume entry looks like:
        # Test MP+dmb+svc-R-svc-R:
        #  p1:x0=0  p1:x2=1  : 35620
        #  p1:x0=0  p1:x2=0  : 919
        #  p1:x0=1  p1:x2=1  : 463461
        # Observation MP+dmb+svc-R-svc-R: 0 (of 500000)

        for line in f:
            line = line.strip()

            # first "Test ....:" line
            _, prefix, name = line.partition("Test ")
            if prefix:
                name, _, _ = name.partition(":")
                current_test = name
                continue

            # for each " pN:xM=K other_reg=L..." line
            m = re.match(
                r"\s*((.+=\d+)\s+)+\s*:\s*\d+",
                line,
            )

            if m:
                regpairs, _, count = line.rpartition(":")
                count, is_marked, _ = count.partition("*")
                rr = RunResult(regpairs.strip(), int(count), bool(is_marked))
                recorded_test.append(rr)
                continue

            m = re.match(
                r"Observation (?P<tname>.+?): (?P<obs>\d+) \(of (?P<total>\d+)\)",
                line,
            )
            if m:
                test_name = m.group("tname")
                h = Hist(test_name, recorded_test)
                lfr.results += [h]
                if test_name not in lfr.total:
                    lfr.total[test_name] = Hist(test_name, [])
                lfr.total[test_name].update(h)
                recorded_test = []
                continue

            # catch Exception as special outcome
            m = re.match(
                r"\s*Unhandled Exception",
                line
            )

            if m:
                rr = RunResult("*exception*:1", 1)
                test_name = current_test
                h = Hist(test_name, recorded_test + [rr])
                lfr.results += [h]
                if test_name not in lfr.total:
                    lfr.total[test_name] = Hist(test_name, [])
                lfr.total[test_name].update(h)
                recorded_test = []

    return lfr


def collect_all(d: Device, fnames) -> 'Mapping[TestName, TestLog]':
    for fname in fnames:
        print(f"-- Collecting {fname}")

        yield read_test_file(d, fname)


def vari(xs):
    mean = sum(xs) / len(xs)
    dists = [(x - mean) ** 2 for x in xs]
    return sum(dists) / len(xs)


def humanize(n):
    prefixes = {int(1e9): "G", int(1e6): "M", int(1e3): "K"}
    for p, unit in sorted(prefixes.items(), reverse=True):
        if n >= p:
            d = n
            n /= p
            if d / p == d // p:
                return f"{n:.0f}{unit}"
            return f"{n:.2f}{unit}"

    if isinstance(n, int):
        return str(n)

    return f"{n:.2f}"


MACROS_TEX = r"""
\newcommand\comment[3]{{%
  \expandafter\renewcommand\csname #1\endcsname{{#2 & #3 &}}%
}}

\newcommand\shapemacro{{l}}
\newcommand\headermacro{{DEFAULT HEADER}}
\newcommand\titlemacro{{DEFAULT TITLE}}
%% Auto generated
{tests}
"""

def accumulate(l: FilteredLog, h: Hist) -> None:
    # may be < 500k if test failed due to exception
    assert sum(r.count for r in h.results) <= 500_000
    l.results += [h]
    c, t = l.total
    for r in h.results:
        if r.is_marked:
            c += r.count
            l.running += [r.count]
        t += r.count
    l.total = (c, t)
    l.batch_size = 500_000

def filter_devices(grp_list, devices: "Mapping[Device, List[LogFileResult]]", includes=[], excludes=[], print_skips=False):
    """given the collection of log results
    filter out the excluded tests and create
    a FilteredLog for each test
    """
    #  first we collect the list of tests we saw over all devices
    # this is a
    tests : "Mapping[TestName, FilteredTest]" = {}
    test_names = set()

    for d, lfr_list in devices.items():
        for lfr in lfr_list:
            for r in lfr.results:
                test_name = r.test_name
                test_names.add(test_name)
                if test_name not in tests:
                    tests[test_name] = FilteredTest(test_name, [], {})
                if d not in tests[test_name].results:
                    tests[test_name].results[d] = FilteredLog(
                        device=d,
                        test_name=test_name,
                        results=[],
                        total=(0,0),
                        running=[],
                        distribution=(0,0),
                    )
                accumulate(tests[test_name].results[d], r)

    for ftest in tests.values():
        for d, flog in ftest.results.items():
            r = flog.running
            if len(r) == 0:
                u = 0
                sd = 0
            else:
                u = sum(r) / len(r)
                sd = math.sqrt(vari(r))
            flog.distribution = (u, sd)

    # we assign orphaned tests a category
    orphans = test_names - {t for (t, g) in grp_list}
    for orphan in orphans:
        grp_list.append(
            (orphan, ["all", "errata" if orphan.endswith(".errata") else "orhpan"])
        )

    for (test_name, groups) in sorted(grp_list, key=lambda t: (t[1], t[0])):
        # not all tests in the test list will necessarily have results
        if test_name in tests:
            tests[test_name].groups = groups

        if (includes and not any(g in includes for g in groups)) or (
            any(g in excludes for g in groups)
        ):
            if print_skips:
                print("Skip ! {}".format(test_name))

            # just remove from the test dictionary directly
            # we do a little redundant work up to this point
            # but it makes the above code simpler
            del tests[test_name]
            continue

    return tests

def write_explicit_table(grp_list, f, devices, includes=[], excludes=[]):
    """write out an explicit table of results
    with the histogram given for each result in full

    for now just ascii text
    """

    filtered_tests = filter_devices(grp_list, devices, includes, excludes)

    table = []
    widths = []
    headings = {}
    next_level_widths = {}
    next_level_headings = {}

    def merge(l1, l2):
        if l1 is None:
            return l2

        if l2 is None:
            return l1

        l1, l2 = (l1, l2) if len(l1) < len(l2) else (l2, l1)
        return l1 + l2[len(l1):]

    @contextlib.contextmanager
    def next_row():
        row = []

        def _append(v, heading=None, next_level_heading=None):
            if not v:
                return

            i = len(row)
            headings[i] = merge(headings.get(i, ""), heading)
            next_level_headings[i] = merge(next_level_headings.get(i, []), next_level_heading)
            row.append(v)

        yield _append

        for i, v in enumerate(row):
            if i == len(widths):
                widths.append(0)

            if isinstance(v, list):
                # multicol
                if i not in next_level_widths:
                    next_level_widths[i] = [0]*len(v)

                for j, w in enumerate(v):
                    if j == len(next_level_widths[i]):
                        next_level_widths[i].append(0)

                    next_level_widths[i][j] = max(next_level_widths[i][j], len(w))
                    if next_level_headings[i][j]:
                        next_level_widths[i][j] = max(next_level_widths[i][j], len(next_level_headings[i][j]))

                widths[i] = max(widths[i], len(next_level_widths[i]) + sum(next_level_widths[i]))
            else:
                widths[i] = max(widths[i], len(v))

            if headings[i]:
                widths[i] = max(widths[i], len(headings[i]))

        if row:
            table.append(row)

    @contextlib.contextmanager
    def new_multicol(r, heading=None):
        c = []
        next_level_heading = []
        def _append(v, heading=None):
            if not v:
                return
            next_level_heading.append(heading)
            c.append(v)
        yield _append

        if c:
            r(c, heading=heading, next_level_heading=next_level_heading)

    for ftest in filtered_tests.values():
        test_name = ftest.test_name
        groups = ftest.groups

        with next_row() as row:
            group = groups[-1]
            # use verb to escape test and group names with symbols
            row(f"{group}", heading="group")
            row(f"{test_name}", heading="test")

            for d in devices:
                with new_multicol(row, heading=f"{d.name}") as mcol:
                    # hack: insert zero'd entry
                    if d not in filtered_tests[test_name].results:
                        filtered_tests[test_name].results[d] = FilteredLog(d, test_name)

                    flog = filtered_tests[test_name].results[d]
                    total_obs, total_runs = flog.total
                    mcol(f"{humanize(total_obs)}/{humanize(total_runs)}", heading="total")

                    for oi, (r, c) in enumerate(sorted(flog.merge().items(), reverse=True, key=operator.itemgetter(1))):
                        mcol(f"{humanize(c)}*{{{','.join(r.split())}}}", heading=f"outcome#{oi}")

    # write header
    for i, v in sorted(headings.items()):
        v = v.ljust(widths[i])
        f.write(f"{v} ")
    f.write("\n")

    for i, h in sorted(next_level_headings.items()):
        if not isinstance(h, list):
            f.write(" "*(1+widths[i]))
        else:
            tot = 0

            for j, v in enumerate(h):
                w = next_level_widths[i][j]
                tot += w+1
                if not v:
                    f.write(" "*(1+w))
                else:
                    f.write(v.rjust(w) + " ")
            f.write(" "*(widths[i]-tot))
    f.write("\n")
    for i, v in sorted(headings.items()):
        w = widths[i]
        f.write("-"*w)
        if i < len(headings):
            f.write("+")
    f.write("\n")

    # whitespace align everything
    for row in table:
        for ci, c in enumerate(row):
            if isinstance(c, list):
                tot = 0
                for mci, mc in enumerate(c):
                    w = next_level_widths[ci][mci]
                    tot += w+1
                    mc = mc.rjust(w)
                    f.write(mc + " ")
                f.write(" "*(widths[ci]-tot))
            else:
                c = c.rjust(widths[ci])
                f.write(c + " ")

        f.write("\n")

def write_combo_table(grp_list, f, devices: "Mapping[Device, List[LogFileResult]]", includes=[], excludes=[], print_skips=True):
    """write out a standard table of results for all the devices
    """
    filtered_tests = filter_devices(grp_list, devices, includes, excludes)
    all_groups = set(g for ftest in filtered_tests.values() for g in ftest.groups if g != "all")
    one_group = len(all_groups) == 1

    # Total/Distribution for each device
    rows = []
    for ftest in filtered_tests.values():
        test_name = ftest.test_name
        groups = ftest.groups

        row = []
        group = groups[-1]
        # use verb to escape test and group names with symbols
        row.append(f"\\verb|{group}|")
        row.append(f"\\verb|{test_name}|")

        for d in devices:
            # hack: insert zero'd entry
            if test_name not in filtered_tests[test_name].results:
                filtered_tests[test_name].results[d] = FilteredLog(d, test_name)

            flog = filtered_tests[test_name].results[d]
            total_obs, total_runs = flog.total
            avg, u = flog.distribution
            run = flog.batch_size
            h_total_observations = humanize(total_obs)
            h_total_runs = humanize(total_runs)
            h_u = humanize(u)
            h_avg = humanize(avg)
            h_run = humanize(run)

            row.append(f"{h_total_observations}/{h_total_runs}")
            if total_obs > 0:
                row.append(f"{h_avg}/{h_run}")
                row.append(f"$\\pm$ {h_u}/{h_run}")
            else:
                row.append("")
                row.append("")

        row.append(f"\\csname {test_name}\\endcsname")
        rows.append(row)

    maxcolrows = [max(len(r[i]) for r in rows) for i in range(len(rows[0]))]
    if len("\\textbf{Type}") > maxcolrows[0]:
        maxcolrows[0] = len("\\textbf{Type}")
    if len("\\textbf{Name}") > maxcolrows[1]:
        maxcolrows[1] = len("\\textbf{Name}")
    for i, _ in enumerate(devices):
        if len("\\textbf{Total}") > maxcolrows[2 + 3 * i]:
            maxcolrows[2 + 3 * i] = len("\\textbf{Total}")
    for i, _ in enumerate(devices):
        if len("\\textbf{Distribution}") > maxcolrows[2 + 3 * i + 1]:
            maxcolrows[2 + 3 * i + 1] = len("\\textbf{Distribution}")

    device_headers = [
        "\\multicolumn{3}{l}{\\textbf{%s}}".ljust(
            3 + sum(maxcolrows[2 + 3 * i : 2 + 3 * i + 3])
        )
        % d
        for i, d in enumerate(devices)
    ]
    device_cols_heads = []
    for i, _ in enumerate(devices):
        device_cols_heads.append("\\textbf{Total}".ljust(maxcolrows[2 + 3 * i]))
        device_cols_heads.append(
            "\\textbf{Distribution}".ljust(maxcolrows[2 + 3 * i + 1])
        )
        device_cols_heads.append("".ljust(maxcolrows[2 + 3 * i + 2]))

    for i, hd in enumerate(device_cols_heads, start=2):
        if len(hd) > maxcolrows[i]:
            maxcolrows[i] = len(hd)

    device_cols_ls = " | r r l" * len(devices)
    tab_shape = "l "
    if not one_group:
        tab_shape += "l "
    tab_shape += device_cols_ls
    tab_shape += " l"
    if args.macros:
        tab_shape += " | \\shapemacro"

    if args.macros:
        f.write("\\input{table_macros}\n")
        with open("table_macros.tex", "w") as g:
            test_macros = [f"\\expandafter\\newcommand\\csname {t}\\endcsname{{}}" for (t, _) in filtered]
            g.write(MACROS_TEX.format(tests="\n".join(test_macros)))

    f.write("\\begin{tabular}{%s}\n" % tab_shape)

    # first line
    #  Type  & Name & multicol{3}{rpi4}         &  multicol{3}{qemu} & etc
    f.write("   ")
    if not one_group:
        f.write("\\textbf{Type}".ljust(maxcolrows[0]) + " & ")
    f.write("\\textbf{Name}".ljust(maxcolrows[1]) + " & ")
    f.write("%s & " % " & ".join(device_headers))
    if args.macros:
        f.write("\\headermacro{} & ".ljust(maxcolrows[-1]))
    f.write("\\\\\n")
    # second line
    #               & total & distribution &   & total & distribution & ... & etc
    f.write("   ")
    if not one_group:
        f.write("".ljust(maxcolrows[0]) + " & ")
    f.write("".ljust(maxcolrows[1]) + " & ")
    f.write("%s & " % " & ".join(device_cols_heads))
    if args.macros:
        f.write("\\titlemacro{} & ".ljust(maxcolrows[-1]))
    f.write("\\\\\n")

    for row in rows:
        f.write("   ")
        for i, (c, maxlen) in enumerate(zip(row, maxcolrows)):
            if one_group and i == 0:
                continue
            if not args.macros and i == len(row) - 1:
                continue
            f.write(f"{c!s:>{maxlen}} & ")
        f.write("\\\\ \\hline \n")
    f.write("\\end{tabular}\n")


def collect_logs(d):
    for fname in d.iterdir():
        if fname.suffix == ".log":
            yield str(fname.expanduser())


def main(args):
    excludes = [] if not args.excludes else [a for exclargs in args.excludes for a in exclargs.split(",")]
    includes = [] if not args.includes else [a for inclargs in args.includes for a in inclargs.split(",")]

    includes = [i.strip("@") for i in includes]
    excludes = [e.strip("@") for e in excludes]

    includes = [i for i in includes if i]
    excludes = [e for e in excludes if e]

    devices = {}
    if args.device:
        for device_dir in args.device:
            device_dir_path = root / "raw" / device_dir
            if not device_dir_path.is_dir():
                raise ValueError(
                    "-d accepts directories not files: {}".format(device_dir_path)
                )

            logs = collect_logs(device_dir_path)
            d = Device(device_dir_path.stem, device_dir_path)
            devices[d] = list(collect_all(d, logs))
    elif args.file:
        d = Device("all", ".")
        devices[d] = list(collect_all(d, args.file))

    with open(root.parent / "litmus" / "test_list.txt", "r") as f:
        group_list = []
        for line in f:
            (_, _, _, _, test_name, *groups,) = line.split()
            group_list.append((test_name, groups))

    with open(root / args.all_file, "w") as f:
        write_combo_table(group_list, f, devices, includes=includes, excludes=excludes)

    with open(root / "results-breakdown.txt", "w") as f:
        write_explicit_table(group_list, f, devices, includes=includes, excludes=excludes)

    if args.standalone:
        with open(args.standalone_file, "w") as f:
            f.write("\\documentclass{standalone}\n")
            f.write("\\begin{document}\n")
            sio = io.StringIO()
            write_combo_table(
                group_list,
                sio,
                devices,
                includes=includes,
                excludes=excludes,
                print_skips=False,
            )
            for line in sio.getvalue().splitlines():
                f.write(f"   {line}\n")
            f.write("\\end{document}\n")
    else:
        for d, (results, running) in devices.items():
                with open(root / f"results-{d!s}.tex", "w") as f:
                    write_combo_table(
                        group_list,
                        f,
                        {d: (results, running)},
                        includes=includes,
                        excludes=excludes,
                    )


if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
