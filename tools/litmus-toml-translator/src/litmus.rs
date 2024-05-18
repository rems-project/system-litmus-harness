//! litmus.rs contains definitions for intermediate test data structure [`Litmus`].
//!
//! This file is mostly declarative code, defining how to represent a test, thread, exception
//! handler etc.

use std::collections::{HashMap, BTreeSet};
use std::fmt;

use once_cell::sync::Lazy;
use regex::Regex;

use isla_axiomatic::litmus::exp::Exp;
use isla_axiomatic::page_table;
use isla_lib::bitvector::{b64::B64, BV};

use crate::error::{Error, Result};

/// Main test data structure. Output of `parse` function in parse.rs and input to `write_output`
/// function in output.rs.
#[derive(Debug)]
pub struct Litmus {
    pub arch: String,
    pub name: String,
    pub page_table_setup_str: String,
    pub threads: Vec<Thread>,
    pub thread_sync_handlers: Vec<ThreadSyncHandler>,
    pub var_names: Vec<String>,
    pub additional_var_names: Vec<String>,
    pub regs: Vec<(u8, Reg)>,
    pub final_assertion_str: String,
    pub final_assertion: Exp<String>,
    pub mmu_on: bool,
    pub init_state: Vec<InitState>,
}

/// Wrapper type (mainly for `MovSrc`), allowing for negatable, expression-like representations.
/// Used in assertion compilation.
#[derive(Debug, Clone)]
pub enum Negatable<T> {
    Eq(T),
    Not(T),
}

/// Representation of a thread for use by `Litmus` struct.
#[derive(Debug)]
pub struct Thread {
    pub name: String,
    pub code: String,
    pub el: u8,
    pub regs_clobber: BTreeSet<Reg>,
    pub vbar_el1: Option<B64>,
    pub reset: HashMap<Reg, MovSrc>,
    pub assert: Option<Vec<(Reg, Negatable<MovSrc>)>>,
} // assert field: Some(conjunction of reg-val pairs) or ..
  //               None if 'keep_histogram' cli option used.

/// Representation of a synchronous exception handler (similar to thread).
#[derive(Debug)]
pub struct ThreadSyncHandler {
    pub name: String,
    pub code: String,
    pub regs_clobber: BTreeSet<Reg>,
    pub threads_els: Vec<(usize, u8)>, // (thread_no, el)
}

/// Convenient type for representing a value which could be moved into a register in assembly
/// (eg. could move a natural number or the page table entry of a symbol).
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum MovSrc {
    Nat(B64),
    Bin(B64),
    Hex(B64),
    Reg(String),
    Pte(String, u8),
    Desc(String, u8),
    Page(String),
}

/// Representation of a register. Note that most of these remain unused. Support for non X-W
/// registers is not guaranteed since irrelevant in almost all tests.
#[derive(Debug, Clone, Hash, PartialOrd, Ord, PartialEq, Eq)]
pub enum Reg {
    X(u8), // X0..X30 General purpose reg (all 64 bits)
    W(u8), // W0..W30 General purpose reg (bottom 32 bits)

    B(u8), // B0..B30 General purpose floating point reg (bottom 8 bits)
    H(u8), // H0..H30 General purpose floating point reg (bottom 16 bits)
    S(u8), // S0..S30 General purpose floating point reg (bottom 32 bits)
    D(u8), // D0..D30 General purpose floating point reg (bottom 64 bits)
    Q(u8), // Q0..Q30 General purpose floating point reg (all 128 bits)
    // V(u8, u8)   // V[0..30].[0..?] General purpose floating point reg (vec)
    PState(String), // PSTATE regs
    VBar(String),   // VBAR regs
    Isla(String),   // isla-specific register (ignored)
}

/// Representation of a line of output code to be formatted into `INIT_STATE` macro in C.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum InitState {
    Unmapped(String),
    Var(String, String), // TODO: This should use MovSrc's?
    Alias(String, String),
}

