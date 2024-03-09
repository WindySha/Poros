package com.wind.xposed.entry;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;

import com.tendcloud.tenddata.TalkingDataSDK;
import com.wind.xposed.entry.util.DeviceIdUtil;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public class StatisticReport {

    public static void reportDeviceInfo(Context context, String injectorVersionCode) {
        String package_name = context.getPackageName();
        String device_id = DeviceIdUtil.getDeviceId(context);
        int api_level = Build.VERSION.SDK_INT;
        String device_model = Build.MODEL;
        String device_brand = Build.BRAND;

        SimpleDateFormat formatter = new SimpleDateFormat("yyyy年MM月dd日 HH:mm:ss");
        Date curDate = new Date(System.currentTimeMillis());//获取当前时间
        String current_time = formatter.format(curDate);

        String time_simple = (new SimpleDateFormat("MM_dd_HH:mm:ss")).format(curDate);

        boolean is_64Bit = is64Bit(context);

        int version_code = 0;
        String version_name = "";
        try {
            PackageInfo packageInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            version_code = packageInfo.versionCode;
            version_name = packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }

        int len  = device_id.length();
        String did_sub = device_id.substring(0, Math.min(len, 6));
        String combined_infos = did_sub + "_" + api_level + "_" + device_model + "_" + package_name + "_" + version_name + "_" + is_64Bit;

        Map<String, Object> data = new HashMap<>();
        data.put("package_name", package_name);
        data.put("device_id", device_id);
        data.put("api_level", api_level);
        data.put("device_model", device_model);
        data.put("device_brand", device_brand);
        data.put("current_time", current_time);
        data.put("is_64Bit", is_64Bit);
        data.put("version_code", version_code);
        data.put("version_name", version_name);
        data.put("injector_version", injectorVersionCode);
        data.put("total_device_infos", combined_infos);
        data.put("total_time_and_device_infos", time_simple + "_" + combined_infos);

        TalkingDataSDK.onEvent(context, "device_and_app_info", data);
    }

    public static boolean is64Bit(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return android.os.Process.is64Bit();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            ApplicationInfo appInfo = context.getApplicationInfo();
            String nativeLibDir = appInfo.nativeLibraryDir;
            if (nativeLibDir.contains("64")) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    public static void reportPluginInfo(Context context, String pluginPackageName, int copyNativeLibResult, boolean hasNativeLibFile) {
        Map<String, Object> data = new HashMap<>();

        data.put("plugin_package_name", pluginPackageName);
        data.put("copy_native_lib_result", copyNativeLibResult);
        data.put("has_native_lib", hasNativeLibFile);

        TalkingDataSDK.onEvent(context, "plugin_info", data);
    }

    public static void reportInstalledPluginInfo(Context context, String pluginPackageName) {
        Map<String, Object> data = new HashMap<>();

        data.put("installed_plugin_package_name", pluginPackageName);
        TalkingDataSDK.onEvent(context, "plugin_info", data);
    }
}
