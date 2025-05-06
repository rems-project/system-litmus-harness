import re
import sys
import pathlib
import argparse
import collections

HANDLER_PATTERN = r"\s*\(u32\s*\*\s*\[\]\)\{\s*(\(u32\*\))?(?P<name0>.+?)\s*,\s*(\(u32\s*\*\))?(?P<name1>.+?)\s*\}"
LITMUS_FIELDS = {
    'handlers': (
        r"\.thread_sync_handlers\s*="
        r"\s*\(u32\s*\*\s*\*\s*\[\s*\]\s*\)\s*\{"
        r"(?P<handlers>\s*(\s*\(\s*u32\*\[\s*\]\s*\)\s*\{.+?\},?)+)"
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
    r'\s*MAKE_THREADS\((?P<no_threads>\d+)\)\s*?,'
    r'\s*MAKE_VARS\(VARS(\s*,\s*pa\d+)*\)\s*?,'
    r'\s*MAKE_REGS\(REGS\)\s*?,'
    r'(?P<fields>[\s\S]*?)'
    r'\s*\}\s*;'
)

ASM_PAT = (
    r"asm\s*(volatile)?\s*\("
    r"[^;]*?"
    r"\s*\);"
)

ASM_BLOCK_PAT = (
    r"asm\s*(volatile)?\s*\("
    r"\s*(?P<code>[^;]*)"
    r"(\s*:(?P<outreg>[\s\S]+?)"
    r"\s*:(?P<inreg>[\s\S]+?)"
    r"\s*:(?P<clobber>[\s\S]+?))"
    r"\s*\)\s*;"
)

ASM_BLOCK_PAT_NO_VARS = (
    r"asm\s*(volatile)?\s*\("
    r"\s*(?P<code>[^;]*)"
    r"\s*\)\s*;"
)

C_FUN_PAT = (
    r"void (?P<fname>.+)\(.+\)\s*\{"
    r"(?P<body>[\s\S]*?)"
    r"^}"
)

C_INIT_ST = (
    r'INIT_STATE\s*\('
    r'\s*(?P<no_init_states>\d+)\s*,'
    r'\s*(?P<body>'
    r'([^\(]*?|([^\(]*?\([^\)]*?\)))*?'
    r')'
    r'\s*\)'
)

def get_reg_names(asm):
    if not asm:
        return []
    return ['x{}'.format(i) for i in re.findall(r'(?<=(?<!.0|\%\[)[xwXW])\d+', asm['code'])]

def parse_litmus_code(path, c_code):
    match = re.search(LITMUS_TEST_T_PAT_NEW, c_code, re.MULTILINE)
    if not match:
        old_match = re.search(LITMUS_TEST_T_PAT_OLD, c_code, re.MULTILINE)
        if old_match and args.warn_old_style:
            warn({'path': path}, "old-style litmus file")
        # let fallthrough to error

    if 'handlers' in LITMUS_FIELDS:
        handlers = re.search(LITMUS_FIELDS['handlers'], match['fields'], re.MULTILINE)
    else:
        handlers = None

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
        asm = asm_blk.group(0)
        return (re.match(ASM_BLOCK_PAT, asm, re.MULTILINE)
                or re.match(ASM_BLOCK_PAT_NO_VARS, asm, re.MULTILINE))

    handler_fns = {
        f: _get_asm(f) if f != 'NULL' else None
        for (el0, el1) in handlers
        for f in (el0, el1)
    }

    init_st = re.search(C_INIT_ST, c_code, re.MULTILINE)
    thr_blocks = [(int(fun['fname'][1:]), fun) for fname, fun in funs.items() if re.fullmatch(r'P\d+', fname)]
    threads = [(i, _get_asm(m['fname']), handlers[i]) for (i, m) in thr_blocks]

    return {
        'path': path,
        'handlers': handler_fns,
        'handler_fields': handlers,
        'cname': match['cname'],
        'human_name': match['human_name'],
        'init': init_st,
        #'vars': (int(match['no_vars']), match['var_names']),
        #'regs': (int(match['no_regs']), match['reg_names']),
        'threads': (int(match['no_threads']), threads),
        'c_code': c_code,
    }


def warn(litmus, msg):
    print(('! [{path}] Warning: %s' % msg).format(**litmus), file=sys.stderr)

def collect_litmuses(paths):
    for path in paths:
        with open(path) as f:
            try:
                litmus = parse_litmus_code(path, f.read())
            except Exception:
                if args.debug:
                    raise SyntaxError(path)
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

    # for toml-translator generated names, strip the .toml.c
    if p.endswith(".litmus.toml"):
        p = p[:-len(".litmus.toml")]
        pass

    if l['human_name'] != p:
        warn(l, "File name and Litmus name mismatch.")


def check_thread_count(l):
    (count, threads) = l['threads']
    if (count != len(threads)):
        warn(l, "Thread count mismatch.")

    if 'handler_fields' in l and len(l['handler_fields']) != len(threads):
        warn(l, "Thread count does not match number of exception handler entries.")


def check_clobber_registers(l):
    (count, threads) = l['threads']
    for i, thr, (el0, el1) in threads:
        el0_asm = l['handlers'][el0]
        el1_asm = l['handlers'][el1]
        regs = get_reg_names(thr) + get_reg_names(el0_asm) + get_reg_names(el1_asm)
        clobbers = re.findall('"(.+?)"', thr['clobber']) if thr['clobber'] else []
        for r in sorted(set(regs)):
            if r not in clobbers:
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

def _check_asm_start_end(l, name, block):
    if block is None:
        return

    if (   "LITMUS_START_ASM" not in block["code"]
        or "LITMUS_END_ASM"   not in block["code"]
    ):
        warn(l, "asm block in {} missing LITMUS_START_ASM/LITMUS_END_ASM markers.".format(name))

def check_asm_start_end(l):
    (count, threads) = l['threads']
    for i, thr, (el0, el1) in threads:
        el0_asm = l['handlers'][el0]
        el1_asm = l['handlers'][el1]
        _check_asm_start_end(l, "Thread {}".format(i), thr)
        _check_asm_start_end(l, "Thread {} EL0 Handler".format(i), el0_asm)
        _check_asm_start_end(l, "Thread {} EL1 Handler".format(i), el1_asm)


def check_init_count(lit):
    init = lit['init']
    count = int(init['no_init_states'])
    init_body = init['body']
    commas = 0
    b = 0
    body = init_body.strip()
    for i, c in enumerate(body):
        if c == '(':
            b += 1
        elif c == ')':
            b -= 1
        elif c == ',' and b == 0 and i != len(body) - 1:
            commas += 1
    no_sts = commas + 1
    if no_sts != count:
        warn(lit, 'initial state contains {no_sts} states but INIT_STATE arguments claim it has {n} states'.format(
            no_sts=no_sts,
            n=count,
        ))


def check_requires_pgtable(lit):
    code = lit['c_code']

    req_pgtable_re = r'\.requires\s*=\s*REQUIRES_PGTABLE'
    m_req_pgtable = re.search(req_pgtable_re, code, re.MULTILINE)

    # look for things that look like it requires the MMU enabled:
    mmu_enable_check_re = (
        r'(tlbi)'
        r'|(INIT\_UNMAPPED)'
        r'|(INIT\_PTE)'
        r'|(\%\[\w+desc\])'
    )

    m_mmu_enable_check = re.search(mmu_enable_check_re, code, re.MULTILINE)

    if m_mmu_enable_check and not m_req_pgtable:
        warn(lit, 'missing requires=REQUIRES_PGTABLE?')

def run_lint(linter, litmus):
    if linter.__name__ in args.excl:
        return

    try:
        linter(litmus)
    except Exception:
        if args.debug:
            raise

def _lint(lit):
    run_lint(check_human_match_path, lit)
    run_lint(check_thread_count, lit)
    run_lint(check_clobber_registers, lit)
    run_lint(check_register_use, lit)
    run_lint(check_asm_start_end, lit)
    run_lint(check_init_count, lit)
    run_lint(check_requires_pgtable, lit)


def _lint_many(lits):
    run_lint(check_uniq_names, lits)

    for l in lits:
        if args.verbose or args.debug:
            print('[{}]'.format(l['path']))
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
    p.add_argument('--warn-old-style', action='store_true')
    p.add_argument('-d', '--debug', action='store_true')
    p.add_argument('-v', '--verbose', action='store_true')
    p.add_argument('-e', '--excl', action='append', default=[])
    args = p.parse_args()

    try:
        lint_files(args.file)
    except Exception as e:
        # dump the traceback to stderr, but compress it as they can get big and noisy
        if args.debug:
            raise

        import traceback
        import base64
        import gzip
        tb = traceback.format_exc().encode()
        bs = base64.b64encode(gzip.compress(tb))
        sys.stderr.write('! linter exc: {!r}\n'.format(bs.decode()))