impl MovSrc {
    /// Update the inner binary value of a `MovSrc` with some function.
    pub fn map<F>(&self, f: F) -> Self
    where
        F: FnOnce(B64) -> B64,
    {
        match self {
            Self::Nat(bv) => Self::Nat(f(*bv)),
            Self::Bin(bv) => Self::Bin(f(*bv)),
            Self::Hex(bv) => Self::Hex(f(*bv)),
            Self::Reg(_) | Self::Pte(..) | Self::Desc(..) | Self::Page(_) => panic!(),
        }
    }

    /// Get raw binary value inside a `MovSrc`.
    pub fn bits(&self) -> &B64 {
        match self {
            Self::Nat(bv) | Self::Bin(bv) | Self::Hex(bv) => bv,
            Self::Reg(_) | Self::Pte(..) | Self::Desc(..) | Self::Page(_) => panic!(),
        }
    }

    /// Translate `MovSrc` into string for output assembly.
    pub fn as_asm(&self) -> String {
        match self {
            Self::Nat(n) => format!("#{}", n.lower_u64()),
            Self::Bin(n) => format!("#0b{:b}", n.lower_u64()),
            Self::Hex(n) => format!("#0x{:x}", n.lower_u64()),
            Self::Reg(reg) => format!("%[{reg}]"),
            Self::Page(sym) => format!("%[{sym}page]"),
            Self::Pte(sym, 3) => format!("%[{sym}pte]"),
            Self::Pte(sym, 2) => format!("%[{sym}pmd]"),
            Self::Pte(sym, 1) => format!("%[{sym}pud]"),
            Self::Desc(sym, 3) => format!("%[{sym}desc]"),
            Self::Desc(sym, 2) => format!("%[{sym}pmddesc]"),
            Self::Desc(sym, 1) => format!("%[{sym}puddesc]"),

            Self::Pte(_, _) => unimplemented!(),
            Self::Desc(_, _) => unimplemented!(),
        }
    }

    /// Translate `MovSrc` into string for output C code (used in assertion compilation).
    pub fn as_c_code(&self) -> String {
        match self {
            Self::Nat(n) => format!("{}", n.lower_u64()),
            Self::Bin(n) => format!("0b{:b}", n.lower_u64()),
            Self::Hex(n) => format!("0x{:x}", n.lower_u64()),
            Self::Reg(reg) => format!("out{}", reg),
            Self::Pte(sym, 3) => format!("var_pte(data, \"{sym}\")"),
            Self::Pte(sym, 2) => format!("var_pmd(data, \"{sym}\")"),
            Self::Pte(sym, 1) => format!("var_pud(data, \"{sym}\")"),
            Self::Desc(sym, 3) => format!("var_ptedesc(data, \"{sym}\")"),
            Self::Desc(sym, 2) => format!("var_pmddesc(data, \"{sym}\")"),
            Self::Desc(sym, 1) => format!("var_puddesc(data, \"{sym}\")"),
            Self::Page(sym) => format!("var_page(\"{sym}\")"),

            Self::Pte(_, _) => unimplemented!(),
            Self::Desc(_, _) => unimplemented!(),
        }
    }
}

impl TryFrom<&Exp<String>> for MovSrc {
    type Error = Error;

