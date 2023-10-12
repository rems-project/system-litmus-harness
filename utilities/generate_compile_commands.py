#!/usr/bin/env python
# Generates a clangd suitable compile_commands.json file

import json
import pathlib

HERE = pathlib.Path(__file__).parent
ROOT = HERE.parent

compile_commands = []

for f in ROOT.rglob("*.c"):
    cmd_path = (ROOT / "bin" / (f.relative_to(ROOT))).with_suffix(".o.cmd")
    if cmd_path.exists():
        compile_commands.append(
            {
                "command": cmd_path.read_text(),
                "directory": str(ROOT),
                "file": str(f),
            }
        )

with open(ROOT / "compile_commands.json", "w") as f:
    json.dump(compile_commands, f, indent=4)