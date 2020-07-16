import re
import sys
import pathlib
import collections

TEST_FILES = []
UPDATED_FILES = []
already_seen_files = set()
already_seen_tests = set()

Group = collections.namedtuple('Group', ['name', 'tests'])
Test = collections.namedtuple('Test', ['ident', 'name', 'matches'])
TestFile = collections.namedtuple('File', ['path', 'modified', 'test', 'groups'])


class GroupMap:
    def __init__(self, name=""):
        self.name = name
        self.groups = collections.defaultdict(GroupMap)
        self.tests = []

    def append(self, testfile, groups=None):
        groups = groups if groups is not None else testfile.groups
        if len(groups) > 0:
            first, *rest = groups
            if first == self.name:
                self.append(testfile, groups=rest)
            else:
                self.groups[first].name = first
                self.groups[first].append(testfile, groups=rest)
        else:
            self.tests.append(testfile)

    def group(self, group_name):
        self.groups[group_name].name = group_name
        return self.groups[group_name]

    def flat(self):
        yield from self.tests
        for g in self.groups.values():
            yield from g.flat()

    def flat_with_groups(self, prefix=[]):
        for t in self.tests:
            yield (prefix+[self.name], t)
        for g in self.groups.values():
            yield from g.flat_with_groups(prefix=prefix+[self.name])

    def find(self, tfile):
        cur = self

        for g in tfile.groups:
            cur = cur.group(g)

        for t in cur.tests:
            if t.test.ident == tfile.test.ident:
                return t

        return None

    def contains_test(self, tfile):
        return self.find(tfile) is not None

    def updated_test(self, tfile):
        t = self.find(tfile)
        if t is None:
            return True

        if t.modified != tfile.modified:
            return True

        return False

    def is_empty(self):
        return len(self.tests) == 0 and all(g.is_empty() for g in self.groups.values())

    def __repr__(self):
        return "GroupMap(name={!r}, tests={}, groups={})".format(self.name, self.tests, self.groups)

class TestGroups:
    def __init__(self, root, force, includes):
        self.root = root
        self.force = force
        self.includes = includes

        self.all_tests = GroupMap("all")
        self.already_seen = GroupMap("all")
        self.matching_tests = GroupMap("all")
        self.updated_tests = GroupMap("all")

    def updated_groups(self):
        return not self.updated_tests.is_empty() or self.force or not (root / 'groups.c').exists()

    def matches(self, name, prefix, extra_includes=[]):
        """returns True if some test name matches the set of included tests
        """
        includes = set().union(self.includes, extra_includes)
        return ((name in includes or '@all' in includes) and not any('-@'+p in includes for p in prefix)
                and not '-'+name in includes)

    def read_previous_tests(self):
        if not self.force:
            try:
                with open(self.root / 'group_list.txt', 'r') as f:
                    header = f.read()
                    self.includes = set(header.split())
            except FileNotFoundError:
                self.includes = {"@all"}
                if not quiet:
                    sys.stderr.write("   (COLLECT) defaulting to @all\n")

        try:
            o = open(self.root / 'test_list.txt', 'r')
        except FileNotFoundError:
            return

        with o as f:
            for line in f:
                try:
                    include, file_path, last_modified_timestamp, test_ident, test_name, *groups = line.split()
                except ValueError:
                    continue
                else:
                    matches = True if include == '1' else False
                    self.already_seen.append(TestFile(file_path, last_modified_timestamp, Test(test_ident, test_name, matches), groups))


    def find_tests_in_file(self, path, groups=[], extra_includes=[]):
        """find the test (if it exists) in the .c file given by path
        """
        found_match = False

        st = path.stat()
        with open(path, "r") as f:
            for line in f:
                if re.match(r'litmus_test_t .+\s*=\s*{\s*', line):
                    # litmus_test_t name = {
                    _, __, rem = line.partition(" ")
                    cname, _, __ = rem.partition("=")

                    # try find test name to see if it should be excluded
                    # this is pretty horrible and assumes the test name is the first "double quoted string"
                    # on one of the successor lines
                    while True:
                        line = next(f)
                        if '"' in line:
                            _, _, rem = line.partition('"')
                            testname, _, _ = rem.partition('"')
                            break

                    if found_match:
                        sys.stderr.write('File {path}: multiple tests encountered -- only 1 test per file allowed.\n')
                        sys.exit(1)

                    found_match = True
                    matches = self.matches(testname, groups, extra_includes=extra_includes)
                    tfile = TestFile(path, str(st.st_mtime), Test(cname.strip(), testname, matches), groups)
                    self.all_tests.append(tfile)

                    if matches:
                        self.matching_tests.append(tfile)

                        if self.already_seen.updated_test(tfile):
                            self.updated_tests.append(tfile)
                            if not quiet:
                                sys.stderr.write('   (COLLECT) {}\n'.format(tfile.path))

    def get_tests(self, d, extra_includes=[], groups=[]):
        """recursively find all tests in a directory that match
        """
        for f in d.iterdir():
            if f.is_dir():
                # if this *group* matches then match all tests recursively down
                grp = f.stem
                self.get_tests(
                    f,
                    groups=groups+[grp],
                    extra_includes=(['@all'] if self.matches('@'+f.name, groups) else []),
                )
            elif f.suffix == '.c':
                self.find_tests_in_file(f, groups=groups, extra_includes=extra_includes)

