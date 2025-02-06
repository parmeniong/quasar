use clap::Parser;
use std::path::PathBuf;

#[derive(Parser)]
#[command(version, about, long_about = None)]
struct Args {
    /// The input file
    file: Option<PathBuf>
}

fn main() {
    let args = Args::parse();

    if let Some(file) = args.file {
        run_file(file);
    } else {
        repl();
    }
}

fn run_file(_file: PathBuf) {
    todo!()
}

fn repl() {
    todo!()
}
