//
// Created by madoka on 2026/1/10.
//

#include <jni.h>
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Madokawaii", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Madokawaii", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "Madokawaii", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "Madokawaii", __VA_ARGS__)

JavaVM* g_vm = nullptr;
jobject g_context = nullptr;  // 全局 Context 引用

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    __android_log_print(ANDROID_LOG_INFO,  "Madokawaii", "JNI: GetEnv Succeed");
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_raylib_raymob_NativeLoader_initContext(JNIEnv *env, jobject thiz, jobject context) {
    // TODO: implement initContext()
    if (g_context != nullptr) {
        env->DeleteGlobalRef(g_context);
    }
    g_context = env->NewGlobalRef(context);
}

static bool checkException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return true;
    }
    return false;
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

    int GetMonitorRefreshRate(int monitor) {
        (void)monitor;
        JNIEnv* env = nullptr;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
                return -1.0f;
            }
        }

        float refreshRate = -1.0f;
        bool needDetach = false;

        int status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            needDetach = true;
        }

        do {
            jclass contextClass = env->FindClass("android/content/Context");
            if (checkException(env) || contextClass == nullptr) {
                LOGE("Failed to find Context class");
                break;
            }

            jfieldID windowServiceField = env->GetStaticFieldID(contextClass, "WINDOW_SERVICE", "Ljava/lang/String;");
            if (checkException(env) || windowServiceField == nullptr) {
                LOGE("Failed to get WINDOW_SERVICE field");
                break;
            }

            auto windowService = (jstring)env->GetStaticObjectField(contextClass, windowServiceField);

            jmethodID getSystemService = env->GetMethodID(contextClass, "getSystemService",
                                                          "(Ljava/lang/String;)Ljava/lang/Object;");
            if (checkException(env) || getSystemService == nullptr) {
                LOGE("Failed to get getSystemService method");
                break;
            }

            jobject windowManager = env->CallObjectMethod(g_context, getSystemService, windowService);
            if (checkException(env) || windowManager == nullptr) {
                LOGE("Failed to get WindowManager");
                break;
            }

            jclass windowManagerClass = env->FindClass("android/view/WindowManager");
            if (checkException(env) || windowManagerClass == nullptr) {
                LOGE("Failed to find WindowManager class");
                break;
            }

            // 调用 getDefaultDisplay()
            jmethodID getDefaultDisplay = env->GetMethodID(windowManagerClass, "getDefaultDisplay",
                                                           "()Landroid/view/Display;");
            if (checkException(env) || getDefaultDisplay == nullptr) {
                LOGE("Failed to get getDefaultDisplay method");
                break;
            }

            jobject display = env->CallObjectMethod(windowManager, getDefaultDisplay);
            if (checkException(env) || display == nullptr) {
                LOGE("Failed to get Display");
                break;
            }

            jclass displayClass = env->FindClass("android/view/Display");
            if (checkException(env) || displayClass == nullptr) {
                LOGE("Failed to find Display class");
                break;
            }

            jmethodID getRefreshRate = env->GetMethodID(displayClass, "getRefreshRate", "()F");
            if (checkException(env) || getRefreshRate == nullptr) {
                LOGE("Failed to get getRefreshRate method");
                break;
            }

            refreshRate = env->CallFloatMethod(display, getRefreshRate);
            if (checkException(env)) {
                LOGE("Failed to call getRefreshRate");
                refreshRate = -1.0f;
                break;
            }

            LOGI("Screen refresh rate: %.2f Hz", refreshRate);

        } while (false);

        if (needDetach) {
            g_vm->DetachCurrentThread();
        }

        return std::floor(refreshRate);
    }


    int GetScreenWidth() {
        bool needDetach = false;
        JNIEnv* env = nullptr;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
                return -1.0f;
            }
        }

        int status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            needDetach = true;
        }

        if (env == nullptr || g_context == nullptr) {
            LOGE("JNIEnv or Context is null");
            return -1;
        }

        int screenWidth = -1;

        do {
            jclass contextClass = env->FindClass("android/content/Context");
            if (checkException(env) || contextClass == nullptr) break;

            jmethodID getResources = env->GetMethodID(contextClass, "getResources",
                                                      "()Landroid/content/res/Resources;");
            jobject resources = env->CallObjectMethod(g_context, getResources);
            if (checkException(env) || resources == nullptr) break;

            jclass resourcesClass = env->FindClass("android/content/res/Resources");
            jmethodID getDisplayMetrics = env->GetMethodID(resourcesClass, "getDisplayMetrics",
                                                           "()Landroid/util/DisplayMetrics;");
            jobject displayMetrics = env->CallObjectMethod(resources, getDisplayMetrics);
            if (checkException(env) || displayMetrics == nullptr) break;

            jclass displayMetricsClass = env->FindClass("android/util/DisplayMetrics");
            jfieldID widthPixelsField = env->GetFieldID(displayMetricsClass, "widthPixels", "I");
            if (checkException(env) || widthPixelsField == nullptr) break;

            screenWidth = env->GetIntField(displayMetrics, widthPixelsField);
        } while (false);

        if (needDetach) {
            g_vm->DetachCurrentThread();
        }

        return screenWidth;
    }

    int GetScreenHeight() {
        bool needDetach = false;
        JNIEnv* env = nullptr;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
                return -1.0f;
            }
        }

        int status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            needDetach = true;
        }

        if (env == nullptr || g_context == nullptr) {
            LOGE("JNIEnv or Context is null");
            return -1;
        }

        int screenHeight = -1;

        do {
            jclass contextClass = env->FindClass("android/content/Context");
            if (checkException(env) || contextClass == nullptr) break;

            jmethodID getResources = env->GetMethodID(contextClass, "getResources",
                                                      "()Landroid/content/res/Resources;");
            jobject resources = env->CallObjectMethod(g_context, getResources);
            if (checkException(env) || resources == nullptr) break;

            jclass resourcesClass = env->FindClass("android/content/res/Resources");
            jmethodID getDisplayMetrics = env->GetMethodID(resourcesClass, "getDisplayMetrics",
                                                           "()Landroid/util/DisplayMetrics;");
            jobject displayMetrics = env->CallObjectMethod(resources, getDisplayMetrics);
            if (checkException(env) || displayMetrics == nullptr) break;

            jclass displayMetricsClass = env->FindClass("android/util/DisplayMetrics");
            jfieldID heightPixelsField = env->GetFieldID(displayMetricsClass, "heightPixels", "I");
            if (checkException(env) || heightPixelsField == nullptr) break;

            screenHeight = env->GetIntField(displayMetrics, heightPixelsField);

        } while (false);

        if (needDetach) {
            g_vm->DetachCurrentThread();
        }

        return screenHeight;
    }


}