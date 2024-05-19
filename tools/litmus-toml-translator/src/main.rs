//! litmus-toml-translator tool for litmus test translation.
//!
//! This file includes the main control loop of the binary. It contains all file system and user
//! interface functionality.

mod arch;
mod error;
mod litmus;
mod output;
mod parse;

use std::collections::HashSet;
use std::io::Write;
use std::path::{Path, PathBuf};

use clap::{CommandFactory, Parser};
use colored::*;
use is_terminal::IsTerminal;

/// User interface (CLI) structure which specifies all operations supported by the tool.
#[derive(Debug, Parser)]
#[command(name = "litmus-toml-translator")]
#[command(author = "thud <thud@thud.dev>")]
#[command(about = "Translate isla TOML tests for use with system-litmus-harness.")]
#[command(version = concat!("v", env!("CARGO_PKG_VERSION"), " isla@", env!("ISLA_VERSION")))]
#[command(help_template(
    "\
{before-help}{name} {version}
{author-with-newline}{about-with-newline}
{usage-heading} {usage}

{all-args}{after-help}
"
))]
struct Cli {
    #[arg(help = "input file(s) or dir of tests (instead of stdin)")]
    input: Option<Vec<PathBuf>>,
    #[arg(short, long, help = "output file or dir (default is stdout)")]
    output: Option<PathBuf>,
    #[arg(short, long, help = "allow overwriting files with output")]
    force: bool,
    #[arg(short = 'F', long, help = "flatten output tests into single output dir")]
    flatten: bool,
    #[arg(short = 'x', long, help = "specify text file with names of tests to ignore")]
    ignore_list: Option<PathBuf>,
    #[arg(long, help = "don't compile final assertion into one reg per thread for each test run")]
    keep_histogram: bool,
    #[arg(short, long, action = clap::ArgAction::Count)]
    verbose: u8,
}

/// Translate toml into `Litmus` then output string (C code).
fn process_toml(raw_toml: String, keep_histogram: bool) -> error::Result<String> {
    let parsed_litmus = parse::parse(&raw_toml, keep_histogram)?;
    output::write_output(parsed_litmus, keep_histogram)
}

/// Check if a file is toml (by checking file extension).
fn is_toml(p: &Path) -> bool {
    p.extension().filter(|ext| ext.to_str().unwrap() == "toml").is_some()
}

// Recursively translate a path, descending into directories to find all descendent toml tests.
fn process_path(
    file_or_dir: &Path,
    out_dir: &Path,
    force: bool,
    flatten_to: &Option<PathBuf>,
    ignore: &HashSet<String>,
    keep_histogram: bool,
    results: &mut parse::TranslationResults,
) -> error::Result<()> {
    if ignore.contains(file_or_dir.file_name().unwrap().to_str().unwrap()) {
        log::warn!("file {file_or_dir:?} exists. skipping...");
        results.skipped += 1;
        eprintln!(
            "{} {} (in ignore list)",
            "Skipping".yellow().bold(),
            file_or_dir.as_os_str().to_string_lossy().bold(),
        );
    } else if file_or_dir.is_dir() {
        let dir = file_or_dir;
        let files = std::fs::read_dir(dir)
            .unwrap()
            .map(|entry| entry.unwrap().path())
            .filter(|entry| entry.is_dir() || is_toml(entry));
        for input in files {
            log::info!("dir:{dir:?} input:{input:?}");
            process_path(
                &input,
                &out_dir.join(PathBuf::from(input.file_name().unwrap())),
                force,
                flatten_to,
                ignore,
                keep_histogram,
                results,
            )
            .unwrap();
        }
    } else {
        let file = file_or_dir;
        if !is_toml(file) {
            log::info!("skipping {file:?}");
            return Ok(());
        }
        let file_name = file.file_name().unwrap().to_string_lossy().into_owned() + ".c";
        let out_dir = flatten_to.as_deref().unwrap_or(out_dir);
        let output_path = out_dir.with_file_name(file_name);
        if output_path.exists() && !force {
            log::warn!("file {output_path:?} exists. skipping...");
            results.skipped += 1;
            eprintln!(
                "{} {} ({} exists)",
                "Skipping".yellow().bold(),
                file.as_os_str().to_string_lossy().bold(),
                output_path.as_os_str().to_string_lossy().bold(),
            );
        } else {
            let toml = std::fs::read_to_string(file).unwrap();
            match process_toml(toml, keep_histogram) {
                Ok(output_code) => {
                    let parent = out_dir.parent().unwrap();
                    std::fs::create_dir_all(parent).unwrap();
                    log::info!("creating file at {output_path:?}");
                    let mut f = std::fs::File::create(&output_path).unwrap();
                    log::info!("writing process_toml({file:?}) to {output_path:?}");
                    write!(f, "{output_code}").unwrap();
                    log::info!("successfully translated {file:?} -> {output_path:?}");
                    results.succeeded += 1;
                    eprintln!(
                        "{} {} -> {}",
                        "Success".green().bold(),
                        file.as_os_str().to_string_lossy().bold(),
                        output_path.as_os_str().to_string_lossy().bold(),
                    );
                }
                Err(error::Error::Unsupported(e)) => {
                    log::error!("unsupported test {file:?} {e}");
                    results.unsupported += 1;
                    eprintln!("{} {}", "Unsupported".blue().bold(), file.as_os_str().to_string_lossy().bold(),);
                }
                Err(e) => {
                    log::error!("error translating test {file:?} {e}");
                    results.failed += 1;
                    eprintln!("{} translating {}", "Error".red().bold(), file.as_os_str().to_string_lossy().bold(),);
                }
            }
        }
    }
    Ok(())
}

