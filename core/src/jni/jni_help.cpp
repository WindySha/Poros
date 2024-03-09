//
// Created by WindySha
//

#include "jni_help.h"
#include "../base/scoped_local_ref.h"
#include "../base/logging.h"


namespace jni {
    // call the object method
    static jobject
    CallObjectMethod(JNIEnv *env, jobject obj, const char *name, const char *sig, ...) {
        if (!obj) {
            return nullptr;
        }
        ScopedLocalRef<jclass> clazz(env, env->GetObjectClass(obj));
        jmethodID methodId = env->GetMethodID(clazz.get(), name, sig);;
        while (!methodId) {
            clazz.reset(env->GetSuperclass(clazz.get()));
            if (!clazz.get()) {
                break;
            }
            methodId = env->GetMethodID(clazz.get(), name, sig);
        }
        if (!methodId) {
            return nullptr;
        }
        va_list args;
        va_start(args, sig);
        jobject result = env->CallObjectMethodV(clazz.get(), methodId, args);
        va_end(args);
        return result;
    }

    static jobject
    CallStaticMethod(JNIEnv *env, jclass clazz, const char *name, const char *sig, ...) {
        if (!clazz) {
            return nullptr;
        }
        jmethodID methodId = env->GetStaticMethodID(clazz, name, sig);;
        while (!methodId) {
            clazz = env->GetSuperclass(clazz);
            if (!clazz) {
                break;
            }
            methodId = env->GetStaticMethodID(clazz, name, sig);
        }
        if (!methodId) {
            return nullptr;
        }
        va_list args;
        va_start(args, sig);
        jobject result = env->CallStaticObjectMethodV(clazz, methodId, args);
        va_end(args);
        return result;
    }

    // get the object field id
    static jfieldID GetObjectFieldId(JNIEnv *env, jobject obj, const char *name, const char *sig) {
        if (!obj) {
            return nullptr;
        }
        ScopedLocalRef<jclass> clazz(env, env->GetObjectClass(obj));
        jfieldID fieldId = env->GetFieldID(clazz.get(), name, sig);
        while (!fieldId) {
            clazz.reset(env->GetSuperclass(clazz.get()));
            if (!clazz.get()) {
                break;
            }
            fieldId = env->GetFieldID(clazz.get(), name, sig);
        }
        return fieldId;
    }

    // add object array to the obejct field
    static void
    AppendArrayObject(JNIEnv *env, jobject obj, const char *field_name, const char *sig,
                      jobject add_objects) {
        jfieldID fieldId = GetObjectFieldId(env, obj, field_name, sig);
        if (!field_name) return;

        ScopedLocalRef<jobject> field_obj(env, env->GetObjectField(obj, fieldId));

        ScopedLocalRef<jclass> field_obj_clazz(env, env->GetObjectClass(field_obj.get()));
        ScopedLocalRef<jclass> list_clazz(env, env->FindClass("java/util/List"));
        jboolean is_list_expand = env->IsAssignableFrom(field_obj_clazz.get(), list_clazz.get());
        if (is_list_expand) {
            jmethodID addAll_mid = env->GetMethodID(field_obj_clazz.get(), "addAll",
                                                    "(Ljava/util/Collection;)Z");
            env->CallBooleanMethod(field_obj.get(), addAll_mid, add_objects);
        } else {
            auto ori_array = reinterpret_cast<jobjectArray>(field_obj.get());
            auto add_array = reinterpret_cast<jobjectArray>(add_objects);

            auto ori_size = env->GetArrayLength(ori_array);
            auto add_size = env->GetArrayLength(add_array);
            auto total_size = ori_size + add_size;

            // get the to be added first element class type
            ScopedLocalRef<jobject> add_first_obj(env, env->GetObjectArrayElement(add_array, 0));
            ScopedLocalRef<jclass> add_first_clazz(env, env->GetObjectClass(add_first_obj.get()));

            jobjectArray result = env->NewObjectArray(total_size, add_first_clazz.get(), nullptr);
            for (int i = 0; i < total_size; i++) {
                if (i < ori_size) {
                    ScopedLocalRef<jobject> ori_obj(env, env->GetObjectArrayElement(ori_array, i));
                    env->SetObjectArrayElement(result, i, ori_obj.get());
                } else {
                    ScopedLocalRef<jobject> add_obj(env, env->GetObjectArrayElement(add_array,
                                                                                    i - ori_size));
                    env->SetObjectArrayElement(result, i, add_obj.get());
                }
            }

            env->SetObjectField(obj, fieldId, result);
        }
    }

