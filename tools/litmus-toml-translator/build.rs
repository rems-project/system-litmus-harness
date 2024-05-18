// BSD 2-Clause License
//
// Copyright (c) 2019, 2020 Alasdair Armstrong
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This is a modified version of isla/isla-lib/build.rs in order to set the ISLA_VERSION
// env variable to its relevant value.

use std::path::Path;
use std::process::Command;

use isla_lib::bitvector::b64::B64;
use isla_lib::ir::serialize::write_serialized_architecture;
use isla_lib::ir::*;
use isla_lib::ir_lexer::new_ir_lexer;
use isla_lib::ir_parser;

fn git_version() -> Option<String> {
    let output = Command::new("git")
        .current_dir(concat!(env!("CARGO_MANIFEST_DIR"), "/isla"))
        .args(["describe", "--dirty"])
        .output()
        .ok()?;
    if !output.status.success() {
        return None;
    }
    String::from_utf8(output.stdout).ok()
}

fn version() -> String {
    match git_version() {
        None => {
            let mut s = String::from("v");
            s.push_str(env!("CARGO_PKG_VERSION"));
            s
        }
        Some(version) => version,
    }
}

fn preprocess_ir(arch_name: &str) {
    let mut symtab = Symtab::new();
    let snapshot_path = Path::new(env!("CARGO_MANIFEST_DIR")).join(arch_name);
    let contents = std::fs::read_to_string(snapshot_path.clone()).unwrap();
    let arch = ir_parser::IrParser::new().parse(&mut symtab, new_ir_lexer(&contents)).unwrap();
    write_serialized_architecture::<B64>(snapshot_path.with_extension("irx").to_str().unwrap(), arch, &symtab).unwrap();
}

fn main() {
    lalrpop::process_root().unwrap();

    preprocess_ir("isla-snapshots/armv8p5.ir");

    println!("cargo:rustc-env=ISLA_VERSION={}", version());
}
