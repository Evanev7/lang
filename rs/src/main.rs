#![allow(unused)]
use std::path::{Path, PathBuf};

macro_rules! ASM {
    () => {
        "
    .globl main
main:
    movl ${}, %eax
    ret
"
    };
}

#[derive(Debug)]
struct Args {
    source: PathBuf,
    dest: PathBuf,
}
impl Args {
    fn parse() -> Result<Self, String> {
        // first arg is untrustworthy.
        let mut args = std::env::args_os().skip(1);
        let mut jc = Default::default();
        args.next()
            .map(|it| {
                jc = it.clone();
                it.into()
            })
            .and_then(|it: PathBuf| it.canonicalize().ok())
            .and_then(|source| {
                source
                    .parent()
                    .map(|it| it.join("out.s"))
                    .map(|dest| Self { source, dest })
            })
            .ok_or(
                (if jc == "" {
                    "u a bozo".to_owned()
                } else {
                    format!("there's no way that {jc:?} is real")
                }),
            )
    }
}

fn main() -> Result<(), String> {
    let args = Args::parse()?;
    println!("{}", format!(ASM!(), "2"));
    Ok(())
}
