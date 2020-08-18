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

group = parser.add_mutually_exclusive_group()
group.add_argument("--file", "-f", action="append")
group.add_argument("--device", "-d", action="append")

parser.add_argument(
    "--excludes",
    help="Comma-separated list of excluded groups, e.g. --excludes=@grp1,@grp2",
)
parser.add_argument(
    "--includes",
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


def write_one_table(grp_list, f, device, results, running, includes=[], excludes=[], print_skips=True):
    filtered = []

    orphans = results.keys() - {t for (t, g) in grp_list}
    for orphan in orphans:
        grp_list.append((orphan, ["all", "errata"]))

    for (test_name, groups) in sorted(grp_list, key=lambda t: (t[1], t[0])):
        if (
            (includes and not any(g in includes for g in groups))
            or (any(g in excludes for g in groups))
            or (test_name not in results)
        ):
            if print_skips:
                print('Skip ! {}: {}'.format(device, test_name))
            continue
        filtered.append((test_name, groups))

    one_group = len(set(g for (_, grps) in filtered for g in grps if g != 'all')) == 1

    if one_group:
        f.write("\\begin{tabular}{r l l l}\n")
        f.write("\\textbf{Name} & \\textbf{Total} & \\textbf{Distribution} &\\\\\n")
    else:
        f.write("\\begin{tabular}{l r l l l}\n")
        f.write("\\textbf{Type} & \\textbf{Name} & \\textbf{Total} & \\textbf{Distribution} &\\\\\n")

    for (test_name, groups) in filtered:
        total_observations, total_runs = results[test_name]
        # assuming d is Poission distribution then var(d) = 1/n sum(d)
        d = running[test_name]
        u = math.sqrt(vari(d))
        avg = sum(d) / len(d)
        run = total_runs / len(d)

        h_total_observations = humanize(total_observations)
        h_total_runs = humanize(total_runs)
        h_u = humanize(u)
        h_avg = humanize(avg)
        h_run = humanize(run)

        group = groups[-1]
        f.write("   ")  # padding for nice .tex
        if not one_group:
            f.write(f"{group} &")

        f.write(f"{test_name} & {h_total_observations}/{h_total_runs}")
        if total_observations > 0:
            f.write(f" & {h_avg}/{h_run} & $\\pm$ {h_u}/{h_run} \\\\\n")
        else:
            f.write(f" & & \\\\\n")
    f.write("\\hline\n")
    f.write("\\end{tabular}\n")


def collect_logs(d):
    for fname in d.iterdir():
        if fname.suffix == ".log":
            yield str(fname.expanduser())


if __name__ == "__main__":
    args = parser.parse_args()

    excludes = [] if not args.excludes else args.excludes.split(",")
    includes = [] if not args.includes else args.includes.split(",")

    includes = [i.strip("@") for i in includes]
    excludes = [e.strip("@") for e in excludes]

    devices = {}
    if args.device:
        for device_dir in args.device:
            device_dir_path = root / device_dir
            if not device_dir_path.is_dir():
                raise ValueError("-d accepts directories not files")

            logs = collect_logs(device_dir_path)
            devices[device_dir_path.stem] = collect_all(logs)
    elif args.file:
        devices["all"] = collect_all(args.file)

    with open(root.parent / "litmus" / "test_list.txt", "r") as f:
        group_list = []
        for line in f:
            include, file_path, last_modified_timestamp, test_ident, test_name, *groups = line.split()
            group_list.append((test_name, groups))

    for d, (results, running) in devices.items():
        with open(root / f"results-{d!s}.tex", "w") as f:
            write_one_table(group_list, f, d, results, running, includes=includes, excludes=excludes)

    if args.standalone:
        with open(args.standalone_file, "w") as f:
            f.write("\\documentclass{standalone}\n")
            f.write("\\begin{document}\n")
            for d, (results, running) in devices.items():
                f.write(f"   \\textbf{{{d!s}}}\n")
                sio = io.StringIO()
                write_one_table(
                    group_list,
                    sio,
                    d,
                    results,
                    running,
                    includes=includes,
                    excludes=excludes,
                    print_skips=False,
                )
                for line in sio.getvalue().splitlines():
                    f.write(f"   {line}\n")
                f.write("   \\hspace{3cm}")
            f.write("\\end{document}\n")
