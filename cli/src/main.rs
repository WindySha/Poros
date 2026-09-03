use std::path::Path;

use anyhow::{Error, Ok};
use clap::Parser;

use crate::utils::{execute, extract_assets};
use anyhow::Result;

mod utils;

/// Inject xposed plugin into a application
#[derive(Parser, Debug)]
#[command(version, about)]
struct Args {
    /// target application's package name
    #[arg(short = 'p', long)]
    package_name: String,

    /// path of the xposed plugin to inject
    #[arg(short = 'f', long)]
    file_path: Option<String>,

    /// whether use the quick mode
    #[arg(short = 'q', long)]
    quick: bool,

    /// whether use the non-ptrace mode, using /proc/mem, only aarch64 is supported
    #[arg(short = 'n', long)]
    non_ptrace: bool,

    /// whether inject the plugin on the main thread,  this is not very stable.
    #[arg(short = 'm', long)]
    main_thread_injection: bool,

    /// whether restart the app before injection
    #[arg(short = 'r', long)]
    restart: bool,

    /// serial number of the target device, the same as `adb -s <SERIAL>`,
    /// it can be ignored when only one device is connected
    #[arg(short = 's', long)]
    serial: Option<String>,
}

fn main() -> Result<()> {
    let args = Args::parse();

    let adb = &adb_command(&args.serial);

    if !is_device_connected(adb) {
        return Err(Error::msg(device_connection_error(&args.serial)));
    }

    let root_cmd = get_root_cmd(adb);

    let is_32_bit_app = is_32_bit_app(adb, &args.package_name);
    println!(" is_32_bit_app = {}", is_32_bit_app);

    let device_tmp_path = "/data/local/tmp";

    if let Some(plugin_path) = args.file_path {
        let path = Path::new(&plugin_path);
        if !path.exists() {
            return Err(Error::msg("Plugin file not exists"));
        }
        let Some(plugin_apk_file_name) = path.file_name() else {
            return Err(Error::msg("Plugin file name not found"));
        };

        execute(format!("{} push {} {}", adb, plugin_path, device_tmp_path))?;
        execute(format!(
            "{} shell su {} mv {}/{} {}/xposed_plugin.apk",
            adb,
            root_cmd,
            device_tmp_path,
            plugin_apk_file_name.to_str().unwrap(),
            device_tmp_path
        ))?;
    } else {
        println!("No input xposed plugin founded, You will inject the xposed plugin injected last time !!!")
    }

    if !args.quick {
        let working_dir = tempfile::tempdir()?;
        let working_path = working_dir.path();
        println!(" working_path = {}", working_path.display());
        let assets_file_path = working_path.join("assets");

        extract_assets(&assets_file_path)?;

        let xposed_loader_dex_path = assets_file_path.join("plugin_loader/classes.dex");
        let mut xposed_loader_so_path = assets_file_path.join("plugin_loader/libsandhook-64.so");
        if is_32_bit_app {
            xposed_loader_so_path = assets_file_path.join("plugin_loader/libsandhook.so");
        }
        // push dex and so file into the device
        execute(format!(
            "{} push {} {}",
            adb,
            xposed_loader_dex_path.to_string_lossy(),
            device_tmp_path
        ))?;
        execute(format!(
            "{} push {} {}",
            adb,
            xposed_loader_so_path.to_string_lossy(),
            device_tmp_path
        ))?;

        let injector_path;
        let native_injector_path;

        if is_32_bit_app {
            injector_path = assets_file_path.join("injector/armeabi-v7a/xinjector");
            native_injector_path = assets_file_path.join("glue/armeabi-v7a/libinjector-glue.so");
        } else {
            if args.non_ptrace {
                injector_path = assets_file_path.join("injector/arm64-v8a/linjector-cli");
            } else {
                injector_path = assets_file_path.join("injector/arm64-v8a/xinjector");
            }
            native_injector_path = assets_file_path.join("glue/arm64-v8a/libinjector-glue.so");
        }
        // push injector and injected file into the device
        execute(format!(
            "{} push {} {}",
            adb,
            injector_path.to_string_lossy(),
            device_tmp_path
        ))?;
        execute(format!(
            "{} push {} {}",
            adb,
            native_injector_path.to_string_lossy(),
            device_tmp_path
        ))?;
    }

    let main_thread_injection_flag_file_path = format!("{}/.do_main_thread_injection_flag", device_tmp_path);

    if args.main_thread_injection {
        // create do_main_thread_injection_flag file to indicate that we do the injection on the main thread.
        execute(format!(
            "{} shell su {} touch {}",
            adb, root_cmd, main_thread_injection_flag_file_path
        ))?;
    }

    // execute the injector command
    if !is_32_bit_app && args.non_ptrace {
        execute(format!(
            "{} shell su {} chmod 755 {}/linjector-cli",
            adb, root_cmd, device_tmp_path
        ))?;
        let restart_flag = if args.restart { "-r " } else { "" };
        execute(format!(
            "{} shell su {} .{}/linjector-cli {}-a {} -f {}/libinjector-glue.so",
            adb, root_cmd, device_tmp_path, restart_flag, args.package_name, device_tmp_path
        ))?;
    } else {
        execute(format!(
            "{} shell su {} chmod 755 {}/xinjector",
            adb, root_cmd, device_tmp_path
        ))?;
        if args.restart {
            execute(format!(
                "{} shell su {} .{}/xinjector -r {} {}/libinjector-glue.so",
                adb, root_cmd, device_tmp_path, args.package_name, device_tmp_path
            ))?;
        } else {
            execute(format!(
                "{} shell su {} .{}/xinjector {} {}/libinjector-glue.so",
                adb, root_cmd, device_tmp_path, args.package_name, device_tmp_path
            ))?;
        }
    }
    execute(format!(
        "{} shell su {} rm {}",
        adb, root_cmd, main_thread_injection_flag_file_path
    ))?;
    Ok(())
}

