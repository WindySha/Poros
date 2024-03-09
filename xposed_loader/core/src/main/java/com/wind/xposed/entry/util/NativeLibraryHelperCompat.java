package com.wind.xposed.entry.util;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Process;
import android.util.Log;

import com.wind.xposed.entry.reflect.Reflect;

import java.io.File;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class NativeLibraryHelperCompat {

    private static String TAG = NativeLibraryHelperCompat.class.getSimpleName();

    public static int copyNativeBinaries(File apkFile, File sharedLibraryDir) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return copyNativeBinariesAfterL(apkFile, sharedLibraryDir);
        } else {
            return copyNativeBinariesBeforeL(apkFile, sharedLibraryDir);
        }
    }

    public static void copyNativeBinaries2(String packageName, File libDir) {
//        File[] libDirFiles = libDir.listFiles();
//        if (libDirFiles == null || libDirFiles.length == 0) {
//            String appLibPath = null;
//            try {
//                appLibPath = HostHelper.getHostAppContext().getPackageManager()
//                        .getApplicationInfo(packageName, 0).nativeLibraryDir;
//            } catch (PackageManager.NameNotFoundException e) {
//                //
//            }
//            if (!TextUtils.isEmpty(appLibPath)) {
//                File[] libFiles = new File(appLibPath).listFiles();
//                if (libFiles != null && libFiles.length != 0) {
//                    for (File file : libFiles) {
//                        File dst = new File(libDir, file.getName());
//                        try {
//                            FileUtils.copyFile(file, dst);
//                        } catch (IOException e) {
//                            dst.delete();
//                        }
//                    }
//                }
//            }
//            libDirFiles = libDir.listFiles();
//            if (libDirFiles == null || libDirFiles.length == 0) {
//                Log.e(TAG, "copyNativeBinaries 2_1 failed!!! for" + appLibPath);
//            }
//        }
//        libDirFiles = libDir.listFiles();
//        if (libDirFiles == null || libDirFiles.length == 0) {
//            try {
//                String apkPath = null;
//                try {
//                    apkPath = HostHelper.getHostAppContext().getPackageManager()
//                            .getApplicationInfo(packageName, 0).sourceDir;
//                } catch (PackageManager.NameNotFoundException e) {
//                    //
//                }
//                if (!TextUtils.isEmpty(apkPath)) {
//                    File appSourceDir = new File(apkPath).getParentFile();
//                    File[] libFiles = appSourceDir.listFiles(new FilenameFilter() {
//                        @Override
//                        public boolean accept(File dir, String name) {
//                            return name.endsWith("apk");
//                        }
//                    });
//                    if (libFiles != null) {
//                        for (File file : libFiles) {
//                            NativeLibraryHelperCompat.copyNativeBinaries(file, libDir);
//                        }
//                    }
//                }
//                libDirFiles = libDir.listFiles();
//                if (libDirFiles == null || libDirFiles.length == 0) {
//                    Log.e(TAG, "copyNativeBinaries 2_2 failed!!! for" + apkPath);
//                }
//            } catch (Exception e) {
//                //
//            }
//        }
    }

    private static int copyNativeBinariesBeforeL(File apkFile, File sharedLibraryDir) {
        try {
            return Reflect.on(NativeLibraryHelper.TYPE).call("copyNativeBinariesIfNeededLI", apkFile, sharedLibraryDir)
                    .get();
        } catch (Throwable e) {
            e.printStackTrace();
        }
        return -1;
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private static int copyNativeBinariesAfterL(File apkFile, File sharedLibraryDir) {
        try {
            Object handle = NativeLibraryHelper.Handle.create.call(apkFile);
            if (handle == null) {
                return -1;
            }

            String abi = null;
            Set<String> abiSet = getSupportAbiList(apkFile.getAbsolutePath());
            if (abiSet == null || abiSet.isEmpty()) {
                return 0;
            }
            boolean is64Bit = is64bit();
            if (is64Bit && contain64bitAbi(abiSet)) {
                if (Build.SUPPORTED_64_BIT_ABIS.length > 0) {
                    int abiIndex = NativeLibraryHelper.findSupportedAbi.call(handle, Build.SUPPORTED_64_BIT_ABIS);
                    if (abiIndex >= 0) {
                        abi = Build.SUPPORTED_64_BIT_ABIS[abiIndex];
                    }
                }
            } else {
                if (Build.SUPPORTED_32_BIT_ABIS.length > 0) {
                    int abiIndex = NativeLibraryHelper.findSupportedAbi.call(handle, Build.SUPPORTED_32_BIT_ABIS);
                    if (abiIndex >= 0) {
                        abi = Build.SUPPORTED_32_BIT_ABIS[abiIndex];
                    }
                }
            }
            Log.e(TAG, " is64Bit=" + is64Bit + " abi = " + abi + " abiSet = " + abiSet + " sharedLibraryDir =" + sharedLibraryDir);
            if (abi == null) {
                Log.e(TAG, "Not match any abi." + apkFile.getAbsolutePath());
                return -1;
            }
            int result = NativeLibraryHelper.copyNativeBinaries.call(handle, sharedLibraryDir, abi);
            Log.e(TAG, "copyNativeBinaries result = " + result + " apkFile path = " + apkFile.getAbsolutePath());
            return result;
        } catch (Throwable e) {
            Log.d(TAG, "copyNativeBinaries with error", e);
            e.printStackTrace();
        }

        return -1;
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static boolean is64bitAbi(String abi) {
        return "arm64-v8a".equals(abi)
                || "x86_64".equals(abi)
                || "mips64".equals(abi);
    }

    public static boolean is32bitAbi(String abi) {
        return "armeabi".equals(abi)
                || "armeabi-v7a".equals(abi)
                || "mips".equals(abi)
                || "x86".equals(abi);
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static boolean contain64bitAbi(Set<String> supportedABIs) {
        for (String supportedAbi : supportedABIs) {
            if (is64bitAbi(supportedAbi)) {
                return true;
            }
        }
        return false;
    }

    public static boolean contain32bitAbi(Set<String> abiList) {
        for (String supportedAbi : abiList) {
            if (is32bitAbi(supportedAbi)) {
                return true;
            }
        }
        return false;
    }

    public static Set<String> getSupportAbiList(String apk) {
        try {
            ZipFile apkFile = new ZipFile(apk);
            Enumeration<? extends ZipEntry> entries = apkFile.entries();
            Set<String> supportedABIs = new HashSet<String>();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                String name = entry.getName();
                if (name.contains("../")) {
                    continue;
                }
                if (name.startsWith("lib/") && !entry.isDirectory() && name.endsWith(".so")) {
                    String supportedAbi = name.substring(name.indexOf("/") + 1, name.lastIndexOf("/"));
                    supportedABIs.add(supportedAbi);
                }
            }
            return supportedABIs;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return Collections.emptySet();
    }

    public static boolean is64bit() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            return false;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return Process.is64Bit();
        }
        return VMRuntime.is64Bit.call(VMRuntime.getRuntime.call());

    }
}
