#!/bin/bash
echo "Build project script."
echo "First, build xposed loader library; then copy injector so to output path, then build core."

rm -rf output
mkdir output

rm -rf cli/assets
mkdir cli/assets

cd xposed_loader
if [ ! -f "local.properties" ]; then
  echo "create file : local.properties"
  echo "sdk.dir=${HOME}/Library/Android/sdk" > local.properties
fi

./gradlew clean assembleDebug

cd ..

mkdir output/plugin_loader
mkdir cli/assets/plugin_loader
cp ./xposed_loader/app/build/intermediates/dex/debug/mergeDexDebug/classes.dex output/plugin_loader
cp ./xposed_loader/app/build/intermediates/dex/debug/mergeDexDebug/classes.dex cli/assets/plugin_loader

cp ./xposed_loader/app/build/intermediates/stripped_native_libs/debug/out/lib/arm64-v8a/libsandhook.so output/plugin_loader/libsandhook-64.so
cp ./xposed_loader/app/build/intermediates/stripped_native_libs/debug/out/lib/arm64-v8a/libsandhook.so cli/assets/plugin_loader/libsandhook-64.so

cp ./xposed_loader/app/build/intermediates/stripped_native_libs/debug/out/lib/armeabi-v7a/libsandhook.so output/plugin_loader/libsandhook.so
cp ./xposed_loader/app/build/intermediates/stripped_native_libs/debug/out/lib/armeabi-v7a/libsandhook.so cli/assets/plugin_loader/libsandhook.so


mkdir output/injector
mkdir cli/assets/injector
cp -r injector/arm64-v8a output/injector
cp -r injector/arm64-v8a cli/assets/injector

cp -r injector/armeabi-v7a output/injector
cp -r injector/armeabi-v7a cli/assets/injector


./core/build.sh
mkdir output/glue
mkdir cli/assets/glue
cp -r core/output/arm64-v8a output/glue
cp -r core/output/arm64-v8a cli/assets/glue

cp -r core/output/armeabi-v7a output/glue
cp -r core/output/armeabi-v7a cli/assets/glue

cp start.sh output/start.sh

cargo build --release --manifest-path cli/Cargo.toml

echo "Build success, build files is copied to output directory"

