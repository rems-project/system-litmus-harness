use std::io::{BufReader, Read};

use isla_lib::bitvector::b64::B64;
use isla_lib::config::ISAConfig;
use isla_lib::ir::serialize::DeserializedArchitecture;
use isla_lib::ir::*;

use crate::error::{Error, Result};

pub fn load_aarch64_config_irx() -> Result<DeserializedArchitecture<B64>> {
    let ir = include_bytes!(concat!(env!("CARGO_MANIFEST_DIR"), "/isla-snapshots/armv8p5.irx"));
    let mut buf = BufReader::new(&ir[..]);

    let mut isla_magic = [0u8; 8];
    buf.read_exact(&mut isla_magic).map_err(|e| e.to_string()).map_err(Error::LoadArchConfig)?;
    if &isla_magic != b"ISLAARCH" {
        panic!("Isla arch snapshot magic invalid {:?}", String::from_utf8(isla_magic.to_vec()));
    }

    let mut len = [0u8; 8];

    buf.read_exact(&mut len).map_err(|e| Error::LoadArchConfig(e.to_string()))?;
    let mut version = vec![0; usize::from_le_bytes(len)];
    buf.read_exact(&mut version).map_err(|e| Error::LoadArchConfig(e.to_string()))?;

    let v_exp = env!("ISLA_VERSION").as_bytes();
    if version != v_exp {
        let v_got = String::from_utf8_lossy(&version).into_owned();
        let v_exp = String::from_utf8_lossy(v_exp).into_owned();
        panic!("Isla version mismatch (got {v_got}, expected {v_exp})");
    }

    buf.read_exact(&mut len).map_err(|e| Error::LoadArchConfig(e.to_string()))?;
    let mut raw_ir = vec![0; usize::from_le_bytes(len)];
    buf.read_exact(&mut raw_ir).map_err(|e| Error::LoadArchConfig(e.to_string()))?;

    buf.read_exact(&mut len).map_err(|e| Error::LoadArchConfig(e.to_string()))?;
    let mut raw_symtab = vec![0; usize::from_le_bytes(len)];
    buf.read_exact(&mut raw_symtab).map_err(|e| Error::LoadArchConfig(e.to_string()))?;

    let ir: Vec<Def<Name, B64>> =
        serialize::deserialize(&raw_ir).ok_or(Error::LoadArchConfig("failed to deserialise armv8p5 ir".to_owned()))?;
    let (strings, files): (Vec<String>, Vec<String>) =
        isla_lib::bincode::deserialize(&raw_symtab).map_err(|e| Error::LoadArchConfig(e.to_string()))?;

    let arch = DeserializedArchitecture { files, strings, ir: ir.clone() };

    Ok(arch)
}

pub fn load_aarch64_isa(arch: &DeserializedArchitecture<B64>, mmu_on: bool) -> Result<(ISAConfig<B64>, Symtab)> {
    let symtab = Symtab::from_raw_table(&arch.strings, &arch.files);
    let type_info = IRTypeInfo::new(&arch.ir);
    let isa = if mmu_on {
        ISAConfig::parse(
            include_str!(concat!(env!("CARGO_MANIFEST_DIR"), "/isla/configs/armv8p5_mmu_on.toml")),
            None,
            &symtab,
            &type_info,
        )
        .map_err(Error::LoadArchConfig)?
    } else {
        // assert!(
        //     litmus_toml.get("symbolic").is_none(),
        //     "\"symbolic\" key should not be present if no page table setup specified"
        // );
        ISAConfig::parse(
            include_str!(concat!(env!("CARGO_MANIFEST_DIR"), "/isla/configs/armv8p5.toml")),
            None,
            &symtab,
            &type_info,
        )
        .map_err(Error::LoadArchConfig)?
    };
    Ok((isa, symtab))
}
