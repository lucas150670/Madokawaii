//
// Created by madoka on 25-9-11.
//

#include <raylib.h>
#include "Madokawaii/platform/core.h"

namespace Madokawaii::Platform::Core {
    bool FileExists(const char *path) { return ::FileExists(path); }

    unsigned char *LoadFileData(const char *path, int *fileSize) { return ::LoadFileData(path, fileSize); }

    char *LoadFileText(const char *path) { return ::LoadFileText(path); }

    void UnloadFileData(unsigned char *data) { ::UnloadFileData(data); }

    void UnloadFileText(char *text) { ::UnloadFileText(text); }

    int GetMonitorCount() { return ::GetMonitorCount(); }

    int GetCurrentMonitor() { return ::GetCurrentMonitor(); }

    int GetMonitorRefreshRate(int monitor) { return ::GetMonitorRefreshRate(monitor); }

    void InitWindow(int width, int height, const char *title) { ::InitWindow(width, height, title); }

    bool WindowShouldClose() { return ::WindowShouldClose(); }

    void CloseWindow() { ::CloseWindow(); }
}
