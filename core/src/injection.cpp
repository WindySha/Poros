//
// Created by Windysha.
//

#include "injection.h"
#include "runtime.h"
#include "base/logging.h"
#include "jni/jni_help.h"
#include "atrace_func_hook.h"
#include "looper/ThreadLooper.h"
#include <string>
#include "base/file_utils.h"
#include "base/utils.h"
#include "constants.h"
#include<thread>
#include <unistd.h>

namespace poros {

    static ThreadLooper* main_thread_looper = nullptr;
    static bool sHasInjectedSuccess = false;
    static constexpr bool kDoInjectionInMainThread = false;

    static void LoadXposedModules(JNIEnv* env) {
        jni::CallStaticMethodByJavaMethodInvoke(env, XPOSED_LOADER_ENTRY_CLASS_NAME, XPOSED_LOADER_ENTRY_METHOD_NAME);
    }

    static void InjectXposedLibraryInternal(JNIEnv* env) {
        LOGD("Start Inject Xposed Library.");
        if (sHasInjectedSuccess) return;
        sHasInjectedSuccess = true;

        int api_level = poros::GetDeviceApiLevel();
        if (api_level >= 28) {  // android P
            jni::BypassHiddenApiByRefection(env);
        }

        std::string package_name = jni::GetPackageName(env);
        LOGD("Android Debug Injection, start insert dex and so into the application: %s", package_name.c_str());

        auto xposed_injection_apth = "/data/data/" + package_name + "/xposed_injection";
        poros::mk_all_dir(xposed_injection_apth.c_str());

        auto dex_apth = "/data/data/" + package_name + "/xposed_injection/classes.dex";
        const char* temp_dex_file_path = "/data/local/tmp/classes.dex";
        poros::copy_file(temp_dex_file_path, dex_apth.c_str());

        // 修改文件权限为不可写
        if (chmod(dex_apth.c_str(), S_IRUSR | S_IRGRP | S_IROTH) != 0) {
            LOGE("Failed to change dex file to not writable.");
        }

        std::string so_apth;
        const char* temp_sandhook_so_path;
        if (poros::Is64BitRuntime()) {
            so_apth = "/data/data/" + package_name + "/xposed_injection/arm64";
            temp_sandhook_so_path = TEMP_ART_HOOK_LIBRARY_SO_FILE_PATH_64;
        }
        else {
            so_apth = "/data/data/" + package_name + "/xposed_injection/arm";
            temp_sandhook_so_path = TEMP_ART_HOOK_LIBRARY_SO_FILE_PATH;
        }

        std::string dst_sandhook_so_path = so_apth + "/" + ART_HOOK_LIBRARY_SO_NAME;
        poros::mk_all_dir(so_apth.c_str());

        poros::copy_file(temp_sandhook_so_path, dst_sandhook_so_path.c_str());

        auto target_plugin_apk_dir = "/data/data/" + package_name + "/xposed_injection/plugin";
        poros::mk_all_dir(target_plugin_apk_dir.c_str());

        auto target_plugin_apk_path = target_plugin_apk_dir + "/xposed_plugin.apk";
        const char* temp_plugin_apk_path = "/data/local/tmp/xposed_plugin.apk";
        std::remove(target_plugin_apk_path.c_str());
        poros::copy_file(temp_plugin_apk_path, target_plugin_apk_path.c_str());

        // 修改文件权限为不可写
        if (chmod(target_plugin_apk_path.c_str(), S_IRUSR | S_IRGRP | S_IROTH) != 0) {
            LOGE("Failed to change apk plugin file to not writable.");
        }

        jni::MergeDexAndSoToClassLoader(env, dex_apth.c_str(), so_apth.c_str());

        LoadXposedModules(env);
    }

    static void OnAtraceFuncCalled() {
        void* current_thread_ptr = runtime::CurrentThreadFunc();
        JNIEnv* env = runtime::GetJNIEnvFromThread(current_thread_ptr);

        if (!env) {
            LOGE("Failed to get JNIEnv, Inject Xposed Module failed.");
            return;
        }
        InjectXposedLibraryInternal(env);
    }

