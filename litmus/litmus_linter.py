import re
import sys
import pathlib
import argparse
import collections

HANDLER_PATTERN = r"\s*\(uint32_t\*\[\]\)\{(\(uint32_t\*\))?(?P<name0>.+?),\s*(\(uint32_t\*\))?(?P<name1>.+?)\}"
LITMUS_FIELDS = {
    'handlers': (
        r"\.thread_sync_handlers\s*="
        r"\s*\(uint32_t\*\*\[\]\)\{"
        r"(?P<handlers>\s*(\s*\(uint32_t\*\[\]\)\{.+?\},?)+)"
        r"\s*\},"
    ),
}

LITMUS_TEST_T_PAT_OLD = (
    r"litmus_test_t\s+(?P<cname>.+?)\s*=\s*\{"
    r"\s*\"(?P<human_name>.+?)\"\s*,"
    r"\s*(?P<no_threads>\d+),\(th_f\*\[\]\)\{[\s\S]+\}\s*,"
    r'\s*(?P<no_vars>\d+),\(const char\*\[\]\){(?P<var_names>.+?)},'
    r'\s*(?P<no_regs>\d+),\(const char\*\[\]\){(?P<reg_names>.+?)},'
    r'(?P<fields>[\s\S]*?)'
    r'\s*\}\s*;'
)

LITMUS_TEST_T_PAT_NEW = (
    r"litmus_test_t\s+(?P<cname>.+?)\s*=\s*\{"
    r"\s*\"(?P<human_name>.+?)\"\s*,"
    r"\s*(?P<no_threads>\d+),\(th_f\*\[\]\)\{[\s\S]+\}\s*,"
    r'\s*MAKE_VARS\(VARS\)\s*?,'
    r'\s*MAKE_REGS\(REGS\)\s*?,'
    r'(?P<fields>[\s\S]*?)'
    r'\s*\}\s*;'
)

ASM_PAT = (
    r"asm\s*(volatile)?\s*\("
    r"(([^\)]*?)|([^/(]*?\([^\)]*?\)))*?"
    r"\s*[^/)]*?"
    r"\s*\);"
)

ASM_BLOCK_PAT = (
    r"asm\s*(volatile)?\s*\("
    r"\s*(?P<code>[^(]*?)"
    r"(\s*:(?P<outreg>[\s\S]+?)"
    r"\s*:(?P<inreg>[\s\S]+?)"
    r"\s*:(?P<clobber>[\s\S]+?))?"
    r"\s*\)\s*;"
)

C_FUN_PAT = (
    r"void (?P<fname>.+)\(.+\)\s*\{"
    r"(?P<body>[\s\S]*?)"
    r"\s*\}"
)

def get_reg_names(asm):
    if not asm:
        return []

    return ['x{}'.format(i) for i in re.findall(r'(?<=(?<!\%\[)[xw])\d+', asm['code'])]

def parse_litmus_code(path, c_code):
    match = re.search(LITMUS_TEST_T_PAT_NEW, c_code, re.MULTILINE)
    if not match:
        old_match = re.search(LITMUS_TEST_T_PAT_OLD, c_code, re.MULTILINE)
        if old_match:
            warn({'path': path}, "old-style litmus file")
        # let fallthrough to error
    handlers = re.search(LITMUS_FIELDS['handlers'], match['fields'], re.MULTILINE)
    if handlers is None:
        handlers = [('NULL','NULL')]*int(match['no_threads'])
    else:
        hs = handlers['handlers']
        handlers = []
        for h in re.finditer(HANDLER_PATTERN, hs, re.MULTILINE):
            handlers.append((h['name0'], h['name1']))

    funs = {m['fname']: m for m in re.finditer(C_FUN_PAT, c_code, re.MULTILINE)}

    def _get_asm(fn):
        asm_blk = re.search(ASM_PAT, funs[fn]['body'], re.MULTILINE)
        return re.match(ASM_BLOCK_PAT, asm_blk.group(0), re.MULTILINE)

    handler_fns = {
        f: _get_asm(f) if f != 'NULL' else None
        for (el0, el1) in handlers
        for f in (el0, el1)
    }

    thr_blocks = [(int(fun['fname'][1:]), fun) for fname, fun in funs.items() if re.fullmatch('P\d+', fname)]
    threads = [(i, _get_asm(m['fname']), handlers[i]) for (i, m) in thr_blocks]
    return {'path': path,
            'handlers': handler_fns,
            'cname': match['cname'],
            'human_name': match['human_name'],
            #'vars': (int(match['no_vars']), match['var_names']),
            #'regs': (int(match['no_regs']), match['reg_names']),
            'threads': (int(match['no_threads']), threads)}


