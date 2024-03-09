# Poros

Poros is a dynamic instrumentation tool for android developers, reverse-engineers, similar in functionality to Frida.
It utilizes the Xposed module to facilitate the injection of Java and Native codes on rooted Android devices.


## Features
- Support Java hook using Xposed api.
- Support loading native libraries in Xposed modules.
- Support arm64 and arm32.
- Easy to use: execute only one command to do the injection.
- Injection is completed during the early stage of the app's launch.

## Usage
1. Download the zip file on the macOS, unzip it;
2. Open terminal and change directory to the unzipped file path;
3. Execute this command to inject the xposed module into the Settings Application:
```
./start.sh -p com.android.settings  -f ./xposed_module_sample.apk
```
4. Use the `-q` parameter for a quicker injection. Use this parameter when injecting into the same app for the second time to enhance injection performance.

  
This is a xposed module sample project that contains the Java hook and native hook:  
[XposedModuleSample](https://github.com/WindySha/XposedModuleSample)

## Build
Mark: Only supports macOS currently.
1. Clone this project.
2. Change parameters `android_sdk_path` and `ndk_version` in file `/core/build.sh` to your android sdk directory and ndk version.
3. Run the shell file `build.sh` to build this project.

## How it works
TODO

## References 
- [SandHook](https://github.com/asLody/SandHook)
- [XInjector](https://github.com/WindySha/XInjector)
- [xposed_module_loader](https://github.com/WindySha/xposed_module_loader)

## Licence
This project is licensed under the Apache License 2.0.