    fn try_from(exp: &Exp<String>) -> Result<Self> {
        // eprintln!("exp {exp:?}");

        fn bv_from_exp(exp: &Exp<String>) -> Result<B64> {
            match exp {
                Exp::Nat(n) => Ok(B64::new(*n, 64)),
                Exp::Bin(s) | Exp::Hex(s) => {
                    B64::from_str(s).ok_or_else(|| Error::ParseExp(format!("couldn't create B64 from {s}")))
                }
                Exp::Bits64(bits, _len) => Ok(B64::new(*bits, 64)),
                exp => Err(Error::ParseExp(format!("couldn't create B64 from {exp:?}"))),
            }
        }

        fn get_arg<'a>(fun: &str, args: &'a [Exp<String>], idx: usize) -> Result<&'a Exp<String>> {
            args.get(idx).ok_or_else(|| Error::GetFunctionArg(format!("{fun}:arg{idx}")))
        }

        fn get_kwarg<'a>(fun: &str, kw_args: &'a HashMap<String, Exp<String>>, kw: &str) -> Result<&'a Exp<String>> {
            kw_args.get(kw).ok_or_else(|| Error::GetFunctionArg(format!("{fun}:arg_{kw}")))
        }

        log::info!("parsing {exp:?}");
        match exp {
            Exp::Nat(n) => Ok(Self::Nat(B64::new(*n, 64))),
            Exp::Bin(s) => {
                let b64 = u64::from_str_radix(s, 2).map_err(Error::ParseBitsFromString)?;
                Ok(Self::Bin(B64::new(b64, 64)))
            }
            Exp::Hex(s) => {
                let b64 = u64::from_str_radix(s, 16).map_err(Error::ParseBitsFromString)?;
                Ok(Self::Hex(B64::new(b64, 64)))
            }
            Exp::Bits64(bits, _len) => Ok(Self::Bin(B64::new(*bits, 64))),
            Exp::Loc(var) => Ok(Self::Reg(var.clone())),

            Exp::App(f, args, kw_args) => match f.as_ref() {
                "extz" => get_arg(f, args, 0)?.try_into(),
                "exts" => {
                    let mov_src: MovSrc = get_arg(f, args, 0)?.try_into()?;
                    let extend_by = bv_from_exp(get_arg(f, args, 1)?)?.lower_u64() as u32;
                    Ok(mov_src.map(|bv| bv.sign_extend(extend_by)))
                }
                "bvand" => {
                    let mov_src_bv1: MovSrc = get_arg(f, args, 0)?.try_into()?;
                    let bv2 = bv_from_exp(&get_arg(f, args, 1)?.clone())?;
                    Ok(mov_src_bv1.map(|bv1| bv1 & bv2))
                }
                "bvor" => {
                    let mov_src_bv1: MovSrc = get_arg(f, args, 0)?.try_into()?;
                    let bv2 = bv_from_exp(&get_arg(f, args, 1)?.clone())?;
                    Ok(mov_src_bv1.map(|bv1| bv1 | bv2))
                }
                "bvxor" => {
                    let mov_src_bv1: MovSrc = get_arg(f, args, 0)?.try_into()?;
                    let bv2 = bv_from_exp(&get_arg(f, args, 1)?.clone())?;
                    Ok(mov_src_bv1.map(|bv1| bv1 ^ bv2))
                }
                "bvlshr" => {
                    let mov_src: MovSrc = get_arg(f, args, 0)?.try_into()?;
                    let shift_by = bv_from_exp(&get_arg(f, args, 1)?.clone())?;
                    Ok(mov_src.map(|bv1| bv1 >> shift_by))
                }
                "bvshl" => {
                    let mov_src: MovSrc = get_arg(f, args, 0)?.try_into()?;
                    let shift_by = bv_from_exp(&get_arg(f, args, 1)?.clone())?;
                    Ok(mov_src.map(|bv1| bv1 << shift_by))
                }
                "page" => {
                    if let Exp::Loc(var) = get_arg(f, args, 0)? {
                        Ok(MovSrc::Page(var.to_owned()))
                    } else {
                        Err(Error::GetFunctionArg("page:arg0 was not parsed correctly".to_owned()))
                    }
                }
                pte if pte.starts_with("pte") => {
                    let lvl = pte
                        .strip_prefix("pte")
                        .unwrap()
                        .parse()
                        .map_err(|_| Error::UnimplementedFunction(pte.to_owned()))?;
                    if lvl > 0 && lvl <= 3 {
                        if let Exp::Loc(var) = get_arg(f, args, 0)? {
                            Ok(MovSrc::Pte(var.clone(), lvl))
                        } else {
                            Err(Error::GetFunctionArg("pte3:arg0 was not parsed correctly".to_owned()))
                        }
                    } else {
                        Err(Error::UnimplementedFunction(format!("pte [lvl = {lvl}] function not supported")))
                    }
                }
                desc if desc.starts_with("desc") => {
                    let lvl = desc
                        .strip_prefix("desc")
                        .unwrap()
                        .parse()
                        .map_err(|_| Error::UnimplementedFunction(desc.to_owned()))?;
                    if lvl > 0 && lvl <= 3 {
                        if let Exp::Loc(var) = get_arg(f, args, 0)? {
                            Ok(MovSrc::Desc(var.to_owned(), lvl))
                        } else {
                            Err(Error::GetFunctionArg("desc:arg0 was not parsed correctly".to_owned()))
                        }
                    } else {
                        Err(Error::UnimplementedFunction(format!("desc [lvl = {lvl}] function not supported")))
                    }
                }
                mkdesc if mkdesc.starts_with("mkdesc") => {
                    let lvl = mkdesc
                        .strip_prefix("mkdesc")
                        .unwrap()
                        .parse()
                        .map_err(|_| Error::UnimplementedFunction(mkdesc.to_owned()))?;
                    if lvl > 0 && lvl <= 3 {
                        if let Exp::Loc(oa) = get_kwarg(f, kw_args, "oa")? {
                            Ok(MovSrc::Desc(oa.to_owned(), lvl))
                        } else {
                            Err(Error::GetFunctionArg("mkdesc:arg_oa was not parsed correctly".to_owned()))
                        }
                    } else {
                        Err(Error::UnimplementedFunction(format!("mkdesc [lvl = {lvl}] function not supported")))
                    }
                }
                // "vector_subrange" => {
                //     let mov_src: MovSrc = get_arg(f, args, 0)?.try_into()?;
                //     let from = bv_from_exp(get_arg(f, args, 1)?)?.lower_u64() as u32;
                //     let len = bv_from_exp(get_arg(f, args, 2)?)?.lower_u64() as u32;
                //     Ok(mov_src.map(|bv| bv.slice(from, len).unwrap()))
                // }
                // "pte1" | "pte2" | "desc1" | "desc2" | "mkdesc1" | "mkdesc2" => {
                //     Err(Error::Unsupported(format!("function {f} not supported")))
                // }
                // "page" | "offset" => Err(Error::Unimpl
                // TODO: offset()
                f => Err(Error::UnimplementedFunction(f.to_owned())),
            },
            other => Err(Error::ParseResetValue(format!("handling of {other:?} is not implemented"))),
        }
    }
}

