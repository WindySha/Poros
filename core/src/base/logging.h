#include <android/log.h>

#define  POROS_LOG_TAG "Poros_Tag"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, POROS_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, POROS_LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, POROS_LOG_TAG, __VA_ARGS__)
