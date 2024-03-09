//
// Created by Windysha.
//

#include "atrace_func_hook.h"
#include "base/utils.h"
#include "base/got_hook.h"
#include "base/logging.h"

namespace poros {

#ifdef __LP64__
    static const char* libcutils_path = "/system/lib64/libcutils.so";
    static const char* libandroid_runtime_path = "/system/lib64/libandroid_runtime.so";
#else
    static const char* libcutils_path = "/system/lib/libcutils.so";
    static const char* libandroid_runtime_path = "/system/lib/libandroid_runtime.so";
#endif

    static OnAtraceUpdateCallback atrace_func_callback = nullptr;

    // void atrace_set_debuggable(bool debuggable)  android 5 - 11
    static void (*original_atrace_set_debuggable)(bool debuggable);
    static void atrace_set_debuggable_proxy(bool debuggable) {
        LOGD(" %s is called !", __FUNCTION__);
        original_atrace_set_debuggable(debuggable);

        if (atrace_func_callback) {
            atrace_func_callback();
        }
    }

    // void atrace_update_tags()    android 12,13 14
    static void (*original_atrace_update_tags)();
    static void atrace_update_tag_proxy() {
        LOGD(" %s is called !", __FUNCTION__);
        original_atrace_update_tags();

        if (atrace_func_callback) {
            atrace_func_callback();
        }
    }

    void HookAtraceFunctions(OnAtraceUpdateCallback callback) {
        atrace_func_callback = callback;

        int api_level = poros::GetDeviceApiLevel();
        if (api_level >= 31) {
            int result = poros::startGotHook(libcutils_path, libandroid_runtime_path, "atrace_update_tags",
                (void*)atrace_update_tag_proxy,
                (void**)&original_atrace_update_tags);

        }
        else {
            int result = poros::startGotHook(libcutils_path, libandroid_runtime_path, "atrace_set_debuggable",
                (void*)atrace_set_debuggable_proxy,
                (void**)&original_atrace_set_debuggable);

        }
    }

}