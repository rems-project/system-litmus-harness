# Litmus Test Translator

`litmus-toml-translator` is a tool for converting [Isla](https://github.com/rems-project/isla)-style
`toml` litmus tests into C code which can be built into
[`system-litmus-harness`](https://github.com/rems-project/system-litmus-harness) for running on
ARMv8-A hardware.

This tool was tested against the list of tests present in
[`litmus-tests/litmus-tests-armv8a-system-vmsa`](https://github.com/litmus-tests/litmus-tests-armv8a-system-vmsa/)
where it has a success rate of approximately 67% (179/267 tests).

## Building and Running

You can build `litmus-toml-translator` like any other Rust binary.

```sh
git clone --recurse-submodules git@github.com:thud/litmus-toml-translator
cd litmus-toml-translator
cargo build --release && cp target/release/litmus-toml-translator .
```

Then run with `./litmus-toml-translator`.

### Ignore Lists

It can be useful to manually specify the names of tests to ignore. I include a list of tests which
I know to be problematic (at harness runtime) in [`toml-tests-ignore.txt`](toml-tests-ignore.txt).
Using the `-x` flag will tell `litmus-toml-translator` to ignore all these tests.

### Interpreting Output

The default behaviour of this tool is to take all register-value pairs in a "final assertion" and
compile them into a single boolean flag per mentioned thread. This produces an output where the only
"interesting" result is one where all output registers are equal to 1, discarding the histogram for
a test. This is done because of how result values are checked by the harness generating too many
results and failing at runtime. This only works for tests where the final assertion is a simple
conjunction of register-value pairs, otherwise the test is listed as unsupported.

If you would rather keep histograms, then use the `--keep-histogram` option.

### Example Usage

In order to actually run any output code in the harness, you will need to have the harness setup
(see [getting started](https://rems-project.github.io/system-litmus-harness/getting_started.html)).

The below will recursively look through a directory of litmus tests, attempt to convert any `toml`
files into C code and dump into the 'translated' group in the harness repo. I use the `-F` (flatten)
flag here to avoid group names conflicting with the pre-existing `system-litmus-harness` groups.

```sh
./litmus-toml-translator -f -F -x toml-tests-ignore.txt path/to/litmus-tests -o path/to/system-litmus-harness/tests/translated
cd path/to/system-litmus-harness
make build
./qemu_litmus @translated
```

### Quick Fixes

Some tests in
[`litmus-tests/litmus-tests-armv8a-system-vmsa`](https://github.com/litmus-tests/litmus-tests-armv8a-system-vmsa/)
can cause problems when compiling the harness. I include a simple
[diff](system-litmus-harness-fixes.diff) which contains changes to support more of these tests.

## Help

```
litmus-toml-translator v0.1.0 isla@esop22-222-gdcf9315
thud <thud@thud.dev>
Translate isla TOML tests for use with system-litmus-harness.

Usage: litmus-toml-translator [OPTIONS] [INPUT]...

Arguments:
  [INPUT]...  input file(s) or dir of tests (instead of stdin)

Options:
  -o, --output <OUTPUT>            output file or dir (default is stdout)
  -f, --force                      allow overwriting files with output
  -F, --flatten                    flatten output tests into single output dir
  -x, --ignore-list <IGNORE_LIST>  specify text file with names of tests to ignore
      --keep-histogram             don't compile final assertion into one reg per thread for each test run
  -v, --verbose...
  -h, --help                       Print help
  -V, --version                    Print version
```
