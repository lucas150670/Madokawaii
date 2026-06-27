//
// Created by madoka on 25-9-16.
//


#include <cstdio>
#include <cstring>
#include <raylib.h>

#include <fast_io.h>

#include "Madokawaii/platform/core.hpp"
#include "raylib_runtime.hpp"

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Madokawaii", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Madokawaii", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "Madokawaii", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "Madokawaii", __VA_ARGS__)
#endif

namespace Madokawaii::Platform::Raylib {

namespace {
RuntimeInfo runtimeInfo{};
}

RuntimeInfo& GetRuntimeInfo() {
    return runtimeInfo;
}

void LogCallback(int msgType, const char *text, va_list args)
{
    char buffer[1024] = {};
    vsnprintf(buffer, sizeof(buffer), text, args);
    std::string_view buffer_view(buffer);
    auto& info = GetRuntimeInfo();

    if (strstr(buffer, "Vendor:")) {
        strcpy(info.vendor.data(), buffer + strlen("    > Vendor:   "));
    }
    if (strstr(buffer, "Renderer:")) {
        strcpy(info.renderer.data(), buffer + strlen("    > Renderer: "));
    }
    if (strstr(buffer, "Version:")) {
        strcpy(info.version.data(), buffer + strlen("    > Version:  "));
    }

    switch (msgType)
    {
#if !defined(PLATFORM_ANDROID)
        case LOG_INFO: fast_io::io::print(fast_io::out(), "INFO:  "); break;
        case LOG_WARNING: fast_io::io::print(fast_io::out(), "WARN:  "); break;
        case LOG_ERROR: fast_io::io::print(fast_io::out(), "ERROR: "); break;
        case LOG_DEBUG: fast_io::io::print(fast_io::out(), "DEBUG: "); break;
        default: break;
    }
    fast_io::io::println(fast_io::out(), buffer_view);
#else
        case LOG_INFO: LOGI("%s\n", buffer); break;
        case LOG_WARNING: LOGW("%s\n", buffer); break;
        case LOG_ERROR: LOGE("%s\n", buffer); break;
        case LOG_DEBUG: LOGD("%s\n", buffer); break;
        default: break;
    }
    fast_io::io::println(fast_io::out(), buffer_view);
#endif
}

} // namespace Madokawaii::Platform::Raylib

int main(int argc, char *argv[]) {
    SetTraceLogCallback(Madokawaii::Platform::Raylib::LogCallback);

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