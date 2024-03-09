package com.wind.xposed.entry.util;


import com.wind.xposed.entry.reflect.MethodParams;
import com.wind.xposed.entry.reflect.RefClass;
import com.wind.xposed.entry.reflect.RefStaticMethod;

import java.io.File;

public class NativeLibraryHelper {
    public static Class<?> TYPE = RefClass
            .load(NativeLibraryHelper.class, "com.android.internal.content.NativeLibraryHelper");

    @MethodParams({Handle.class, File.class, String.class})
    public static RefStaticMethod<Integer> copyNativeBinaries;

    @MethodParams({Handle.class, String[].class})
    public static RefStaticMethod<Integer> findSupportedAbi;

    public static class Handle {
        public static Class<?> TYPE =
                RefClass.load(Handle.class, "com.android.internal.content.NativeLibraryHelper$Handle");

        @MethodParams({File.class})
        public static RefStaticMethod<Object> create;
    }
}
