//! parse.rs contains the main parsing functionality for `litmus-toml-translator`.
//!
//! The purpose of these functions is to translate a string of toml code into a [`Litmus`] object
//! which encodes everything necessary to generate an equivalent C test for use by
//! `system-litmus-harness`.

use std::collections::{BTreeSet, HashMap};

use isla_axiomatic::litmus::exp::{Exp, Loc};
use isla_axiomatic::litmus::{self as axiomatic_litmus, exp_lexer, exp_parser};
use isla_axiomatic::page_table;
use isla_lib::bitvector::{b64::B64, BV};
use isla_lib::config::ISAConfig;
use isla_lib::ir::Symtab;
use isla_lib::zencode;
use toml::Value;

use crate::arch::ParsedArchitecture;
use crate::error::{Error, Result};
use crate::litmus::{self, InitState, Litmus, MovSrc, Negatable, Reg, Thread, ThreadSyncHandler};

/// Tallied results for many translations for statistics.
#[derive(Debug, Default)]
pub struct TranslationResults {
    pub succeeded: usize,
    pub unsupported: usize,
    pub skipped: usize,
    pub failed: usize,
}

impl TranslationResults {
    pub fn total(&self) -> usize {
        self.succeeded + self.unsupported + self.skipped + self.failed
    }

    pub fn percentage_succeeded(&self) -> f64 {
        let f = self.succeeded as f64 / self.total() as f64;
        (f * 1000.).round() / 10.
    }
}

/// Parse toml [`Value`] into a [`MovSrc`].
fn parse_reset_val(unparsed_val: &Value, symtab: &Symtab) -> Result<MovSrc> {
    let parsed = &axiomatic_litmus::parse_reset_value(unparsed_val, symtab)
        .map_err(|e| Error::ParseResetValue(e.to_string()))?;

    parsed.try_into()
}

/// Parse a list of resets into a mapping of registers to values.
fn parse_resets(unparsed_resets: Option<&Value>, symtab: &Symtab) -> Result<HashMap<Reg, MovSrc>> {
    if let Some(unparsed_resets) = unparsed_resets {
        unparsed_resets
            .as_table()
            .ok_or_else(|| "Thread init/reset must be a list of register name/value pairs".to_string())
            .map_err(Error::ParseResetValue)?
            .into_iter()
            .map(|(reg, val)| {
                let reg = litmus::parse_reg_from_str(reg)?;
                match reg {
                    // Isla-specific values can be more difficult to parse. We dump into a Reg type for now.
                    Reg::Isla(..) => Ok((reg, MovSrc::Reg(val.to_string()))),
                    _ => Ok((reg, parse_reset_val(val, symtab)?)),
                }
            })
            .collect()
    } else {
        Ok(HashMap::new())
    }
}

/// Merge a list of inits with a list of resets. This is necessary because of how Isla allows for
/// both but they perform the same function in `system-litmus-harness`.
fn merge_inits_resets(
    inits: HashMap<Reg, MovSrc>,
    resets: HashMap<Reg, MovSrc>,
) -> Result<(HashMap<Reg, MovSrc>, HashMap<Reg, MovSrc>)> {
    let mut gp = HashMap::with_capacity(inits.len() + resets.len());
    let mut special = HashMap::new();
    for (reg, val) in inits.into_iter().chain(resets) {
        match reg {
            // TODO: not all special registers are PSTATEs or VBAR
            Reg::PState(_) => special.insert(reg, val),
            Reg::VBar(_) => special.insert(reg, val),
            _ => gp.insert(reg, val),
        };
    }
    Ok((gp, special))
}

