cmake_minimum_required(VERSION 3.10.2)

project("injector-glue")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(src_path ${CMAKE_CURRENT_SOURCE_DIR}/src)

aux_source_directory(${src_path}  SRC_MAIN)
aux_source_directory(${src_path}/base  BASE_MAIN)


add_library(injector-glue
        SHARED
        ${SRC_MAIN}
        ${BASE_MAIN}
        ${src_path}/dlfcn/fake_dlfcn.c
        ${src_path}/jni/jni_help.cpp
        ${src_path}/looper/ThreadLooper.cpp
        )

find_library(log-lib
        log)

target_link_libraries(injector-glue
        ${log-lib}
        android
        )

# set the target so file's output path
if (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/armeabi-v7a) 
elseif (${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/arm64-v8a) 
endif ()


