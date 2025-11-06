# Poros

Poros is a dynamic instrumentation tool for android developers, reverse-engineers, similar in functionality to Frida.
It utilizes the Xposed module to facilitate the injection of Java and Native codes on rooted Android devices.


## Features
- Support Java hook using Xposed api.
- Support loading native libraries in Xposed modules.
- Support arm64 and arm32.
- Support running command on multi platform (macOs, Windows, Linux).
- Easy to use: execute only one command to do the injection.
- Injection is completed during the early stage of the app's launch.

## Usage
Two ways to use the tool:
1. Use the executable binary file on MacOS, Linux or Windows;
2. Use the shell file on MacOS or Linux;
### Using executable binary file
1. Download the executable binary file on the github release page.
2. Execute the command line file.

for example:
```
$ poros-Darwin-x86_64 -p com.android.settings -f ./xposed_module_sample.apk
```
use `-h` to get the help doc:
```
$ poros-Darwin-x86_64 -h
```
```
Usage: poros-Darwin-x86_64 [OPTIONS] --package-name <PACKAGE_NAME>

Options:
  -p, --package-name <PACKAGE_NAME>  target application's package name
  -f, --file-path <FILE_PATH>        path of the xposed plugin to inject
  -q, --quick                        whether use the quick mode
  -n, --non-ptrace                   whether use the non-ptrace mode
  -h, --help                         Print help
  -V, --version                      Print version
```
### Using shell file
1. Download the `poros_shell_cmd.tar.gz` file on the release page, and unzip it;
2. Open terminal and change directory to the unzipped file path;
3. Execute this command to inject the xposed module into the Settings Application:
```
./start.sh -p com.android.settings  -f ./xposed_module_sample.apk
```
4. Use the `-q` parameter for a quicker injection. Use this parameter when injecting into the same app for the second time to enhance injection performance.

  
This is a xposed module sample project that contains the Java hook and native hook:  
[XposedModuleSample](https://github.com/WindySha/XposedModuleSample)

## Build
Two ways to build:
1. Build the project using the github action.
2. Build it locally:
> - Clone this project.
> - Change parameters `ANDROID_NDK_HOME` in file `/core/build.sh` to the android ndk directory.
> - Run the file `build.sh` to build this the whole project.
> - shell command file is in the `output` directory, the executable command line tool is the this directory: `cli/target/release/poros`, either can do the injection.

## How it works
[Android Root环境下动态注入Java和Native代码的实践](https://windysha.github.io/2024/03/22/%E4%B8%80%E7%A7%8DAndroid-Root%E7%8E%AF%E5%A2%83%E4%B8%8B%E5%8A%A8%E6%80%81%E6%B3%A8%E5%85%A5Java%E5%92%8CNative%E4%BB%A3%E7%A0%81%E7%9A%84%E6%96%B9%E6%A1%88/)

## References 
- [SandHook](https://github.com/asLody/SandHook)
- [XInjector](https://github.com/WindySha/XInjector)
- [xposed_module_loader](https://github.com/WindySha/xposed_module_loader)

## Licence
This project is licensed under the Apache License 2.0.