/// Parse a thread toml [`Value`] into a [`Thread`] type for use in the main [`Litmus`] struct.
fn parse_thread(
    thread_name: &str,
    thread: &Value,
    final_assertion: Exp<String>,
    keep_histogram: bool,
    symtab: &Symtab,
) -> Result<Thread> {
    let (code, regs_clobber) = match thread.get("code") {
        Some(code) => code
            .as_str()
            .map(|code| {
                let regs_clobber = litmus::parse_regs_from_asm(code)?;
                Ok((code, regs_clobber))
            })
            .ok_or_else(|| Error::ParseThread("thread code must be a string".to_string()))?,
        None => match thread.get("call") {
            Some(_call) => {
                // This is left unimplemented for now since almost no tests use this syntax.
                unimplemented!()
            }
            None => Err(Error::ParseThread(format!("No code or call found for thread {}", thread_name)))?,
        },
    }?;

    let assert_conj_of_regs = if keep_histogram {
        None
    } else {
        let reg_val_pairs = parse_assertion_into_conjunction_of_regs(symtab, final_assertion)?;
        let filtered = reg_val_pairs
            .into_iter()
            .filter_map(|((thread, reg), val)| if thread.to_string() == thread_name { Some((reg, val)) } else { None })
            .collect();
        Some(filtered)
    };

    let inits = parse_resets(thread.get("init"), symtab)?;
    let resets = parse_resets(thread.get("reset"), symtab)?;
    let (merged_resets, special_resets) = merge_inits_resets(inits, resets)?;

    let el = {
        let el_src = special_resets.get(&Reg::PState("PSTATE.EL".to_owned()));

        if let Some(MovSrc::Reg(r)) = el_src {
            return Err(Error::ParseThread(format!("Invalid EL level ({r})")));
        }

        let el_u8 = el_src.map(|mov_src| mov_src.bits().lower_u8()).unwrap_or(B64::new(0, 64).lower_u8()); // By default EL = 0

        if el_u8 > 1 {
            log::warn!("EL > 1 not allowed");
        }
        el_u8
    };

    let vbar_el1 = special_resets.get(&Reg::VBar("VBAR_EL1".to_owned())).map(MovSrc::bits).copied();

    // Harness doesn't support EL2.
    if special_resets.contains_key(&Reg::VBar("VBAR_EL2".to_owned())) {
        return Err(Error::Unsupported("Harness does not support handling of EL2 exceptions.".to_owned()));
    }

    Ok(Thread {
        name: thread_name.to_owned(),
        code: code.to_owned(),
        el,
        reset: merged_resets.into_iter().map(|(reg, val)| (reg, val.to_owned())).collect(),
        assert: assert_conj_of_regs,
        regs_clobber,
        vbar_el1,
    })
}

/// Take 'section' toml [`Value`] and translate into a [`ThreadSyncHandler`] type.
fn parse_thread_sync_handler_from_section(
    handler_name: &str,
    handler: &Value,
    threads: &[Thread],
    symtab: &Symtab,
) -> Result<Option<ThreadSyncHandler>> {
    if let Some(address) = handler.get("address") {
        let address = parse_reset_val(address, symtab)?; // TODO: this is probably the wrong
                                                         // parsing fn to use.
        let address = address.bits();
        fn el_from_vec_offset(address: B64, vbar: B64) -> Option<u8> {
            let offset = (address - vbar).lower_u64();
            if offset == 0 || offset == 0x200 {
                Some(1)
            } else if offset == 0x400 || offset == 0x600 {
                Some(0)
            } else {
                None
            }
        }
        let threads_els = threads
            .iter()
            .filter_map(|t| {
                t.vbar_el1.and_then(|vbar| el_from_vec_offset(*address, vbar)).map(|el| (t.name.parse().unwrap(), el))
            })
            .collect::<Vec<_>>();

        if let Some((_, el)) = threads_els.iter().find(|(_, el)| *el != 0 && *el != 1) {
            return Err(Error::Unsupported(format!("unsupported exception level {el}")));
        }

        let code = handler
            .get("code")
            .ok_or_else(|| Error::ParseThread(format!("No code or call found for thread {}", handler_name)))
            .map(|code| code.as_str())
            .and_then(|code| code.ok_or_else(|| Error::ParseThread("thread code must be a string".to_string())))?;

        let regs_clobber = litmus::parse_regs_from_asm(code)?;

        Ok(Some(ThreadSyncHandler { name: handler_name.to_owned(), code: code.to_owned(), regs_clobber, threads_els }))
    } else {
        // Can't be a functioning thread sync handler.
        Ok(None)
    }
}