// adb   or   adb -s <serial>
fn adb_command(serial: &Option<String>) -> String {
    match serial {
        Some(serial) => format!("adb -s {}", serial),
        None => "adb".to_string(),
    }
}

// list the serial numbers of all the connected devices
fn connected_devices() -> Vec<String> {
    let std::result::Result::Ok((true, output)) = execute("adb devices") else {
        return Vec::new();
    };
    output
        .lines()
        .skip(1)
        .filter_map(|line| {
            let mut columns = line.split_whitespace();
            let serial = columns.next()?;
            // skip the empty lines, a valid line is like: "emulator-5554    device"
            columns.next()?;
            Some(serial.to_string())
        })
        .collect()
}

fn device_connection_error(serial: &Option<String>) -> String {
    if let Some(serial) = serial {
        return format!("Failed to connect to device: {}", serial);
    }
    let devices = connected_devices();
    if devices.len() > 1 {
        return format!(
            "More than one device connected: {}, please specify one with -s <SERIAL>",
            devices.join(", ")
        );
    }
    "Failed to connect to device".to_string()
}

fn is_device_connected(adb: &str) -> bool {
    let std::result::Result::Ok(state) = execute(format!("{} get-state", adb)) else {
        return false;
    };
    return state.0 && state.1.trim() == "device";
}

// adb shell su -c   or   adb shell su root
fn get_root_cmd(adb: &str) -> String {
    let su_result = execute(format!("{} shell su -v", adb)).unwrap_or_else(|_| {
        return (false, "unknow".to_string());
    });

    return if su_result.0 && (su_result.1.contains("MAGISKSU") || su_result.1.contains("KernelSU"))
    {
        println!("Use command: adb shell su -c");
        "-c".to_string()
    } else {
        println!("Use command: adb shell su root");
        "root".to_string()
    };
}

fn is_32_bit_app(adb: &str, package_name: &str) -> bool {
    let pm_path_result =
        execute(format!("{} shell pm path {}", adb, package_name)).unwrap_or_else(|_| {
            return (false, "".to_string());
        });

    if !pm_path_result.0 || pm_path_result.1.is_empty() {
        println!("Package: {} not found.", package_name);
    }

    let apk_installed_path = pm_path_result.1.replace("package:", "").trim().to_string();

    let path = Path::new(&apk_installed_path);

    let Some(parent) = path.parent() else {
        println!("No parent directory");
        return false;
    };
    let installed_directory = parent.to_str().unwrap();

    let mut is_32_bit_app = false;

    let command = format!(
        "{} shell 'if [ -d {}/lib/oat ]; then echo \"exists\"; else echo \"not exists\"; fi'",
        adb, installed_directory
    );

    let dir_exist_result = execute(command).unwrap_or((false, "".to_string()));

    if dir_exist_result.0 && dir_exist_result.1.trim() == "exists" {
        is_32_bit_app = true;
    } else {
        let command = format!(
            "{} shell 'if [ -d {}/lib/arm ]; then echo \"exists\"; else echo \"not exists\"; fi'",
            adb, installed_directory
        );
        let dir_exist_result = execute(command).unwrap_or((false, "".to_string()));
        if dir_exist_result.0 && dir_exist_result.1.trim() == "exists" {
            is_32_bit_app = true;
        }
    }
    return is_32_bit_app;
}
