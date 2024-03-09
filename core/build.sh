#!/bin/bash
echo "use cmake to build project"

android_sdk_path="${HOME}/Library/Android/sdk"
ndk_version="22.0.7026061"
cmake_version="3.10.2.4988404"
android_ndk_path="$android_sdk_path/ndk/$ndk_version"
cur_dir=$(cd `dirname $0`; pwd)

echo "cur_dir : $cur_dir"

abi_arm64="arm64-v8a"
abi_arm32="armeabi-v7a"

if [ ! -d "$cur_dir/build" ]; then
  mkdir "$cur_dir/build"
fi

build_type_array=($abi_arm32 $abi_arm64)

for value in "${build_type_array[@]}"; do
  build_dir=$cur_dir/build/$value
  if [ ! -d "$build_dir" ]; then
    mkdir "$build_dir"
  fi
  
  # Enter build directory to build, build files will be created here
  cd $build_dir
  
  echo "start build $value"
  
  $android_sdk_path/cmake/$cmake_version/bin/cmake \
    -DANDROID_ABI=$value \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-s" \
    -DCMAKE_CXX_FLAGS="-s" \
    -DCMAKE_TOOLCHAIN_FILE=$android_ndk_path/build/cmake/android.toolchain.cmake \
    -DANDROID_NDK=$android_ndk_path \
    -DCMAKE_ANDROID_NDK=$android_ndk_path \
    -DANDROID_PLATFORM=android-21 \
    -DANDROID_TOOLCHAIN=clang \
    -DCMAKE_GENERATOR="Ninja" \
    -DCMAKE_MAKE_PROGRAM=$android_sdk_path/cmake/$cmake_version/bin/ninja \
    -DANDROID_STL=c++_static \
    $cur_dir \
  
  $android_sdk_path/cmake/$cmake_version/bin/ninja
done