/// Extract list of registers from final assertion (used when keeping histogram).
fn regs_from_final_assertion(symtab: &Symtab, final_assertion: Exp<String>) -> Result<BTreeSet<(u8, Reg)>> {
    fn extract_regs_from_exp(symtab: &Symtab, set: &mut BTreeSet<(u8, Reg)>, exp: Exp<String>) -> Result<()> {
        match exp {
            Exp::EqLoc(Loc::Register { reg, thread_id }, _exp) => {
                let thread_id = thread_id.try_into().map_err(|e| Error::ParseThread(format!("{e}")))?;
                set.insert((thread_id, litmus::parse_reg_from_str(&zencode::decode(symtab.to_str(reg)))?));
                Ok(())
            }
            Exp::Not(exp) => extract_regs_from_exp(symtab, set, *exp),
            Exp::And(exps) | Exp::Or(exps) => {
                for exp in exps {
                    extract_regs_from_exp(symtab, set, exp)?;
                }
                Ok(())
            }
            Exp::Implies(exp1, exp2) => {
                extract_regs_from_exp(symtab, set, *exp1)?;
                extract_regs_from_exp(symtab, set, *exp2)?;
                Ok(())
            }
            // TODO: Currently ignoring function application
            exp => {
                Err(Error::Unsupported(format!("Can't yet use final assertions with complex terms {exp:?}")))
            }
            // Exp::Loc(A),
            // Exp::Label(String),
            // Exp::True,
            // Exp::False,
            // Exp::Bin(String),
            // Exp::Hex(String),
            // Exp::Bits64(u64, u32),
            // Exp::Nat(u64),
            // Exp::App(String, Vec<Exp<A>>, HashMap<String, Exp<A>>),
        }
    }
    let mut set = BTreeSet::new();
    extract_regs_from_exp(symtab, &mut set, final_assertion)?;
    Ok(set)
}

/// Check if an assertion is allowed (only conjunctions translatable if not keeping histogram) and
/// convert into a list of register-value pairs
///
/// For example, `1:X0=1 & ~1:X2=0` goes to `[((1, X(0)), Eq(1)), ((1, X(2)), Not(0))]`.
fn parse_assertion_into_conjunction_of_regs(
    symtab: &Symtab,
    final_assertion: Exp<String>,
) -> Result<Vec<((u8, Reg), Negatable<MovSrc>)>> {
    fn extract_regs_from_exp(
        symtab: &Symtab,
        reg_val_pairs: &mut HashMap<(u8, Reg), Negatable<MovSrc>>,
        not: bool,
        exp: Exp<String>,
    ) -> Result<()> {
        match exp {
            Exp::EqLoc(Loc::Register { reg, thread_id }, exp) => {
                let thread_id = thread_id.try_into().map_err(|e| Error::ParseThread(format!("{e}")))?;
                let reg = litmus::parse_reg_from_str(&zencode::decode(symtab.to_str(reg)))?;
                let val: MovSrc = exp.as_ref().try_into().unwrap();
                if not {
                    reg_val_pairs.insert((thread_id, reg), Negatable::Not(val));
                } else {
                    reg_val_pairs.insert((thread_id, reg), Negatable::Eq(val));
                }
                Ok(())
            }
            Exp::Not(exp) => {
                extract_regs_from_exp(symtab, reg_val_pairs, !not, *exp)
            }
            Exp::And(exps) => {
                for exp in exps {
                    extract_regs_from_exp(symtab, reg_val_pairs, not, exp)?;
                }
                Ok(())
            }
            // Exp::Implies(exp1, exp2) => {
            //     extract_regs_from_exp(symtab, set, *exp1)?;
            //     extract_regs_from_exp(symtab, set, *exp2)?;
            //     Ok(())
            // }
            Exp::Or(exps) => {
                Err(Error::Unsupported(format!("Can't yet use '|' (or) in assertion ({exps:?})")))
            }
            // TODO: Currently ignoring function application
            exp => {
                Err(Error::Unsupported(format!("Can't yet use final assertions with complex terms {exp:?}")))
            }
            // Exp::Loc(A),
            // Exp::Label(String),
            // Exp::True,
            // Exp::False,
            // Exp::Bin(String),
            // Exp::Hex(String),
            // Exp::Bits64(u64, u32),
            // Exp::Nat(u64),
            // Exp::App(String, Vec<Exp<A>>, HashMap<String, Exp<A>>),
        }
    }
    let mut reg_val_pairs = HashMap::new();
    extract_regs_from_exp(symtab, &mut reg_val_pairs, false, final_assertion)?;
    Ok(reg_val_pairs.into_iter().collect())
}

