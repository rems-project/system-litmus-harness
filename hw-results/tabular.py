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

parser = argparse.ArgumentParser()

parser.add_argument("--standalone", action="store_true")
parser.add_argument("--standalone-file", "-o", default="top.tex")
group = parser.add_mutually_exclusive_group()
group.add_argument("--file", "-f", action="append")
group.add_argument("--device", "-d", action="append")


def collect_all(fnames):
    results = collections.defaultdict(lambda: (0,0))
    running = collections.defaultdict(list)
    for fname in fnames:
        print(f'Collecting ... {fname}')
        with open(fname, "r") as f:
            for line in f:
                line = line.strip()
                m = re.match(r"Observation (?P<tname>.+?): (?P<obs>\d+) \(of (?P<total>\d+)\)", line)
                if m:
                    test_name = m.group("tname")
                    observations = int(m.group("obs"))
                    total = int(m.group("total"))
                    (old_obs, old_total) = results[test_name]
                    results[test_name] = (old_obs+observations, old_total+total)
                    running[test_name].append(observations)
    return results, running

def vari(xs):
    mean = sum(xs)/len(xs)
    dists = [(x-mean)**2 for x in xs]
    return sum(dists)/len(xs)


def humanize(n):
    prefixes = {int(1e9): "G", int(1e6): "M", int(1e3): "K"}
    for p, unit in sorted(prefixes.items(), reverse=True):
        if n >= p:
            d = n
            n /= p
            if (d / p == d // p):
                return f"{n:.0f}{unit}"
            return f"{n:.2f}{unit}"

    if isinstance(n, int):
        return str(n)

    return f"{n:.2f}"


def write_one_table(f, device, results, running):
    f.write("\\begin{tabular}{r l l l}\n")
    f.write("\\textbf{Name} & \\textbf{Total} & \\textbf{Distribution} &\\\\\n")
    for test_name in sorted(results.keys()):
        total_observations, total_runs = results[test_name]
        # assuming d is Poission distribution then var(d) = 1/n sum(d)
        d = running[test_name]
        u = math.sqrt(vari(d))
        avg = sum(d)/len(d)
        run = total_runs/len(d)

        h_total_observations = humanize(total_observations)
        h_total_runs = humanize(total_runs)
        h_u = humanize(u)
        h_avg = humanize(avg)
        h_run = humanize(run)
        f.write(f"   {test_name} & {h_total_observations}/{h_total_runs}")
        if total_observations > 0:
            f.write(f" & {h_avg}/{h_run} & $\\pm$ {h_u}/{h_run} \\\\\n")
        else:
            f.write(f" & & \\\\\n")
    f.write("\\hline\n")
    f.write("\\end{tabular}\n")


def collect_logs(d):
    for fname in d.iterdir():
        if fname.suffix == '.log':
            yield str(fname.expanduser())


if __name__ == "__main__":
    args = parser.parse_args()

    devices = {}
    if args.device:
        for device_dir in args.device:
            device_dir = pathlib.Path(device_dir)
            if not device_dir.is_dir():
                raise ValueError("-d accepts directories not files")

            logs = collect_logs(device_dir)
            devices[device_dir] = collect_all(logs)
    elif args.file:
        devices['all'] = collect_all(args.file)

    if args.standalone:
        with open(args.standalone_file, "w") as f:
            f.write("\\documentclass{standalone}\n")
            f.write("\\begin{document}\n")
            for d, (results, running) in devices.items():
                f.write(f"   \\textbf{{{d!s}}}\n")
                sio = io.StringIO()
                write_one_table(sio, d, results, running)
                for line in sio.getvalue().splitlines():
                    f.write(f"   {line}\n")
                f.write("   \\hspace{3cm}")
            f.write("\\end{document}\n")
    else:
        for d, (results, running) in devices.items():
            with open(f"results-{d!s}.tex", "w") as f:
                write_one_table(f, d, results, running)