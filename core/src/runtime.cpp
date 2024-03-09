//
// Created by Windysha.
//

#include "runtime.h"
#include "base/utils.h"
#include "base/scoped_fake_dlopen.h"

namespace runtime {

#define Thread_CurrentFromGdb_Symbol  "_ZN3art6Thread14CurrentFromGdbEv"
#define JniShortName_Symbol           "_ZN3art9ArtMethod12JniShortNameEv"
#define GetCurrentMethod_Symbol       "_ZNK3art6Thread16GetCurrentMethodEPjbb"

    JniShortName_Func JniShortName = nullptr;
    GetCurrentMethod_Func GetCurrentMethod = nullptr;
    ThreadCurrent_Func CurrentThreadFunc = nullptr;

    static bool has_inited = false;

    void InitRuntime() {
        if (has_inited) {
            return;
        }
        has_inited = true;

        ScopedFakeDlopen handle{ "libart.so" };

        GetCurrentMethod = reinterpret_cast<GetCurrentMethod_Func>(fake_dlsym(handle.get(), GetCurrentMethod_Symbol));
        JniShortName = reinterpret_cast<JniShortName_Func>(fake_dlsym(handle.get(), JniShortName_Symbol));
        CurrentThreadFunc = reinterpret_cast<ThreadCurrent_Func>(fake_dlsym(handle.get(), Thread_CurrentFromGdb_Symbol));
    }

    JNIEnv* GetJNIEnvFromThread(void* thread_ptr) {
        int step = 4;
        JNIEnv* jni_env = nullptr;
        int ptr_length = sizeof(void*);
        int max_count = 100;
        for (int i = 0; i < max_count; i++) {
            void* jni_env_ptr = *(void**)((uintptr_t)thread_ptr + i * step);
            void** thread_self = (void**)((uintptr_t)jni_env_ptr + ptr_length);
            if (poros::IsValidPtr(thread_self) && *thread_self == thread_ptr) {
                jni_env = reinterpret_cast<JNIEnv*>(jni_env_ptr);
                break;
            }
        }
        return jni_env;
    }
}