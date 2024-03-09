# 简介
这是App加载已安装的Xposed Modules的一个库。  
其中，
> * app : App Sample代码，应用启动后，就会默认加载系统里所有已安装的Xposed Modules;  
> * core : 加载代码，主要逻辑是读取所有已安装的Xposed Module，并构建classloader加载其入口代码。

# 使用
只需要在Application中加入：
```
XposedModuleEntry.init()
```

# Hook框架
Xposed hook库默认使用的是[SandHook](https://github.com/ganyao114/SandHook)  
假如Hook失败，也可以尝试更改为[whale](https://github.com/asLody/whale), 只需要修改core/build.gradle文件：
```
 // api 'com.swift.sandhook:hooklib:3.6.0'
 // api 'com.swift.sandhook:xposedcompat:3.6.0'

 api 'com.wind.xposed:xposed-on-whale:0.1.1'
```
并去掉SandHook初始化代码即可。

# 应用
二次打包工具[Xpatch](https://github.com/WindySha/Xpatch), 植入Apk中的dex以及so文件都是由此工程编译生成。

# Note
App需要获取读写外部存储的权限后，才能使用`mnt/sdcard/xposed_config/modules.list`文件来控制Xposed模块的加载  
否则会默认加载全部Xposed模块，具体使用方法请参考[Xpatch](https://github.com/WindySha/Xpatch)的Readme文档
