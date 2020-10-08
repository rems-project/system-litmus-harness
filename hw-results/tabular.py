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
import collections

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


def collect_all(fnames):
    results = collections.defaultdict(lambda: (0, 0))
    running = collections.defaultdict(list)
    for fname in fnames:
        print(f"Collecting ... {fname}")
        with open(fname, "r") as f:
            for line in f:
                line = line.strip()
                m = re.match(
                    r"Observation (?P<tname>.+?): (?P<obs>\d+) \(of (?P<total>\d+)\)",
                    line,
                )
                if m:
                    test_name = m.group("tname")
                    observations = int(m.group("obs"))
                    total = int(m.group("total"))
                    (old_obs, old_total) = results[test_name]
                    results[test_name] = (old_obs + observations, old_total + total)
                    running[test_name].append(observations)
    return results, running


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


def write_table(grp_list, f, devices, includes=[], excludes=[], print_skips=True):
    filtered = []

    #  first we collect the list of tests we saw over all *devices*
    tests = collections.defaultdict(lambda: collections.defaultdict(lambda: (0, 0, [])))

    for d, (results, running) in devices.items():
        for test_name in results:
            cobs, ctotal, crun = tests[test_name][d]
            dobs, dtotal = results[test_name]
            drun = running[test_name]

            crun.extend(drun)
            tests[test_name][d] = (cobs + dobs, ctotal + dtotal, crun)

    # we assign orphaned tests a category
    orphans = tests.keys() - {t for (t, g) in grp_list}
    for orphan in orphans:
        grp_list.append(
            (orphan, ["all", "errata" if orphan.endswith(".errata") else "orhpan"])
        )

    for (test_name, groups) in sorted(grp_list, key=lambda t: (t[1], t[0])):
        if (includes and not any(g in includes for g in groups)) or (
            any(g in excludes for g in groups)
        ):
            if print_skips:
                print("Skip ! {}".format(test_name))
            continue
        filtered.append((test_name, groups))

    one_group = len(set(g for (_, grps) in filtered for g in grps if g != "all")) == 1

    # Total/Distribution for each device
    rows = []
    for (test_name, groups) in filtered:
        row = []
        group = groups[-1]
        # use verb to escape test and group names with symbols
        row.append(f"\\verb|{group}|")
        row.append(f"\\verb|{test_name}|")

        for d in devices:
            total_observations, total_runs, running = tests[test_name][d]

            if len(running) == 0:
                u = 0
                avg = 0
                run = 0
            else:
                u = math.sqrt(vari(running))
                avg = sum(running) / len(running)
                run = total_runs / len(running)

            h_total_observations = humanize(total_observations)
            h_total_runs = humanize(total_runs)
            h_u = humanize(u)
            h_avg = humanize(avg)
            h_run = humanize(run)

            row.append(f"{h_total_observations}/{h_total_runs}")
            if total_observations > 0:
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
        f.write("\\input{table_macros}")
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
            devices[device_dir_path.stem] = collect_all(logs)
    elif args.file:
        devices["all"] = collect_all(args.file)

    with open(root.parent / "litmus" / "test_list.txt", "r") as f:
        group_list = []
        for line in f:
            (_, _, _, _, test_name, *groups,) = line.split()
            group_list.append((test_name, groups))

    with open(root / args.all_file, "w") as f:
        write_table(group_list, f, devices, includes=includes, excludes=excludes)

    if args.standalone:
        with open(args.standalone_file, "w") as f:
            f.write("\\documentclass{standalone}\n")
            f.write("\\begin{document}\n")
            sio = io.StringIO()
            write_table(
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
                    write_table(
                        group_list,
                        f,
                        {d: (results, running)},
                        includes=includes,
                        excludes=excludes,
                    )


if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
