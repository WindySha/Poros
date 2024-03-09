package com.wind.xposed.entry.util;

import com.wind.xposed.entry.reflect.MethodParams;
import com.wind.xposed.entry.reflect.RefClass;
import com.wind.xposed.entry.reflect.RefMethod;
import com.wind.xposed.entry.reflect.RefStaticMethod;
import com.wind.xposed.entry.reflect.RefMethod;

/**
 *
 */

public class VMRuntime {
    public static Class<?> TYPE = RefClass.load(VMRuntime.class, "dalvik.system.VMRuntime");
    public static RefStaticMethod<Object> getRuntime;
    @MethodParams({int.class})
    public static RefMethod<Void> setTargetSdkVersion;
    public static RefMethod<Boolean> is64Bit;
    @MethodParams({String.class})
    public static RefStaticMethod<Boolean> is64BitAbi;
    public static RefStaticMethod<String> getCurrentInstructionSet;
}
