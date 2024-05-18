use std::collections::{HashMap, HashSet};

use crate::error::Result;
use crate::litmus::{InitState, Litmus, MovSrc, Negatable, Reg};

const INCLUDES: &str = "#include \"lib.h\"";

/// Rename test for name of `litmus_test_t` object in final C code output.
fn sanitised_test_name(name: &str) -> String {
    name.chars()
        .filter_map(|c| match c {
            '+' | '-' | '.' => Some('_'),
            // '-' | '.' => None,
            'a'..='z' | 'A'..='Z' | '0'..='9' => Some(c),
            _ => {
                log::warn!("found unexpected char {c:?} in test name");
                Some('_')
            }
        })
        .collect()
}

/// Generate list of substitutions for a given thread. These are the lines after immediately after
/// blocks of assembly.
fn asm_subs_from_thread_reset(
    reset: HashMap<Reg, MovSrc>,
    thread_assert: &Option<Vec<(Reg, Negatable<MovSrc>)>>,
) -> Result<String> {
    let mut vas = HashSet::new();
    let mut ptes = HashSet::new();
    let mut pages = HashSet::new();
    let mut descs = HashSet::new();
    let mut pmds = HashSet::new();
    let mut puds = HashSet::new();
    let mut pmddescs = HashSet::new();
    let mut puddescs = HashSet::new();
    for (reg, val) in reset {
        if matches!(reg, Reg::Isla(_)) {
            continue;
        }
        match val {
            MovSrc::Nat(_) | MovSrc::Bin(_) | MovSrc::Hex(_) => {}
            MovSrc::Reg(var) => {
                vas.insert(var);
            }
            MovSrc::Page(var) => {
                pages.insert(var);
            }
            MovSrc::Pte(var, 1) => {
                puds.insert(var);
            }
            MovSrc::Pte(var, 2) => {
                pmds.insert(var);
            }
            MovSrc::Pte(var, _) => {
                ptes.insert(var);
            }
            MovSrc::Desc(var, 1) => {
                puddescs.insert(var);
            }
            MovSrc::Desc(var, 2) => {
                pmddescs.insert(var);
            }
            MovSrc::Desc(var, _) => {
                descs.insert(var);
            }
        }
    }

    fn to_comma_list(a: HashSet<String>) -> String {
        let mut v = a.iter().cloned().collect::<Vec<_>>();
        v.sort();
        v.join(", ")
    }

    let mut res = vec![];
    if !vas.is_empty() {
        res.push(format!("ASM_VAR_VAs(data, {})", to_comma_list(vas)));
    }
    if !ptes.is_empty() {
        res.push(format!("ASM_VAR_PTEs(data, {})", to_comma_list(ptes)));
    }
    if !pages.is_empty() {
        res.push(format!("ASM_VAR_PAGEs(data, {})", to_comma_list(pages)));
    }
    if !descs.is_empty() {
        res.push(format!("ASM_VAR_DESCs(data, {})", to_comma_list(descs)));
    }
    if !pmds.is_empty() {
        res.push(format!("ASM_VAR_PMDs(data, {})", to_comma_list(pmds)));
    }
    if !puds.is_empty() {
        res.push(format!("ASM_VAR_PUDs(data, {})", to_comma_list(puds)));
    }
    if !pmddescs.is_empty() {
        res.push(format!("ASM_VAR_PMDDESCs(data, {})", to_comma_list(pmddescs)));
    }
    if !puddescs.is_empty() {
        res.push(format!("ASM_VAR_PUDDESCs(data, {})", to_comma_list(puddescs)));
    }

    if let Some(asserts) = thread_assert {
        if !asserts.is_empty() {
            res.push("[tmpout] \"r\" (tmpout)".to_owned());
        }
    } else {
        res.push("ASM_REGS(data, REGS)".to_owned());
    }

    Ok(res.join(",\n    "))
}