/// Get any additional variable names from the page table setup of a toml test.
fn get_additional_vars_from_pts(
    page_table_setup: &Vec<page_table::setup::Constraint>,
    existing_vars: Vec<String>,
) -> Result<Vec<String>> {
    use page_table::setup::{AddressConstraint::*, Constraint::*, Exp, TableConstraint::*};
    let existing_vars: BTreeSet<String> = existing_vars.into_iter().collect();
    let mut all_vars = BTreeSet::new();
    for constraint in page_table_setup {
        match constraint {
            Initial(Exp::Id(id), _) => {
                all_vars.insert(id.clone());
            }
            Table(MapsTo(Exp::Id(from), Exp::Id(to), _, lvl, _)) => {
                if *lvl != 3 {
                    return Err(Error::Unsupported("intermediate addresses are not supported".to_owned()));
                }
                all_vars.insert(from.clone());
                if to != "invalid" {
                    all_vars.insert(to.clone());
                }
            }
            Address(Physical(_, ps)) => {
                // TODO: we should take into account region ownership/pinning.
                for p in ps {
                    all_vars.insert(p.clone());
                }
            }
            Address(Virtual(_, vs)) => {
                for v in vs {
                    all_vars.insert(v.clone());
                }
            }
            Address(Intermediate(..)) => {
                // TODO: we should allow this if var is never used.
                return Err(Error::Unsupported("intermediate addresses are not supported".to_owned()));
            }
            Address(Function(f, ..), ..) if f == "PAGE" || f == "PAGEOFF" => {}
            e => log::warn!("Ignoring page table constraint {e:?}"),
        };
    }

    Ok(all_vars.difference(&existing_vars).cloned().collect())
}

