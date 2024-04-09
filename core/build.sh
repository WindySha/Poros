#!/bin/bash
echo "use cmake to build project"

# ANDROID_NDK_HOME="${HOME}/Library/Android/sdk/ndk/22.0.7026061"
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
  echo ${ANDROID_NDK_HOME}

  cmake \
    -DANDROID_ABI=$value \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
    -DANDROID_NDK=$ANDROID_NDK_HOME \
    -DANDROID_PLATFORM=android-21 \
    -DANDROID_STL=c++_static \
    $cur_dir \

  cmake --build .
  
done

