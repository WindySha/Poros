//
// Created by Windysha.
//

#pragma once

#include <jni.h>
#include <string>

namespace runtime {

    typedef std::string(*JniShortName_Func)(void* art_method);
    typedef void* (*GetCurrentMethod_Func)(void* art_method, bool, bool);
    typedef void* (*ThreadCurrent_Func)();

    // get the short name of an art method, this is  the address of art function: art::ArtMethod::JniShortName
    extern JniShortName_Func JniShortName;

    // get current caller's art method, this is the address of art function: art::Thread::GetCurrentMethod
    extern GetCurrentMethod_Func GetCurrentMethod;

    // get current thread pointer, this is the address of art function:  art::Thread::CurrentFromGdb
    extern ThreadCurrent_Func CurrentThreadFunc;

    void InitRuntime();

    JNIEnv* GetJNIEnvFromThread(void* thread_ptr);


}