    // create a java file object, like Java call:: new File(path)
    static jobject CreateJavaFileObject(JNIEnv *env, const char *path) {
        ScopedLocalRef<jstring> str(env, env->NewStringUTF(path));
        ScopedLocalRef<jclass> file_clazz(env, env->FindClass("java/io/File"));

        jmethodID constructor_mid = env->GetMethodID(file_clazz.get(), "<init>",
                                                     "(Ljava/lang/String;)V");
        return env->NewObject(file_clazz.get(), constructor_mid, str.get());
    }

    // create a java ArrayList object and add one element $obj
    // like Java call: ArrayList list = new ArrayList<File>();  list.add(obj);
    static jobject CreateFileArrayList(JNIEnv *env, jobject obj) {
        ScopedLocalRef<jclass> arrayList_clazz(env, env->FindClass("java/util/ArrayList"));
        jmethodID init_mid = env->GetMethodID(arrayList_clazz.get(), "<init>", "()V");
        jobject arrayList_obj = env->NewObject(arrayList_clazz.get(), init_mid);

        jmethodID add_mid = env->GetMethodID(arrayList_clazz.get(), "add", "(Ljava/lang/Object;)Z");
        env->CallBooleanMethod(arrayList_obj, add_mid, obj);
        return arrayList_obj;
    }

    jobject GetLoadedApkObj(JNIEnv *env) {
        ScopedLocalRef<jclass> ActivityThread_clazz(env,
                                                    env->FindClass("android/app/ActivityThread"));

        jmethodID cur_mid = env->GetStaticMethodID(ActivityThread_clazz.get(),
                                                   "currentActivityThread",
                                                   "()Landroid/app/ActivityThread;");
        ScopedLocalRef<jobject> activityThread_obj(env, env->CallStaticObjectMethod(
                ActivityThread_clazz.get(), cur_mid));

        jfieldID bindData_mid = env->GetFieldID(ActivityThread_clazz.get(), "mBoundApplication",
                                                "Landroid/app/ActivityThread$AppBindData;");
        ScopedLocalRef<jobject> bindData_obj(env, env->GetObjectField(activityThread_obj.get(),
                                                                      bindData_mid));

        ScopedLocalRef<jclass> bindData_clazz(env, env->GetObjectClass(bindData_obj.get()));
        jfieldID bindData_info_mid = env->GetFieldID(bindData_clazz.get(), "info",
                                                     "Landroid/app/LoadedApk;");

        return env->GetObjectField(bindData_obj.get(), bindData_info_mid);
    }

    jobject CreateAppContext(JNIEnv *env) {
        ScopedLocalRef<jobject> app_loaded_apk_obj(env, GetLoadedApkObj(env));
        ScopedLocalRef<jclass> contextimpl_clazz(env, env->FindClass("android/app/ContextImpl"));

        ScopedLocalRef<jclass> ActivityThread_clazz(env,
                                                    env->FindClass("android/app/ActivityThread"));

        jmethodID cur_mid = env->GetStaticMethodID(ActivityThread_clazz.get(),
                                                   "currentActivityThread",
                                                   "()Landroid/app/ActivityThread;");
        ScopedLocalRef<jobject> activityThread_obj(env, env->CallStaticObjectMethod(
                ActivityThread_clazz.get(), cur_mid));

        jmethodID createAppContext_mid = env->GetStaticMethodID(contextimpl_clazz.get(),
                                                                "createAppContext",
                                                                "(Landroid/app/ActivityThread;Landroid/app/LoadedApk;)Landroid/app/ContextImpl;");
        jobject context_obj = env->CallStaticObjectMethod(contextimpl_clazz.get(),
                                                          createAppContext_mid,
                                                          activityThread_obj.get(),
                                                          app_loaded_apk_obj.get());

        return context_obj;
    }

    std::string GetPackageName(JNIEnv* env) {
        ScopedLocalRef<jobject> app_context(env, CreateAppContext(env));

        if (app_context.get() == nullptr) {
            LOGE(" Can not create app context !!!");
            return { "" };
        }

        ScopedLocalRef<jclass> context_clazz(env, env->FindClass("android/content/Context"));

        jmethodID getPackageName_mid = env->GetMethodID(context_clazz.get(), "getPackageName", "()Ljava/lang/String;");
        ScopedLocalRef<jstring> package_name_jobj(env, (jstring)env->CallObjectMethod(app_context.get(), getPackageName_mid));
        jboolean isCopy;
        const char* package_name_str = env->GetStringUTFChars(package_name_jobj.get(), &isCopy);
        std::string result{ package_name_str };
        env->ReleaseStringUTFChars(package_name_jobj.get(), package_name_str);
        return result;
    }