/// Main output control flow. Attempt to convert [`Litmus`] test representation into a C code test
/// (encoded as a string).
pub fn write_output(litmus: Litmus, keep_histogram: bool) -> Result<String> {
    // println!("{litmus:#?}");
    let name = litmus.name;
    let sanitised_name = sanitised_test_name(&name) + "__toml";
    let vars = litmus.var_names.join(", ");
    let additional_vars = if litmus.additional_var_names.is_empty() {
        "".to_owned()
    } else {
        format!(", {}", litmus.additional_var_names.join(", "))
    };
    let regs =
        litmus.regs.iter().map(|(thread, reg)| format!("p{thread}{}", reg.as_asm())).collect::<Vec<_>>().join(", ");

    let thread_count = litmus.threads.len();
    let start_els = if thread_count == 1 {
        litmus.threads[0].el.to_string() + ","
    } else {
        litmus.threads.iter().map(|thread| thread.el.to_string()).collect::<Vec<_>>().join(",")
    };
    let mut handler_clobbers = HashMap::new();
    for t in &litmus.thread_sync_handlers {
        for (thread, _el) in &t.threads_els {
            if !handler_clobbers.contains_key(thread) {
                handler_clobbers.insert(*thread, HashSet::new());
            }
            let clobbers = handler_clobbers.get_mut(thread).unwrap();
            clobbers.extend(t.regs_clobber.clone());
        }
    }
    let thread_sync_handler_refs = {
        let mut handler_refs = vec![vec![None, None]; thread_count];
        for handler in &litmus.thread_sync_handlers {
            for (thread, el) in &handler.threads_els {
                handler_refs[*thread][*el as usize] = Some(&handler.name);
            }
        }
        let lines = handler_refs
            .into_iter()
            .map(|els| {
                els.into_iter()
                    .map(|name| name.map(|name| format!("(u32*){name}")).unwrap_or("NULL".to_owned()))
                    .collect::<Vec<_>>()
                    .join(", ")
            })
            .map(|inner| format!("(u32*[]){{{inner}}},"))
            .collect::<Vec<_>>()
            .join("\n    ");
        format!(
            ".thread_sync_handlers = (u32**[]){{\
            \n    {lines}\
            \n  }},\
            "
        )
    };
    let c_thread_sync_handlers: String = litmus
        .thread_sync_handlers
        .into_iter()
        .map(|handler| {
            let handler_name = handler.name;
            let thread_names = handler
                .threads_els
                .iter()
                .map(|(thread, el)| format!("{thread} (EL{el})"))
                .collect::<Vec<_>>()
                .join(", ");

            let body = handler
                .code
                .split('\n')
                .map(|ln| {
                    let trimmed = ln.trim();
                    if !trimmed.is_empty() {
                        format!("    \"{trimmed}\\n\\t\"\n")
                    } else {
                        "".to_string()
                    }
                })
                .collect::<Vec<String>>()
                .join("");
            let body = body.trim();

            format!(
                "// Thread sync handler for thread {thread_names}\
                \nstatic void {handler_name}(void) {{\
                \n  asm volatile (\
                \n    {body}\
                \n  );\
                \n}}\
                "
            )
        })
        .collect::<Vec<String>>()
        .join("\n\n");
    let c_threads: String = litmus
        .threads
        .into_iter()
        .map(|thread| {
            let thread_no: usize = thread.name.parse().unwrap(); // TODO: this should be error checked
            let thread_name = format!("P{thread_no}");

            let assert_regs_setup = thread
                .assert
                .as_ref()
                .and_then(|reg_val_pairs| {
                    let len = reg_val_pairs.len();
                    if len > 0 {
                        Some(format!("\n  u64 tmpout[32] = {{0}};"))
                    } else {
                        None
                    }
                })
                .unwrap_or_default();

            let reg_setup = if thread.reset.is_empty() {
                "".to_owned()
            } else {
                let reg_movs = thread
                    .reset
                    .iter()
                    .filter(|(reg, _val)| !matches!(reg, Reg::Isla(_)))
                    .map(|(reg, val)| format!("\"mov {}, {}\\n\\t\"", reg.as_asm(), val.as_asm()))
                    .collect::<Vec<_>>()
                    .join("\n    ");
                format!(
                    "\n    /* initial registers */\
                         \n    {reg_movs}\n"
                )
            };

            let output_var = if keep_histogram {
                let reg_strs = litmus
                    .regs
                    .iter()
                    .filter(|(thread_name, _)| thread_name.to_string() == thread.name)
                    .map(|(thread, reg)| format!("\"str {}, [%[{}]]\\n\\t\"", reg.as_asm(), reg.as_output_str(*thread)))
                    .collect::<Vec<_>>();
                if reg_strs.is_empty() {
                    "".to_string()
                } else {
                    format!(
                        "\n\n    /* output */\
                             \n    {}",
                        reg_strs.join("\n    ")
                    )
                }
            } else {
                let reg_strs = thread
                    .assert
                    .as_ref()
                    .unwrap()
                    .iter()
                    .map(|(reg, _)| format!("\"str {}, [%[tmpout],#{}]\\n\\t\"", reg.as_asm(), reg.idx() * 8))
                    .collect::<Vec<_>>();
                if reg_strs.is_empty() {
                    "".to_string()
                } else {
                    format!(
                        "\n\n    /* output */\
                             \n    {}",
                        reg_strs.join("\n    ")
                    )
                }
            };

            let body = thread
                .code
                .split('\n')
                .map(|ln| {
                    let trimmed = ln.trim();
                    if !trimmed.is_empty() {
                        format!("    \"{trimmed}\\n\\t\"\n")
                    } else {
                        "".to_string()
                    }
                })
                .collect::<Vec<String>>()
                .join("");
            let body = body.trim();

            let mut all_clobbers = thread.regs_clobber;
            if let Some(clobbers) = handler_clobbers.remove(&thread_no) {
                all_clobbers.extend(clobbers);
            }
            let all_clobbers_sorted = {
                let mut v = all_clobbers.into_iter().collect::<Vec<_>>();
                v.sort();
                v
            };
            let all_clobbers_asm = all_clobbers_sorted.iter().map(Reg::as_asm_quoted).collect::<Vec<_>>().join(", ");

            let asm_subs = asm_subs_from_thread_reset(thread.reset, &thread.assert).unwrap();

            let c_compiled_assert = thread
                .assert
                .map(|reg_val_pairs| {
                    if reg_val_pairs.is_empty() {
                        "".to_owned()
                    } else {
                        let regs = reg_val_pairs
                            .iter()
                            .map(|(reg, val)| {
                                let reg = reg.idx();
                                match val {
                                    Negatable::Eq(val) => format!("(tmpout[{reg}] == {})", val.as_c_code()),
                                    Negatable::Not(val) => format!("(tmpout[{reg}] != {})", val.as_c_code()),
                                }
                            })
                            .collect::<Vec<_>>()
                            .join(" & ");
                        let comment = "/* compile assertion into single register */";
                        format!("\n\n  {comment}\n  *out_reg(data, \"p{}:x0\") = {regs};", thread.name)
                    }
                })
                .unwrap_or_else(|| "".to_owned());

            format!(
                "static void {thread_name}(litmus_test_run* data) {{\
                    {assert_regs_setup}\
                \n  asm volatile (\
                      {reg_setup}\
                \n    /* test */\
                \n    {body}\
                      {output_var}\
                \n  :\
                \n  : {asm_subs}\
                \n  : \"cc\", \"memory\", {all_clobbers_asm}\
                \n  );\
                    {c_compiled_assert}\
                \n}}\
                "
            )
        })
        .collect::<Vec<String>>()
        .join("\n\n");

    let final_assertion_str = litmus.final_assertion_str;

    let init_state = if litmus.init_state.is_empty() {
        "".to_owned()
    } else {
        let len = litmus.init_state.len();
        let mut unmapped = vec![];
        let mut vars = vec![];
        let mut aliases = vec![];
        for state in &litmus.init_state {
            match state {
                InitState::Unmapped(var) => unmapped.push(format!("INIT_UNMAPPED({var})")),
                InitState::Var(var, val) => vars.push(format!("INIT_VAR({var}, {val})")),
                InitState::Alias(from, to) => aliases.push(format!("INIT_ALIAS({from}, {to})")),
            }
        }
        unmapped.sort();
        vars.sort();
        aliases.sort();
        let state_str = [unmapped, vars, aliases].into_iter().flatten().collect::<Vec<_>>().join(",\n    ");
        format!("\n    {len},\n    {state_str},\n  ")
    };

    let interesting_result = if keep_histogram {
        "".to_owned()
    } else {
        let v = vec!["1"; litmus.regs.len()].join(",");
        format!("\n  .interesting_result = (uint64_t[]){{{v}}},")
    };

    let requires_pgtable = if litmus.mmu_on { "1" } else { "0" };

    let out = format!(
        "\
// {name} [litmus-toml-translator]

{INCLUDES}

#define VARS {vars}
#define REGS {regs}

// Thread bodies
{c_threads}

{c_thread_sync_handlers}

// Final assertion
// {final_assertion_str}

// Final test struct
litmus_test_t {sanitised_name} = {{
  \"{name}\",
  MAKE_THREADS({thread_count}),
  MAKE_VARS(VARS{additional_vars}),
  MAKE_REGS(REGS),
  INIT_STATE({init_state}),\
  {interesting_result}
  {thread_sync_handler_refs}
  .requires_pgtable = {requires_pgtable},
  .start_els = (int[]){{{start_els}}},
}};
",
    );

    Ok(out)
}
