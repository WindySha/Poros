use std::path::PathBuf;

use anyhow::Result;
use subprocess::{Exec, Redirection};

use include_dir::{include_dir, Dir};

static PROJECT_DIR: Dir<'_> = include_dir!("$CARGO_MANIFEST_DIR/assets");

pub fn execute(cmd: impl Into<String>) -> Result<(bool, String)> {
    let command = cmd.into();
    // println!(" cmd: {}", command);
    let data = Exec::shell(command)
        .stdout(Redirection::Pipe)
        .capture()?;
    Ok((data.success(), data.stdout_str()))
}

pub fn extract_assets(assets_dir: &PathBuf) -> Result<()> {
    if assets_dir.exists() {
        std::fs::remove_dir_all(&assets_dir)?;
    }
    std::fs::create_dir_all(&assets_dir)?;
    PROJECT_DIR.extract(&assets_dir)?;
    Ok(())
}
