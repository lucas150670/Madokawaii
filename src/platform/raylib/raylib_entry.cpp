//
// Created by madoka on 25-9-16.
//


#include <cstdio>
#include <cstring>
#include <raylib.h>

#include "Madokawaii/platform/core.h"

char gVendor[256] = {0};
char gRenderer[256] = {0};
char gVersion[256] = {0};

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Madokawaii", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Madokawaii", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "Madokawaii", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "Madokawaii", __VA_ARGS__)
#endif

void LogCallback(int msgType, const char *text, va_list args)
{
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    if (strstr(buffer, "Vendor:")) {
        strcpy(gVendor, buffer + strlen("    > Vendor:   "));
    }
    if (strstr(buffer, "Renderer:")) {
        strcpy(gRenderer, buffer + strlen("    > Renderer: "));
    }
    if (strstr(buffer, "Version:")) {
        strcpy(gVersion, buffer + strlen("    > Version:  "));
    }

    switch (msgType)
    {
#if !defined(PLATFORM_ANDROID)
        case LOG_INFO: printf("INFO:  "); break;
        case LOG_WARNING: printf("WARN:  "); break;
        case LOG_ERROR: printf("ERROR: "); break;
        case LOG_DEBUG: printf("DEBUG: "); break;
        default: break;
    }
    printf("%s\n", buffer);
#else
        case LOG_INFO: LOGI("%s\n", buffer); break;
        case LOG_WARNING: LOGW("%s\n", buffer); break;
        case LOG_ERROR: LOGE("%s\n", buffer); break;
        case LOG_DEBUG: LOGD("%s\n", buffer); break;
        default: break;
    }
    printf("%s\n", buffer);
#endif
}

int main(int argc, char *argv[]) {
    SetTraceLogCallback(LogCallback);

    void ** appstate = new void *;
    int retval = AppInit(*appstate);
    if (!retval) {
        delete appstate;
        return retval;
    }
    while (true) {
        retval = AppIterate(*appstate);
        if (!retval) {
            break;
        }
    }
    // Clear up *appstate - AppExit do this.
    AppExit(*appstate);
    delete appstate;
    return 0;
}