    void BypassHiddenApiByRefection(JNIEnv* env) {
        const char* VMRuntime_class_name = "dalvik/system/VMRuntime";
        ScopedLocalRef<jclass> vmRumtime_class(env, env->FindClass(VMRuntime_class_name));
        void* getRuntime_art_method = env->GetStaticMethodID(vmRumtime_class.get(),
            "getRuntime",
            "()Ldalvik/system/VMRuntime;");
        ScopedLocalRef<jobject> vmRuntime_instance(env, env->CallStaticObjectMethod(vmRumtime_class.get(), (jmethodID)getRuntime_art_method));

        const char* target_char = "L";
        ScopedLocalRef<jstring> mystring(env, env->NewStringUTF(target_char));
        ScopedLocalRef<jclass> cls(env, env->FindClass("java/lang/String"));
        ScopedLocalRef<jobjectArray> jarray(env, env->NewObjectArray(1, cls.get(), nullptr));
        env->SetObjectArrayElement(jarray.get(), 0, mystring.get());

        void* setHiddenApiExemptions_art_method = env->GetMethodID(vmRumtime_class.get(),
            "setHiddenApiExemptions",
            "([Ljava/lang/String;)V");
        if (setHiddenApiExemptions_art_method) {
            env->CallVoidMethod(vmRuntime_instance.get(), (jmethodID)setHiddenApiExemptions_art_method, jarray.get());
        }
        else {
            LOGE("setHiddenApiExemptions method id is not founded !!!");
        }
    }

    jobject GetAppClassLoader(JNIEnv *env) {
        ScopedLocalRef<jobject> loadedApk_obj(env, GetLoadedApkObj(env));
        ScopedLocalRef<jclass> loadedApk_clazz(env, env->GetObjectClass(loadedApk_obj.get()));
        jmethodID getClassLoader_mid = env->GetMethodID(loadedApk_clazz.get(), "getClassLoader",
                                                        "()Ljava/lang/ClassLoader;");

        return env->CallObjectMethod(loadedApk_obj.get(), getClassLoader_mid);
    }