impl fmt::Display for MovSrc {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Self::Nat(n) => write!(f, "{}", n.lower_u64()),
            Self::Bin(n) => write!(f, "0b{:b}", n.lower_u64()),
            Self::Hex(n) => write!(f, "0x{:x}", n.lower_u64()),
            Self::Reg(reg) => write!(f, "{reg}"),
            Self::Pte(reg, lvl) => write!(f, "pte{lvl}({reg})"),
            Self::Desc(reg, lvl) => write!(f, "desc{lvl}({reg})"),
            Self::Page(reg) => write!(f, "page({reg})"),
        }
    }
}

impl Reg {
    // Translate `Reg` variant into asm string.
    pub fn as_asm(&self) -> String {
        use Reg::*;
        match &self {
            X(n) => "x".to_owned() + &n.to_string(),
            W(n) => "w".to_owned() + &n.to_string(),
            B(n) => "b".to_owned() + &n.to_string(),
            H(n) => "h".to_owned() + &n.to_string(),
            S(n) => "s".to_owned() + &n.to_string(),
            D(n) => "d".to_owned() + &n.to_string(),
            Q(n) => "q".to_owned() + &n.to_string(),

            PState(_) => unimplemented!("Converting PSTATE regs to asm is not implemented properly yet."),
            VBar(_) => unimplemented!("Converting PSTATE regs to asm is not implemented properly yet."),
            Isla(_) => unimplemented!("Converting isla-specific registers to asm is not possible."),
        }
    }

    /// Get inner index of some `Reg`.
    pub fn idx(&self) -> u8 {
        use Reg::*;
        match &self {
            X(n) => *n,
            W(n) => *n,
            B(n) => *n,
            H(n) => *n,
            S(n) => *n,
            D(n) => *n,
            Q(n) => *n,

            PState(_) => unimplemented!("Converting PSTATE regs to idx is not possible."),
            VBar(_) => unimplemented!("Converting PSTATE regs to idx is not possible."),
            Isla(_) => unimplemented!("Converting isla-specific registers to idx is not possible."),
        }
    }

    /// Convert `Reg` into quoted assembly.
    pub fn as_asm_quoted(&self) -> String {
        format!("\"{}\"", self.as_asm())
    }

