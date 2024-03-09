package com.wind.xposed.entry.util;

import android.content.Context;
import android.content.pm.ApplicationInfo;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class XpatchUtils {

    public static Context createAppContext() {

//        LoadedApk.makeApplication()
//        ContextImpl appContext = ContextImpl.createAppContext(mActivityThread, this);

        try {
            Class activityThreadClass = Class.forName("android.app.ActivityThread");
            Method currentActivityThreadMethod = activityThreadClass.getDeclaredMethod("currentActivityThread");
            currentActivityThreadMethod.setAccessible(true);

            Object activityThreadObj = currentActivityThreadMethod.invoke(null);

            Field boundApplicationField = activityThreadClass.getDeclaredField("mBoundApplication");
            boundApplicationField.setAccessible(true);
            Object mBoundApplication = boundApplicationField.get(activityThreadObj);   // AppBindData

            Field infoField = mBoundApplication.getClass().getDeclaredField("info");   // info
            infoField.setAccessible(true);
            Object loadedApkObj = infoField.get(mBoundApplication);  // LoadedApk

            Class contextImplClass = Class.forName("android.app.ContextImpl");
            Method createAppContextMethod = contextImplClass.getDeclaredMethod("createAppContext", activityThreadClass, loadedApkObj.getClass());
            createAppContextMethod.setAccessible(true);

            Object context = createAppContextMethod.invoke(null, activityThreadObj, loadedApkObj);

            if (context instanceof Context) {
                return (Context) context;
            }

        } catch (ClassNotFoundException | NoSuchMethodException | IllegalAccessException | InvocationTargetException | NoSuchFieldException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static boolean isApkDebugable(Context context) {
        try {
            ApplicationInfo info = context.getApplicationInfo();
            return (info.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
        } catch (Exception e) {
        }
        return false;
    }

    public static final void ensurePathExist(String path) {
        File file = new File(path);
        if (!file.exists()) {
            file.mkdirs();
        }
    }

    private static String getCurrentProcessName(ApplicationInfo applicationInfo) {
        String currentProcessName = applicationInfo.packageName;
        try {
            Class activityThread_clazz = Class.forName("android.app.ActivityThread");
            Method method = activityThread_clazz.getDeclaredMethod("currentProcessName");
            method.setAccessible(true);
            currentProcessName = (String) method.invoke(null);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return currentProcessName;
    }

    public static boolean isMainProcess(Context context) {
        if (context == null) return true;
        String processName = getCurrentProcessName(context.getApplicationInfo());
        if (processName == null || processName.isEmpty()) return true;

        return context.getPackageName().equals(processName);
    }

    public static String strMd5(String input) {
        if (input == null || input.length() == 0) {
            return null;
        }
        try {
            MessageDigest md5 = MessageDigest.getInstance("MD5");
            md5.update(input.getBytes());
            byte[] byteArray = md5.digest();

            BigInteger bigInt = new BigInteger(1, byteArray);
            // 参数16表示16进制
            String result = bigInt.toString(16);
            // 不足32位高位补零
            while (result.length() < 32) {
                result = "0" + result;
            }
            return result;
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        return null;
    }

}
