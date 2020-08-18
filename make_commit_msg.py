import sys
import pathlib
import datetime
import subprocess

curr_dir = pathlib.Path(".")

BRANCH = sys.argv[1]

ls_files = subprocess.run(['git', 'ls-files', '--with-tree', BRANCH], stdout=subprocess.PIPE).stdout.decode('utf-8').splitlines()

all_commits = subprocess.run(['git', 'rev-list', BRANCH], stdout=subprocess.PIPE).stdout.decode('utf-8').splitlines()

source_dir = pathlib.Path("./doc/source")
if not source_dir.exists():
    sys.exit(1)

try:
    with open(".commits", "r") as f:
        remembered_commits = f.read().splitlines()
except FileNotFoundError:
    remembered_commits = []

def cut_commits(list_of_commits):
    for c in list_of_commits:
        if c not in remembered_commits:
            yield c

file_commits = {}
def walk_source(parent: pathlib.Path):
    for f in parent.iterdir():
        if f.is_file() and str(f) in ls_files:
            commits = subprocess.run(['git', 'rev-list', BRANCH, str(f)], stdout=subprocess.PIPE).stdout.decode('utf-8').splitlines()
            file_commits[str(f)] = list(cut_commits(commits))
        elif f.is_dir():
            walk_source(f)

walk_source(source_dir)
all_new_commits = sorted({c for cs in file_commits.values() for c in cs}, key=lambda c: all_commits.index(c))

def line_from_commit(c):
    out = subprocess.run(['git', 'show', c], stdout=subprocess.PIPE).stdout.decode('utf-8').splitlines()
    author = out[1].partition("Author: ")[2].partition("<")[0].strip()
    first_line = out[4].strip()
    short_commit = c[:7]

    return "({}) <{}>  {}".format(short_commit, author, first_line)

commit_lines = "\n".join(line_from_commit(c) for c in all_new_commits)
with open(".commits", "w") as f:
    f.write("\n".join(remembered_commits))
    f.write("\n".join(all_new_commits))

new_commit_msg = """\
make publish -- {}

{}""".format(datetime.datetime.now(datetime.timezone.utc).isoformat(), commit_lines)

print(new_commit_msg)