    /// Convert `Reg` into harness-generated name of form `outp0r1` for register `x1` of thread 0.
    pub fn as_output_str(&self, thread_no: u8) -> String {
        match &self {
            Self::X(n) => format!("outp{thread_no}r{n}"),
            _ => unimplemented!("output reg name generation not implemented for non \"X\" registers."),
        }
    }
}

/// Take block of asm and attempt to return all mentioned registers. This is used to generate a
/// register clobber list.
pub fn parse_regs_from_asm(asm: &str) -> Result<BTreeSet<Reg>> {
    let lines = asm.trim().split('\n');
    let mut hs = BTreeSet::new();
    for line in lines {
        let line = match line.split_once(';') {
            Some((instr, _comment)) => instr,
            _ => line,
        }
        .trim();

        static RE: Lazy<Regex> = Lazy::new(|| Regex::new(r"\b([xXwWbBhHsSdDqQ]([0-9]|[12][0-9]|30))\b").unwrap());
        for tok in RE.find_iter(line) {
            let reg = parse_reg_from_str(tok.into())?;
            hs.insert(reg);
        }
    }
    Ok(hs)
}

/// Attempt to convert string into a register, checking for special register types.
pub fn parse_reg_from_str(asm: &str) -> Result<Reg> {
    use Reg::*;

    if asm.starts_with("PSTATE") {
        return Ok(Reg::PState(asm.to_owned()));
    }

    if asm.starts_with("VBAR") {
        return Ok(Reg::VBar(asm.to_owned()));
    }

    if asm.starts_with("__isla") {
        log::info!("found isla-specific register {asm}");
        return Ok(Reg::Isla(asm.to_owned()));
    }

    if asm.starts_with("ELR_EL") || asm.starts_with("SPSR_EL") || asm.starts_with("TTBR") {
        return Err(Error::Unsupported(format!(
            "litmus-toml-translator does not support special registers like {asm} in thread resets."
        )));
    }

    let (t, idx) = asm.split_at(1);
    let idx: u8 = idx.parse().map_err(|e: std::num::ParseIntError| Error::ParseReg(format!("{e} ({asm})")))?;
    match t {
        "x" | "X" => Ok(X(idx)),
        "w" | "W" => Ok(W(idx)),
        "b" | "B" => Ok(B(idx)),
        "h" | "H" => Ok(H(idx)),
        "s" | "S" => Ok(S(idx)),
        "d" | "D" => Ok(D(idx)),
        "q" | "Q" => Ok(Q(idx)),

        "r" | "R" => Ok(X(idx)), // TODO: this should use register renames
        t => Err(Error::ParseReg(format!("{t} ({asm})"))),
    }
}

/// Convert `page_table_setup` constraints into a list of [`InitState`] variants.
pub fn gen_init_state(
    page_table_setup: &[page_table::setup::Constraint],
    symbolics: &Vec<String>,
) -> Result<Vec<InitState>> {
    use page_table::setup::{AddressConstraint, Constraint, Exp, TableConstraint};
    let mut all_pas = BTreeSet::new();
    let specified_inits = page_table_setup
        .iter()
        .filter_map(|constraint| match constraint {
            Constraint::Initial(Exp::Id(id), val) => {
                let val = match val {
                    Exp::I128(n) => Some(n.to_string()),
                    Exp::Hex(s) | Exp::Bin(s) => Some(s.clone()),
                    _ => None,
                    //Exp:://Some(InitState::Var(id.clone(), val)),
                };
                val.map(|v| InitState::Var(id.clone(), v))
            }
            Constraint::Table(TableConstraint::MapsTo(Exp::Id(from), Exp::Id(to), _, _lvl, _)) => {
                if to.as_str() == "invalid" {
                    Some(InitState::Unmapped(from.clone()))
                } else {
                    Some(InitState::Alias(from.clone(), to.clone()))
                }
            }
            Constraint::Address(AddressConstraint::Physical(_alignment, phys)) => {
                all_pas.extend(phys.clone());
                None
            }
            _ => None,
        })
        .collect();

    fill_unbacked_addrs(specified_inits, symbolics, &all_pas)
}

