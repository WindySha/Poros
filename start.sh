#!/bin/bash
echo "[ Java And C/C++ Code Injection for Android Root Device]"

root_cmd="root"   #  adb shell su root  or adb shell su -c
is_32_bit_app=0
use_quick_mode=0
xposed_module_plugin_apk_path="null"
packageName=""
use_non_ptrace_mode=0
main_thread_injection=0
restart_app=0

while getopts ":p:f:xhqmnr" opt; do
  case $opt in
    p)
      packageName=$OPTARG
      ;;
    f)
      xposed_module_plugin_apk_path=$OPTARG
      ;;
    q) 
      use_quick_mode=1
      ;;
    m) 
      use_non_ptrace_mode=1
      ;;
    n) 
      main_thread_injection=1
      ;;
    r)
      restart_app=1
      ;;
    h) 
      echo "
Usage: ./start.sh -p package_name -f xxx.apk -h -q

Options:
  -p [package name]      the package name to be injected  
  -f [apk path]          the xposed module apk path
  -h                     display this help message
  -m                     do the injection without ptrace, using /proc/mem, only aarch64 is supported
  -n                     inject the plugin on the main thread, this is not very stable.
  -r                     restart the app before injection
  -q                     use quick mode, do not inject the xposed dex and so file, this can only used when it is not the first time to inject target app.
  "
      exit 0
      ;;
    ?) #unknown arg: ?
      echo "Invalid option: -$OPTARG"
      ;;
  esac
done

if [ "${packageName}" == "" ]; then
   echo "Exit with error, Please use './start.sh -p package_name' to specify the target injected app."
   exit 0
fi



##################  Checking whether the device is magisk Root Envriment,  Start ##########

# test if this is magisk root envriment;
test_su_cmd_result=$(adb shell su -v)
su_result=$?
#The failed result should be: su: invalid uid/gid '-v'
echo "test su console output -> $test_su_cmd_result"
echo "cmd result -> $su_result"
if [[ $su_result -eq 0 &&  $test_su_cmd_result == *MAGISKSU* ]]; then
   echo "Use command: adb shell su -c"
   root_cmd="-c"
elif [[ $su_result -eq 0 &&  $test_su_cmd_result == *KernelSU* ]]; then
   echo "Use command: adb shell su -c"
   root_cmd="-c"
else
   echo "Use command: adb shell su root"
   root_cmd="root"
fi
##################   End  ##########


##################  Checking if the target app is 32 bit  Start ##########

# Get library path and bitness
APK_INSTALLED_PATH=$(adb shell pm path $packageName | head -n 1 | sed 's/package://')
APK_INSTALLED_DIR=${APK_INSTALLED_PATH%/*}  # or:   $(dirname $APK_INSTALLED_PATH)
echo "Apk Installed Path: $APK_INSTALLED_DIR"

# Check if library path exists
if [[ -z "$APK_INSTALLED_PATH" ]]; then
  echo "Error: Installed Package not found"
  exit 1
fi

oat_arm_path_exist_result=$(adb shell "if test -d ${APK_INSTALLED_DIR}/oat/arm; then
  echo 1
else
  echo 0
fi
")

if [ $oat_arm_path_exist_result == 1 ]; then
    echo "Target App Installed Path :${APK_INSTALLED_DIR}/oat/arm exist."
    echo "Target Package: $packageName is 32Bit !!!!"
    is_32_bit_app=1
fi

if [ "${is_32_bit_app}" == 0 ]; then
  lib_arm_path_exist_result=$(adb shell "if test -d ${APK_INSTALLED_DIR}/lib/arm; then
    echo 1
  else
    echo 0
  fi
  ")

  if [ $lib_arm_path_exist_result == 1 ]; then
      echo "Target App Installed Path :${APK_INSTALLED_DIR}/lib/arm exist."
      echo "Target Package: $packageName is 32Bit !!!!"
      is_32_bit_app=1
  fi
fi

##################  End ##########



tmp_path="/data/local/tmp"

plugin_apk_file_name=${xposed_module_plugin_apk_path##*/}

if [ "${xposed_module_plugin_apk_path}" == "null" ]; then
   echo "No input xposed plugin founded, You will inject the xposed plugin injected last time !!!"
elif [ -f $xposed_module_plugin_apk_path ]; then
   adb push $xposed_module_plugin_apk_path $tmp_path
   adb shell su $root_cmd mv $tmp_path/$plugin_apk_file_name "$tmp_path/xposed_plugin.apk"
else
   echo "Plugin file not exist: $xposed_module_plugin_apk_path"
   exit 0
fi


xposed_loader_dex_path="/plugin_loader/classes.dex"
xposed_loader_so_path="/plugin_loader/libsandhook-64.so"

if [ ${is_32_bit_app} == 1 ]; then
    xposed_loader_so_path="/plugin_loader/libsandhook.so"
fi 

path_array=($xposed_loader_dex_path $xposed_loader_so_path)

work_dir=$(cd `dirname $0`; pwd)

for value in "${path_array[@]}"; do
  if [ ! -f $work_dir$value ]; then 
    echo "File $work_dir$value do not exist, please check it!!!"
    exit 1
  else
    if [ ${use_quick_mode} == 0 ]; then
      echo "Start Pushing $work_dir$value to $tmp_path"
      adb push $work_dir$value $tmp_path
    fi
  fi
done

injector_path=""
native_injector_path=""


# ${xposed_loader_dex_path##*/}   get the file name of xposed_loader_dex_path

if [ ${is_32_bit_app} == 1 ]; then
    injector_path="/injector/armeabi-v7a/xinjector"
    native_injector_path="/glue/armeabi-v7a/libinjector-glue.so"
else
    if [ ${use_non_ptrace_mode} == 1 ]; then
      injector_path="/injector/arm64-v8a/linjector-cli"
    else 
      injector_path="/injector/arm64-v8a/xinjector"
    fi
    native_injector_path="/glue/arm64-v8a/libinjector-glue.so"
fi

if [ ${use_quick_mode} == 0 ]; then
    adb push $work_dir$native_injector_path $tmp_path
fi
adb push $work_dir$injector_path $tmp_path

main_thread_injection_flag_file_path="$tmp_path/.do_main_thread_injection_flag"

if [ ${main_thread_injection} == 1 ]; then
  adb shell su $root_cmd "touch" $main_thread_injection_flag_file_path
fi

if [ ${is_32_bit_app} == 0 ] && [ ${use_non_ptrace_mode} == 1 ]; then
  adb shell su $root_cmd "chmod 755" ${tmp_path}/${injector_path##*/}
  if [ ${restart_app} == 1 ]; then
    adb shell su $root_cmd .${tmp_path}/${injector_path##*/} "-r" "-a" ${packageName} "-f" ${tmp_path}/${native_injector_path##*/}
  else
    adb shell su $root_cmd .${tmp_path}/${injector_path##*/} "-a" ${packageName} "-f" ${tmp_path}/${native_injector_path##*/}
  fi
else 
  adb shell su $root_cmd "chmod 755" ${tmp_path}/${injector_path##*/}
  if [ ${restart_app} == 1 ]; then
    adb shell su $root_cmd .${tmp_path}/${injector_path##*/} "-r" ${packageName} ${tmp_path}/${native_injector_path##*/}
  else
    adb shell su $root_cmd .${tmp_path}/${injector_path##*/} ${packageName} ${tmp_path}/${native_injector_path##*/}
  fi
fi
adb shell su $root_cmd "rm" $main_thread_injection_flag_file_path

echo "Inject xposed plugin success."