fn init_logger(verbosity: u8) {
    let filter_level = match verbosity {
        0 => log::LevelFilter::Off,
        1 => log::LevelFilter::Warn,
        2 => log::LevelFilter::Info,
        3 => log::LevelFilter::Debug,
        4.. => log::LevelFilter::Trace,
    };
    pretty_env_logger::formatted_builder().filter_level(filter_level).init();
}

fn log_results(results: parse::TranslationResults) {
    eprintln!(
        "[{} succeeded ({}%), {} unsupported, {} skipped, {} failed]",
        results.succeeded.to_string().green(),
        results.percentage_succeeded().to_string().green(),
        results.unsupported.to_string().blue(),
        results.skipped.to_string().yellow(),
        results.failed.to_string().red(),
    );
}

/// Main control flow.
fn main() {
    let cli = Cli::parse();

    init_logger(cli.verbose);

    if let Some(out) = &cli.output {
        if out.exists() && !out.is_dir() && !cli.force {
            log::error!("output file {out:?} exists (and is not a directory). aborting...");
            std::process::exit(1); // TODO: better exit code here? (POSIX)
        }
    }

    if cli.flatten && cli.output.is_none() {
        log::error!("can only flatten output if output directory specified (-o). aborting...");
        std::process::exit(1);
    }

    let flatten_to = if cli.flatten { Some(cli.output.clone().unwrap().join(PathBuf::from("_"))) } else { None };

    let ignore = if let Some(ignore_list) = cli.ignore_list {
        let list: String = std::fs::read_to_string(ignore_list).unwrap().to_owned();
        list.split('\n')
            .map(|ln| ln.split_once('#').map(|(fname, _comment)| fname).unwrap_or(ln).trim().to_owned())
            .collect::<HashSet<String>>()
    } else {
        HashSet::new()
    };

    let mut results = parse::TranslationResults::default();

    match cli.input {
        // If Some(paths), then read file(s) and output to files (or stdout).
        Some(paths) => {
            if paths.len() == 1 {
                let path = &paths[0]; //.canonicalize().unwrap();
                if let Some(out) = &cli.output {
                    process_path(path, out, cli.force, &flatten_to, &ignore, cli.keep_histogram, &mut results).unwrap();
                } else if path.is_file() {
                    if ignore.contains(path.file_name().unwrap().to_str().unwrap()) {
                        log::warn!("skipping file as it's present in ignore list");
                    } else {
                        // print to stdout
                        let toml = std::fs::read_to_string(path).unwrap();
                        match process_toml(toml, cli.keep_histogram) {
                            Ok(output_code) => println!("{output_code}"),
                            Err(e) => log::error!("failed to translate toml from stdin {e}"),
                        };
                    }
                } else {
                    // let parent = PathBuf::from(path.parent().unwrap());
                    process_path(path, path, cli.force, &flatten_to, &ignore, cli.keep_histogram, &mut results)
                        .unwrap();
                }
            } else {
                for path in paths {
                    if let Some(out) = &cli.output {
                        process_path(
                            &path,
                            &out.join(path.file_name().unwrap()),
                            cli.force,
                            &flatten_to,
                            &ignore,
                            cli.keep_histogram,
                            &mut results,
                        )
                        .unwrap();
                    } else {
                        let parent = PathBuf::from(path.parent().unwrap());
                        process_path(&path, &parent, cli.force, &flatten_to, &ignore, cli.keep_histogram, &mut results)
                            .unwrap();
                    }
                }
            }
        }
        // If no paths, then attempt to read from stdin or print help.
        None => {
            let stdin = std::io::stdin();
            if stdin.is_terminal() {
                Cli::command().print_help().unwrap();
            } else {
                let lines: Vec<_> = stdin.lines().map(Result::unwrap).collect();
                let toml = lines.join("\n");
                let output_code = process_toml(toml, cli.keep_histogram).unwrap();
                if let Some(out) = &cli.output {
                    let mut f = std::fs::File::create(out).unwrap();
                    write!(f, "{output_code}").unwrap();
                    log::info!("successfully translated stdin -> {out:?}");
                } else {
                    println!("{output_code}");
                }
            }
        }
    }

    log_results(results);
}
