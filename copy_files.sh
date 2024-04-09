#!/bin/bash
# This script is only used in the github action command. See build.yml

echo "Start Copy project files script."

rm -rf output
mkdir output

mkdir output/plugin_loader
cp ./xposed_loader/app/build/intermediates/dex/debug/mergeDexDebug/classes.dex output/plugin_loader
cp ./xposed_loader/app/build/intermediates/stripped_native_libs/debug/out/lib/arm64-v8a/libsandhook.so output/plugin_loader/libsandhook-64.so
cp ./xposed_loader/app/build/intermediates/stripped_native_libs/debug/out/lib/armeabi-v7a/libsandhook.so output/plugin_loader/libsandhook.so

mkdir output/injector
cp -r injector/arm64-v8a output/injector
cp -r injector/armeabi-v7a output/injector

mkdir output/glue
cp -r core/output/arm64-v8a output/glue
cp -r core/output/armeabi-v7a output/glue

cp start.sh output/start.sh

# this is used for publish
tar -czvf poros_shell_cmd.tar.gz -C output/ .

# the output directoty will be uploaded, so do not need file this any more
rm -rf output/start.sh

echo "Copy files succeed!"

