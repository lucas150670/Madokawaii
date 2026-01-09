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

	const char* GetWorkingDirectory() { return ::GetWorkingDirectory(); }

    void ToggleFullscreen() {
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
        SetWindowPosition(0, 0);
    }

    int GetFileLength(const char* fileName)
    {
        return ::GetFileLength(fileName);
    }

    int GetScreenWidth() {
        return ::GetScreenWidth();
    }

    int GetScreenHeight() {
        return ::GetScreenHeight();
    }

    bool IsAnyKeyPressed() {
        // raylib 提供 GetKeyPressed() 返回按下的键码，0表示无按键
        return ::GetKeyPressed() != 0;
    }
}