group_template = """
const litmus_test_group grp_%s = {
  .name="@%s",
  .tests = (const litmus_test_t*[]){
    %s
  },
  .groups = (const litmus_test_group*[]){
    %s
  },
};\
"""


def build_group_defs(matching):
    for g in matching.groups.values():
        yield from build_group_defs(g)

    name = matching.name
    test_refs = sorted('&{}'.format(t.test.ident) for t in matching.tests)
    grp_refs = sorted('&grp_{}'.format(grp_name) for grp_name in matching.groups)
    test_refs.append('NULL')
    grp_refs.append('NULL')
    yield group_template % (name, name, ',\n    '.join(test_refs), ',\n    '.join(grp_refs))


def build_externs(matching):
    all = sorted(t.test.ident for t in matching.flat())
    externs = ',\n  '.join(all)
    return 'extern litmus_test_t\n  {};'.format(externs)

code_template="""\
/************************
 *  AUTOGENERATED FILE  *
 *     DO NOT EDIT      *
 ************************/

 /* this file was generated with the following command:
  * `make LITMUS_TESTS='{includes}'`
  *
  * please re-run that command to re-generate this file
  */
#include "lib.h"

%s

%s
"""

def build_code(includes, matching):
    extern_line = build_externs(matching)
    litmus_group_defs = build_group_defs(matching)
    return code_template.format(includes=' '.join(includes)) % (extern_line, '\n'.join(litmus_group_defs))

def build_test_grp_list(split_groups):
    all = sorted(all_tests(split_groups), key=lambda t: (t[1], t[0]))
    return '\n'.join('{} @all @{}'.format(t.name, ' @'.join(grps)) for (t, grps) in all)

def write_groups_c(tg):
    with open(tg.root / 'groups.c', 'w') as f:
        f.write(build_code(tg.includes, tg.matching_tests))

def write_group_list_txt(tg):
    with open(tg.root / 'group_list.txt', 'w') as f:
        f.write(' '.join(tg.includes) + '\n')

def write_test_list_txt(tg):
    """writes litmus/test_list.txt
    """
    with open(tg.root / 'test_list.txt', 'w') as f:
        for (grps, tfile) in tg.all_tests.flat_with_groups():
            fname = tfile.path
            ident = tfile.test.ident
            test_name = tfile.test.name
            modified = tfile.modified
            groups = ' '.join(grps)
            include = '1' if tfile.test.matches else '0'
            f.write(f"{include} {fname} {modified} {ident} {test_name} {groups}\n")

if __name__ == "__main__":
    root = pathlib.Path(__file__).parent

    quiet = int(sys.argv[1])
    includes = set(sys.argv[2:])
    force = bool(includes)

    tg = TestGroups(root, force, includes)
    tg.read_previous_tests()
    tg.get_tests(root / 'litmus_tests/')
    if tg.updated_groups():
        write_groups_c(tg)
    write_group_list_txt(tg)
    write_test_list_txt(tg)