def warn(litmus, msg):
    print(('! [{path}] Warning: %s' % msg).format(**litmus), file=sys.stderr)

def collect_litmuses(paths):
    for path in paths:
        with open(path) as f:
            try:
                litmus = parse_litmus_code(path, f.read())
            except Exception:
                pass
            else:
                yield litmus

# this function only checks the given litmus files
# not all of them!
def check_uniq_names(litmuses):
    uniqs = {}
    for l in litmuses:
        if l['cname'] in uniqs:
            warn(l, "Duplicate C identifier: {cname} in both {path} and %s." % uniqs[l['cname']])

        uniqs[l['cname']] = str(l['path'])


def check_human_match_path(l):
    p = pathlib.Path(l['path']).stem
    if l['human_name'] != str(p):
        warn(l, "File name and Litmus name mismatch.")


def check_thread_count(l):
    (count, threads) = l['threads']
    if (count != len(threads)):
        warn(l, "Thread count mismatch.")


def check_clobber_registers(l):
    (count, threads) = l['threads']
    for i, thr, (el0, el1) in threads:
        el0_asm = l['handlers'][el0]
        el1_asm = l['handlers'][el1]
        regs = get_reg_names(thr) + get_reg_names(el0_asm) + get_reg_names(el1_asm)
        for r in sorted(set(regs)):
            if thr['clobber'] and r not in thr['clobber']:
                warn(l, 'clobber missing register {r} in Thread {i}'.format(r=r, i=i))


def check_register_use(l):
    (count, threads) = l['threads']
    for i, thr, (el0, el1) in threads:
        el0_asm = l['handlers'][el0]
        el1_asm = l['handlers'][el1]
        regs = get_reg_names(thr) + get_reg_names(el0_asm) + get_reg_names(el1_asm)

        # ERET_TO_NEXT(xN)  then xN appears once but is not 'unused'
        el0_ret_reg = re.search(r'ERET_TO_NEXT\((.+)\)', el0_asm['code']) if el0_asm else None
        el1_ret_reg = re.search(r'ERET_TO_NEXT\((.+)\)', el1_asm['code']) if el1_asm else None
        counts = collections.Counter(regs)
        for r, c in counts.items():
            if c < 2:
                if (el0_ret_reg and r == el0_ret_reg.group(1)
                        or el1_ret_reg and r == el1_ret_reg.group(1)):
                    continue
                warn(l, 'register {r} in Thread {i} appears to be unused'.format(r=r, i=i))

def _lint(lit):
    check_human_match_path(lit)
    check_thread_count(lit)
    check_clobber_registers(lit)
    check_register_use(lit)


def _lint_many(lits):
    check_uniq_names(lits)

    for l in lits:
        _lint(l)


def lint_file(path):
    litmuses = list(collect_litmuses([path]))
    _lint_many(litmuses)


def lint_files(paths):
    collected_litmuses = list(collect_litmuses(paths))
    _lint_many(collected_litmuses)

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument('file', nargs='*')
    p.add_argument('-s', '--quiet', action='store_true')
    args = p.parse_args()
    lint_files(args.file)