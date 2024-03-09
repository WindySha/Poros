//
// Created by WindySha
//

#pragma once

#include <jni.h>
#include <string>

namespace jni {
    bool MergeDexAndSoToClassLoader(JNIEnv *env, const char *dex_path, const char *so_dir);

    jobject GetLoadedApkObj(JNIEnv *env);

    jobject GetAppClassLoader(JNIEnv *env);

    jobject CreateAppContext(JNIEnv *env);

    std::string GetPackageName(JNIEnv* env);

    void BypassHiddenApiByRefection(JNIEnv* env);

    void CallStaticMethodByJavaMethodInvoke(JNIEnv* env, const char* class_name, const char* method_name);
}