/// Fix list of [`InitState`] variants by looking for unbacked addrs and recomputing.
fn fill_unbacked_addrs(
    specified_inits: Vec<InitState>,
    symbolics: &Vec<String>,
    pas: &BTreeSet<String>,
) -> Result<Vec<InitState>> {
    // Many Isla tests omit initial values for some addresses. This causes problems for
    // system-litmus-harness since aliases should be backed by heap memory. Here, we initialise any
    // so-far unbacked addresses with zeros.
    let mut tweaked_inits = specified_inits.clone();

    // We create a simple graph where aliases correspond to edges (in case we have multiple aliases
    // for same physical address).
    let mut edges = HashMap::new();
    let mut unbacked_addrs = BTreeSet::new();
    let mut backed_addrs = HashMap::new();
    let mut alias_directions: Vec<(String, String)> = vec![];

    for init in &specified_inits {
        if let InitState::Alias(from, to) = init {
            if !edges.contains_key(from) {
                edges.insert(from.clone(), vec![]);
            }
            if !edges.contains_key(to) {
                edges.insert(to.clone(), vec![]);
            }
            let from_adj = edges.get_mut(from).unwrap();
            from_adj.push(to);
            let to_adj = edges.get_mut(to).unwrap();
            to_adj.push(from);
            unbacked_addrs.insert(to);
            unbacked_addrs.insert(from);
        }
    }

    fn dfs(
        addr: &String,
        backed_val: Option<String>,
        edges: &HashMap<String, Vec<&String>>,
        seen: &mut HashMap<String, Option<String>>,
        alias_directions: &mut Vec<(String, String)>,
    ) -> Result<bool> {
        seen.insert(addr.to_owned(), backed_val.clone());
        let mut backed = false;
        if let Some(aliases) = edges.get(addr) {
            for alias in aliases {
                if let Some(val) = seen.get(*alias) {
                    if val != &backed_val {
                        return Err(Error::PageTableSetup(format!(
                            "Multiple backed values for same address ({val:?} and {backed_val:?})"
                        )));
                    }
                } else {
                    backed |= dfs(alias, backed_val.clone(), edges, seen, alias_directions)?;
                    alias_directions.push((addr.to_owned(), (*alias).to_owned()));
                }
            }
        }
        Ok(backed)
    }

    for init in &specified_inits {
        match init {
            InitState::Var(id, val) => {
                unbacked_addrs.remove(id);
                backed_addrs.insert(id.clone(), Some(val.clone()));
            }
            InitState::Unmapped(id) => {
                unbacked_addrs.remove(id);
                backed_addrs.insert(id.clone(), None);
            }
            _ => {}
        }
    }

    let initially_backed_addrs = backed_addrs.clone();

    for (addr, val) in initially_backed_addrs {
        let _ = dfs(&addr, val, &edges, &mut backed_addrs, &mut alias_directions)?;
    }

    for backed_addr in backed_addrs.keys() {
        unbacked_addrs.remove(backed_addr);
    }

    let default_val_for_pa = Some("0".to_owned());

    for pa in pas {
        if !backed_addrs.contains_key(pa) {
            backed_addrs.insert(pa.clone(), default_val_for_pa.clone());
            let _ = dfs(pa, default_val_for_pa.clone(), &edges, &mut backed_addrs, &mut alias_directions)?;
            tweaked_inits.push(InitState::Var(pa.clone(), "0".to_owned()));
        }
    }

    for backed_addr in backed_addrs.keys() {
        unbacked_addrs.remove(&backed_addr);
    }

    // check if any addrs wtill not backed
    for unbacked_addr in unbacked_addrs {
        tweaked_inits.push(InitState::Unmapped(unbacked_addr.clone()));
    }
    for sym in symbolics {
        if !backed_addrs.contains_key(sym) {
            tweaked_inits.push(InitState::Unmapped(sym.clone()));
        }
    }

    // Filter out all aliases
    tweaked_inits = tweaked_inits.into_iter().filter(|init| !matches!(init, InitState::Alias(..))).collect::<Vec<_>>();

    tweaked_inits.extend(alias_directions.into_iter().map(|(from, to)| InitState::Alias(to, from)));

    Ok(tweaked_inits)
}
