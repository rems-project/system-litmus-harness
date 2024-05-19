use std::num::ParseIntError;

use thiserror::Error;

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug, Error)]
pub enum Error {
    #[error("loading aarch64 archictecture failed: {0}")]
    LoadArchConfig(String),
    #[error("parsing test TOML failed: {0}")]
    ParseToml(toml::de::Error),
    #[error("parsing value from test TOML failed: {0}")]
    GetTomlValue(String),
    #[error("parsing reset expression from TOML failed: {0}")]
    ParseResetValue(String),
    #[error("parsing thread failed: {0}")]
    ParseThread(String),
    #[error("parsing final assertion failed: {0}")]
    ParseFinalAssertion(String),
    #[error("parsing register name failed: {0}")]
    ParseReg(String),
    #[error("page table setup failed: {0}")]
    PageTableSetup(String),
    #[error("function lacks argument: {0}")]
    GetFunctionArg(String),
    #[error("parsing expression failed: {0}")]
    ParseExp(String),
    #[error("parsing bits from string failed: {0}")]
    ParseBitsFromString(ParseIntError),
    #[error("parsing eret from sync handler: {0}")]
    ParseSyncHandlerEret(String),
    #[error("unimplemented function: {0}")]
    UnimplementedFunction(String),
    #[error("unable to match handler to a thread and EL: {0}")]
    UnmatchedHandler(String),
    #[error("test not supported: {0}")]
    Unsupported(String),
}
