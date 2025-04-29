use std::fs;
use std::path::Path;
use std::ffi::OsStr;

use isla_lib::bitvector::BV;
use isla_lib::config::ISAConfig;
use isla_lib::ir::*;
use isla_lib::ir_lexer::new_ir_lexer;
use isla_lib::ir_parser;
use isla_lib::ir::serialize::{DeserializedArchitecture, read_serialized_architecture};

use crate::error::{Error, Result};

/// An architecture passed on the command line (via the -A flag) can
/// either be an unparsed Sail IR file, or a serialized pre-parsed
/// file.
pub enum Architecture<B> {
    Unparsed(String),
    Deserialized(DeserializedArchitecture<B>),
}

/// An architecture file which has been parsed into the symbols and ir.
pub struct ParsedArchitecture<'ir, B> {
    pub symtab: Symtab<'ir>,
    pub ir: Vec<Def<Name, B>>,
}

fn parse_ir<'input, B: BV>(contents: &'input str, symtab: &mut Symtab<'input>) -> Result<Vec<Def<Name, B>>> {
    ir_parser::IrParser::new().parse(symtab, new_ir_lexer(contents)).map_err(|e| Error::LoadArchIR(format!("{}", e)))
}

impl<B: BV> Architecture<B> {
    pub fn parse<'ir>(&'ir self) -> Result<ParsedArchitecture<'ir, B>> {
        match self {
            Architecture::Unparsed(arch) => {
                let mut symtab = Symtab::new();
                let ir = parse_ir(arch, &mut symtab)?;
                Ok(ParsedArchitecture { symtab, ir })
            }
            Architecture::Deserialized(arch) => {
                let symtab = Symtab::from_raw_table(&arch.strings, &arch.files);
                let ir = arch.ir.clone();
                Ok(ParsedArchitecture { symtab, ir })
            }
        }
    }
}

pub fn load_ir<P, B>(file: P) -> Result<Architecture<B>>
where
    P: AsRef<Path>,
    B: BV,
{
    let file = file.as_ref();
    if !file.exists() {
        return Err(
            Error::LoadArchIR(format!("file '{}' does not exist", file.display()))
        );
    }

    match file.extension().and_then(OsStr::to_str) {
        Some("irx") => read_serialized_architecture(file, true).map(Architecture::Deserialized).map_err(|e| Error::LoadArchIR(format!("{}", e))),
        _ => {
            let contents = fs::read_to_string(file).map_err(|e| Error::LoadArchIR(format!("{}", e)))?;
            Ok(Architecture::Unparsed(contents))
        }
    }
}

pub fn load_isa_config<'ir, P, B>(file: P, arch: &ParsedArchitecture<'ir, B>) -> Result<ISAConfig<B>>
where
    P: AsRef<Path>,
    B: BV,
{
    let contents = fs::read_to_string(file).map_err(|e| Error::LoadArchConfig(format!("{}", e)))?;
    let type_info = IRTypeInfo::new(&arch.ir);
    let isa =
        ISAConfig::parse(
            &contents,
            None,
            &arch.symtab,
            &type_info,
        )
        .map_err(Error::LoadArchConfig)?;
    Ok(isa)
}
