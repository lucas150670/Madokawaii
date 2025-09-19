// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/platform/platform_common.h
// Usage: Common interfaces used by main app.
// ============================================================

#ifndef MADOKAWAII_PLATFORM_COMMON_H
#define MADOKAWAII_PLATFORM_COMMON_H

namespace Madokawaii::Platform::Core {
    // Common operation, like file io, thread create, etc
    bool FileExists(const char *path);

    unsigned char *LoadFileData(const char *path, int *fileSize);

    char *LoadFileText(const char *path);

    void UnloadFileData(unsigned char *data);

    void UnloadFileText(char *text);

    // window management
    int GetMonitorCount();

    int GetCurrentMonitor();

    int GetMonitorRefreshRate(int monitor);

    void InitWindow(int width, int height, const char *title);

    bool WindowShouldClose();

    void CloseWindow();
}

extern "C" {
void AppInit(void *appstate);

/**
 * @brief App Iterate Callback
 * @param appstate Global AppState
 * @return 0... breaks the iteration.
 */
int AppIterate(void *appstate);

void AppExit(void *appstate);
}

#endif //MADOKAWAII_PLATFORM_COMMON_H