    // deprecated, inject Xposed module on another thread.
    static void InjectXposedLibraryAsync(JNIEnv* jni_env) {
        LOGD(" Inject Xposed Library from work thread.");
        JavaVM* javaVm;
        jni_env->GetJavaVM(&javaVm);
        std::thread worker([javaVm]() {
            JNIEnv* env;
            javaVm->AttachCurrentThread(&env, nullptr);

            int count = 0;
            while (true && count < 10000) {
                jobject app_loaded_apk_obj = jni::GetLoadedApkObj(env);
                LOGD(" loadedApk obj is -> %p wait count -> %d", app_loaded_apk_obj, count);
                // wait here until loaded apk object is available
                if (app_loaded_apk_obj == nullptr) {
                    usleep(100);
                }
                else {
                    break;
                }
                count++;
            }

            InjectXposedLibraryInternal(env);

            if (env) {
                javaVm->DetachCurrentThread();
            }
        });
        worker.detach();
    }

    static void InjectXposedLibraryByHandler(JNIEnv* env) {
        LOGD(" Inject Xposed Library from Main Thread Handler.");
        if (!main_thread_looper) {
            main_thread_looper = new ThreadLooper(
                [](void* param) -> int {
                    InjectXposedLibraryInternal(reinterpret_cast<JNIEnv*>(param));
                    if (main_thread_looper) {
                        delete main_thread_looper;
                    }
                    main_thread_looper = nullptr;
                    return 0;
                }
            );
        }
        main_thread_looper->SendMessage(env);
    }

    int DoInjection(JNIEnv* env) {
        LOGE("DoInjection!!");

        runtime::InitRuntime();

        void* current_thread_ptr = nullptr;
        if (!env) {
            current_thread_ptr = runtime::CurrentThreadFunc();
            env = runtime::GetJNIEnvFromThread(current_thread_ptr);
        }
        if (!env) {
            LOGE("Failed to get JNIEnv !!");
            return -1;
        }

        // Do the injection in main thread is still an experiment yet, it may cause crashes, so we do it in another thread
        if (!kDoInjectionInMainThread) {
            InjectXposedLibraryAsync(env);
        } else {
            // When ptrace reaches the JNI method Java_java_lang_Object_wait, issues of env->FindClass getting stuck due to waiting for locks may occur. Here, the Xposed module is loaded with a delay.
            // When ptrace reaches the JNI method Java_com_android_internal_os_ClassLoaderFactory_createClassloaderNamespace, the ClassLoader is being constructed, and calling FindClass at this instance can lead to a deadlock
            // To bypass these two methods, tasks are sent to the main thread's Handler for execution.
            void* art_method = runtime::GetCurrentMethod(current_thread_ptr, false, false);
            if (art_method != nullptr) {
                std::string name = runtime::JniShortName(art_method);
                LOGD("Currrent ptraced jni method name is : %s ", name.c_str());
                if (strcmp(name.c_str(), "Java_java_lang_Object_wait") == 0
                    || strcmp(name.c_str(), "Java_com_android_internal_os_ClassLoaderFactory_createClassloaderNamespace") == 0
                    || strcmp(name.c_str(), "Java_dalvik_system_DexFile_openDexFileNative") == 0) {
                    // load xposed modules after in the main message handler, this is later than application's attachBaseContext and onCreate method.
                    InjectXposedLibraryByHandler(env);
                    // InjectXposedLibraryAsync(env);  // like Frida, inject in another thread.
                    return 0;
                }
            }

            // If the inject time is very early, then, the loadedapk info and the app classloader is not ready, so we try to hook atrace_set_debuggable function to make sure 
            // the injection is early enough and the classloader has also been created.
            jobject loaded_apk_obj = jni::GetLoadedApkObj(env);
            LOGD("Try to get the app loaded apk info, loadedapk jobject: %p", loaded_apk_obj);

            if (loaded_apk_obj == nullptr) {
                // load xposed modules after atrace_set_debuggable or atrace_update_tags is called.
                HookAtraceFunctions(OnAtraceFuncCalled);
            }             else {
                // loadedapk and classloader is ready, so load the xposed modules directly.
                InjectXposedLibraryInternal(env);
            }
        }
        return 0;
    }
}