/// Main parsing function taking a raw string and attempting to a return a [`Litmus`]
/// representation of the same test.
pub fn parse(ir: &ParsedArchitecture<B64>, isa: &ISAConfig<B64>, contents: &str, keep_histogram: bool) -> Result<Litmus> {
    let litmus_toml = contents.parse::<Value>().map_err(Error::ParseToml)?;

    let mmu_on = litmus_toml.get("page_table_setup").is_some();
    // static ARCH: Lazy<DeserializedArchitecture<B64>> = Lazy::new(|| arch::load_aarch64_config_irx().unwrap());
    // let (isa, symtab) = arch::load_aarch64_isa(&ARCH, mmu_on)?;
    let symtab = &ir.symtab;

    let arch =
        litmus_toml.get("arch").and_then(|n| n.as_str().map(str::to_string)).unwrap_or_else(|| "unknown".to_string());

    let name = litmus_toml
        .get("name")
        .and_then(|n| n.as_str().map(str::to_string))
        .ok_or_else(|| Error::GetTomlValue("No name found in litmus file".to_owned()))?;

    let symbolic = litmus_toml
        .get("symbolic")
        .or(litmus_toml.get("addresses"))
        .and_then(Value::as_array)
        .ok_or_else(|| Error::GetTomlValue("No symbolic addresses found in litmus file".to_owned()))?;

    let var_names: Vec<String> = symbolic.iter().map(Value::as_str).map(|v| v.unwrap().to_owned()).collect();

    let (page_table_setup, page_table_setup_str) = if let Some(setup) = litmus_toml.get("page_table_setup") {
        if litmus_toml.get("locations").is_some() {
            return Err(Error::GetTomlValue(
                "Cannot have a page_table_setup and locations in the same test".to_owned(),
            ));
        }
        if let Some(litmus_setup) = setup.as_str() {
            let setup = format!("{}{}", isa.default_page_table_setup, litmus_setup);
            let lexer = page_table::setup_lexer::SetupLexer::new(&setup);
            let page_table_setup = page_table::setup_parser::SetupParser::new()
                .parse(&isa, lexer)
                .map_err(|error| {
                    axiomatic_litmus::format_error_page_table_setup(
                        litmus_setup,
                        error.map_location(|pos| pos - isa.default_page_table_setup.len()),
                    )
                })
                .map_err(Error::PageTableSetup)?;
            let custom_tables = page_table_setup.iter().any(|c| {
                if let isla_axiomatic::page_table::setup::Constraint::Option(op, val) = c {
                    matches!((op.as_str(), val), ("default_tables", false))
                } else {
                    false
                }
            });
            if custom_tables {
                return Err(Error::Unsupported("litmus-toml-translator doesn't support custom tables".to_owned()));
            }
            (page_table_setup, litmus_setup.to_string())
        } else {
            return Err(Error::PageTableSetup("page_table_setup must be a string".to_string()));
        }
    } else {
        (Vec::new(), "".to_string())
    };

    // eprintln!("{page_table_setup:#?}");

    let init_state = if mmu_on {
        litmus::gen_init_state(&page_table_setup, &var_names)?
    } else {
        var_names.clone().into_iter().map(|var| InitState::Var(var, "0".to_owned())).collect()
    };

    let additional_var_names = get_additional_vars_from_pts(&page_table_setup, var_names.clone())?;

    let fin = litmus_toml
        .get("final")
        .ok_or_else(|| Error::GetTomlValue("No final section found in litmus file".to_owned()))?;
    let (final_assertion_str, final_assertion) = (match fin.get("assertion").and_then(Value::as_str) {
        Some(assertion) => {
            let lexer = exp_lexer::ExpLexer::new(assertion);
            let sizeof = axiomatic_litmus::parse_sizeof_types(&litmus_toml).unwrap();
            exp_parser::ExpParser::new()
                .parse(&sizeof, isa.default_sizeof, &symtab, &isa.register_renames, lexer)
                .map(|parsed| (assertion.to_owned(), parsed))
                .map_err(|error| Error::ParseFinalAssertion(error.to_string()))
        }
        None => Err(Error::GetTomlValue("No final assertion found in litmus file".to_owned())),
    })?;

    let threads: Vec<Thread> = litmus_toml
        .get("thread")
        .and_then(|t| t.as_table())
        .ok_or_else(|| Error::GetTomlValue("No threads found in litmus file (must be a toml table)".to_owned()))
        .and_then(|t| {
            t.into_iter()
                .map(|(name, thread)| {
                    parse_thread(name.as_ref(), thread, final_assertion.clone(), keep_histogram, &symtab)
                })
                .collect()
        })?;

    let regs = if keep_histogram {
        regs_from_final_assertion(&symtab, final_assertion.clone())?.into_iter().collect()
    } else {
        let mut threads_cared_about = BTreeSet::new();
        let used_regs = regs_from_final_assertion(&symtab, final_assertion.clone())?;
        for (thread, _reg) in used_regs {
            threads_cared_about.insert(thread);
        }
        threads_cared_about.iter().map(|thread_name| (*thread_name, Reg::X(0))).collect()
    };

    let sections = litmus_toml
        .get("section")
        .map(|s| s.as_table().ok_or_else(|| Error::GetTomlValue("Thread sync handler is not a table".to_owned())))
        .transpose()?
        .cloned()
        .unwrap_or_default();

    let thread_sync_handlers = sections
        .iter()
        .filter_map(|(name, handler)| {
            parse_thread_sync_handler_from_section(name, handler, &threads, &symtab).transpose()
        })
        .collect::<Result<_>>()?;

    Ok(Litmus {
        arch,
        name,
        page_table_setup_str,
        threads,
        thread_sync_handlers,
        final_assertion_str,
        final_assertion,
        var_names,
        additional_var_names,
        regs,
        mmu_on,
        init_state,
    })
}
