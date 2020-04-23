""" tabular.py
Given 1 or more files containing the output of litmus.exe cat them together and build a LaTeX tabular
environment to embed into documents.

Usage:
    python tabular.py [IN...]

Example:
> ./litmus.exe @all -p -n1000 > out
> python3 tabular.py out
> cat results.tex
"""

import re
import sys
import math
import collections

results = collections.defaultdict(lambda: (0,0))
running = collections.defaultdict(list)
for fname in sys.argv[1:]:
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

with open("results.tex", "w") as f:
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
        f.write(f"{test_name} & {h_total_observations}/{h_total_runs}")
        if total_observations > 0:
            f.write(f"& {h_avg}/{h_run} & $\\pm$ {h_u}/{h_run} \\\\\n")
        else:
            f.write(f"& & \\\\\n")
    f.write("\\hline\n")
    f.write("\\end{tabular}\n")