    bool MergeDexAndSoToClassLoader(JNIEnv *env, const char *dex_path, const char *so_dir) {
        LOGD("Start merge dex and so into the appClassLoader.");
        ScopedLocalRef<jobject> classloader_obj(env, GetAppClassLoader(env));
        if (!classloader_obj.get()) return false;

        ScopedLocalRef<jclass> pathclassloader_clazz(env, env->FindClass(
                "dalvik/system/PathClassLoader"));

        ScopedLocalRef<jclass> classloader_clazz(env, env->FindClass("java/lang/ClassLoader"));
        jmethodID getParent_mid = env->GetMethodID(classloader_clazz.get(), "getParent",
                                                   "()Ljava/lang/ClassLoader;");

        // Some app's plugin framework may change the app classloader, so try to find the real PathClassLoader
        while (classloader_obj.get() != nullptr) {
            ScopedLocalRef<jclass> clazz(env, env->GetObjectClass(classloader_obj.get()));

            if (!env->IsSameObject(clazz.get(), pathclassloader_clazz.get())) {
                // get the parent of the classloader
                classloader_obj.reset(env->CallObjectMethod(classloader_obj.get(), getParent_mid));
            } else {
                break;
            }
        }
        if (!classloader_obj.get()) return false;

        jfieldID pathList_fid = GetObjectFieldId(env, classloader_obj.get(), "pathList",
                                                 "Ldalvik/system/DexPathList;");

        jobject pathList_obj = env->GetObjectField(classloader_obj.get(), pathList_fid);
        ScopedLocalRef<jclass> pathList_clazz(env, env->GetObjectClass(pathList_obj));

        int api_level = android_get_device_api_level();

        jobject so_path_obj = CreateJavaFileObject(env, so_dir);
        jobject so_path_arrayList_obj = CreateFileArrayList(env, so_path_obj);
        if (api_level > __ANDROID_API_L_MR1__) {
            AppendArrayObject(env, pathList_obj, "nativeLibraryDirectories", "Ljava/util/List;",
                              so_path_arrayList_obj);
        } else {
            jobjectArray so_path_array_obj = env->NewObjectArray(1, env->FindClass("java/io/File"),
                                                                 so_path_obj);
            AppendArrayObject(env, pathList_obj, "nativeLibraryDirectories", "[Ljava/io/File;",
                              so_path_array_obj);
            env->DeleteLocalRef(so_path_array_obj);
        }

        if (api_level > __ANDROID_API_N_MR1__) {
            jobject NativeLibraryElement_obj = CallStaticMethod(env, pathList_clazz.get(),
                                                                "makePathElements",
                                                                "(Ljava/util/List;)[Ldalvik/system/DexPathList$NativeLibraryElement;",
                                                                so_path_arrayList_obj);
            AppendArrayObject(env, pathList_obj, "nativeLibraryPathElements",
                              "[Ldalvik/system/DexPathList$NativeLibraryElement;",
                              NativeLibraryElement_obj);
            env->DeleteLocalRef(NativeLibraryElement_obj);
        } else if (api_level > __ANDROID_API_L_MR1__) {
            jobject element_array_obj = CallStaticMethod(env, pathList_clazz.get(), "makePathElements",
                                                         "(Ljava/util/List;Ljava/io/File;Ljava/util/List;)[Ldalvik/system/DexPathList$Element;",
                                                         so_path_arrayList_obj,
                                                         nullptr, nullptr);
            AppendArrayObject(env, pathList_obj, "nativeLibraryPathElements",
                              "[Ldalvik/system/DexPathList$Element;", element_array_obj);
            env->DeleteLocalRef(element_array_obj);
        }

        jobject dex_path_obj = CreateJavaFileObject(env, dex_path);
        jobject dex_path_arrayList_obj = CreateFileArrayList(env, dex_path_obj);
        jobject element_array_obj;
        if (api_level > __ANDROID_API_L_MR1__) {
            element_array_obj = CallStaticMethod(env, pathList_clazz.get(), "makePathElements",
                                                 "(Ljava/util/List;Ljava/io/File;Ljava/util/List;)[Ldalvik/system/DexPathList$Element;",
                                                 dex_path_arrayList_obj,
                                                 nullptr, nullptr);
        } else {
            element_array_obj = CallStaticMethod(env, pathList_clazz.get(), "makeDexElements",
                                                 "(Ljava/util/ArrayList;Ljava/io/File;Ljava/util/ArrayList;)[Ldalvik/system/DexPathList$Element;",
                                                 dex_path_arrayList_obj,
                                                 nullptr, nullptr);
        }
        AppendArrayObject(env, pathList_obj, "dexElements",
                          "[Ldalvik/system/DexPathList$Element;", element_array_obj);
        env->DeleteLocalRef(element_array_obj);
        env->DeleteLocalRef(dex_path_obj);
        env->DeleteLocalRef(dex_path_arrayList_obj);
        env->DeleteLocalRef(so_path_obj);
        env->DeleteLocalRef(so_path_arrayList_obj);
        env->DeleteLocalRef(pathList_obj);
        return true;
    }

    void CallStaticMethodByJavaMethodInvoke(JNIEnv* env, const char* class_name, const char* method_name) {
        ScopedLocalRef<jobject> app_class_loader(env, jni::GetAppClassLoader(env));

        const char* classloader_class_name = "java/lang/ClassLoader";
        const char* class_class_name = "java/lang/Class";
        const char* method_class_name = "java/lang/reflect/Method";

        // Class<?> clazz = appClassLoader.loadClass("class_name");
        ScopedLocalRef<jclass> classloader_jclass(env, env->FindClass(classloader_class_name));
        auto loadClass_mid = env->GetMethodID(classloader_jclass.get(), "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        ScopedLocalRef<jstring> class_name_jstr(env, env->NewStringUTF(class_name));
        ScopedLocalRef<jobject> clazz_obj(env, env->CallObjectMethod(app_class_loader.get(), loadClass_mid, class_name_jstr.get()));


        // get Java Method mid: Class.getDeclaredMethod()
        ScopedLocalRef<jclass> class_jclass(env, env->FindClass(class_class_name));
        auto getDeclaredMethod_mid = env->GetMethodID(class_jclass.get(), "getDeclaredMethod",
            "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");

        // Get the Method object
        ScopedLocalRef<jstring> method_name_jstr(env, env->NewStringUTF(method_name));
        jvalue args[] = { {.l = method_name_jstr.get()},{.l = nullptr} };
        ScopedLocalRef<jobject> method_obj(env, env->CallObjectMethodA(clazz_obj.get(), getDeclaredMethod_mid, args));

        ScopedLocalRef<jclass> method_jclass(env, env->FindClass(method_class_name));
        // get Method.invoke jmethodId
        auto invoke_mid = env->GetMethodID(method_jclass.get(), "invoke",
            "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");


        //Call Method.invoke()
        jvalue args2[] = { {.l = nullptr},{.l = nullptr} };
        env->CallObjectMethodA(method_obj.get(), invoke_mid, args2);
    }
}