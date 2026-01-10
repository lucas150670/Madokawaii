//
// Created by madoka on 2026/1/10.
//

#include <jni.h>
#include <android/log.h>

JavaVM* g_vm = nullptr;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    __android_log_print(ANDROID_LOG_INFO,  "Madokawaii", "JNI: GetEnv Succeed");
    return JNI_VERSION_1_6;
}

namespace Madokawaii::Platform::Core {

    std::string GetInternalCachePath() {
        if (!g_vm) return {};

        JNIEnv* env = nullptr;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
                return {};
            }
        }

        // 获取 Application 对象
        jclass activityThread = env->FindClass("android/app/ActivityThread");
        jmethodID currentApp = env->GetStaticMethodID(activityThread,
                                                      "currentApplication",
                                                      "()Landroid/app/Application;");
        jobject app = env->CallStaticObjectMethod(activityThread, currentApp);

        if (app == nullptr) return {};

        jclass contextClass = env->GetObjectClass(app);
        jmethodID midGetCacheDir = env->GetMethodID(contextClass,
                                                    "getCacheDir",
                                                    "()Ljava/io/File;");
        jobject fileObj = env->CallObjectMethod(app, midGetCacheDir);

        if (fileObj == nullptr) return {};

        jclass fileClass = env->GetObjectClass(fileObj);
        jmethodID midGetAbsPath = env->GetMethodID(fileClass,
                                                   "getAbsolutePath",
                                                   "()Ljava/lang/String;");
        auto pathStr = (jstring)env->CallObjectMethod(fileObj, midGetAbsPath);

        const char* cachePathChars = env->GetStringUTFChars(pathStr, nullptr);
        std::string cachePath(cachePathChars);
        env->ReleaseStringUTFChars(pathStr, cachePathChars);

        return cachePath;
    }

}
