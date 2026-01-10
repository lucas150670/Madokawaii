//
// Created by madoka on 2026/1/10.
//
// ============================================================
// Project: Madokawaii
// Route: src/platform/platform_android.cpp
// Usage: Android-specific platform implementation
// ============================================================

#include "Madokawaii/platform/core.h"

#if defined(PLATFORM_ANDROID)

#include <raylib.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#define LOGD(... ) __android_log_print(ANDROID_LOG_DEBUG, "Madokawaii", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Madokawaii", __VA_ARGS__)

extern "C" struct android_app* GetAndroidApp();

namespace {
    // 缓存内部存储路径
    char internalDataPath[512] = {0};
    char externalDataPath[512] = {0};
    bool pathsInitialized = false;

    void InitStoragePaths() {
        if (pathsInitialized) return;

        struct android_app* app = GetAndroidApp();
        if (app && app->activity) {
            if (app->activity->internalDataPath) {
                strncpy(internalDataPath, app->activity->internalDataPath, sizeof(internalDataPath) - 1);
                LOGD("Internal data path:  %s", internalDataPath);
            }

            if (app->activity->externalDataPath) {
                strncpy(externalDataPath, app->activity->externalDataPath, sizeof(externalDataPath) - 1);
                LOGD("External data path: %s", externalDataPath);
            }
        }

        pathsInitialized = true;
    }

    bool IsAssetPath(const char* path) {
        return (strncmp(path, "assets/", 7) == 0 || strncmp(path, "/assets/", 8) == 0);
    }

    const char* GetAssetRelativePath(const char* path) {
        if (strncmp(path, "/assets/", 8) == 0) return path + 8;
        if (strncmp(path, "assets/", 7) == 0) return path + 7;
        return path;
    }

    bool AssetExists(const char* path) {
        struct android_app* app = GetAndroidApp();
        if (!app || !app->activity || !app->activity->assetManager) {
            LOGE("AssetManager not available");
            return false;
        }

        const char* assetPath = GetAssetRelativePath(path);
        AAsset* asset = AAssetManager_open(
                app->activity->assetManager,
                assetPath,
                AASSET_MODE_UNKNOWN
        );

        if (asset) {
            AAsset_close(asset);
            LOGD("Asset exists: %s", assetPath);
            return true;
        }

        LOGD("Asset not found: %s", assetPath);
        return false;
    }

    bool RegularFileExists(const char* path) {
        struct stat buffer{};
        int result = stat(path, &buffer);

        if (result == 0) {
            bool isFile = S_ISREG(buffer.st_mode);
            LOGD("File check: %s - exists=%d, isFile=%d", path, true, isFile);
            return isFile;
        }

        LOGD("File not found: %s (errno=%d:  %s)", path, errno, strerror(errno));
        return false;
    }
}

namespace Madokawaii::Platform::Core {

    bool FileExists(const char* path) {
        if (!path || path[0] == '\0') {
            LOGD("FileExists: empty path");
            return false;
        }

        LOGD("FileExists checking: %s", path);

        if (IsAssetPath(path)) {
            return AssetExists(path);
        }

        if (path[0] == '/') {
            return RegularFileExists(path);
        }

        InitStoragePaths();

        char fullPath[1024];

        if (externalDataPath[0] != '\0') {
            snprintf(fullPath, sizeof(fullPath), "%s/%s", externalDataPath, path);
            if (RegularFileExists(fullPath)) {
                return true;
            }
        }

        if (internalDataPath[0] != '\0') {
            snprintf(fullPath, sizeof(fullPath), "%s/%s", internalDataPath, path);
            if (RegularFileExists(fullPath)) {
                return true;
            }
        }

        return AssetExists(path);
    }

    unsigned char* LoadFileData(const char* path, int* fileSize) {
        if (!path || ! fileSize) return nullptr;

        *fileSize = 0;
        LOGD("LoadFileData: %s", path);

        if (IsAssetPath(path)) {
            struct android_app* app = GetAndroidApp();
            if (!app || !app->activity || !app->activity->assetManager) {
                LOGE("AssetManager not available");
                return nullptr;
            }

            const char* assetPath = GetAssetRelativePath(path);
            AAsset* asset = AAssetManager_open(
                    app->activity->assetManager,
                    assetPath,
                    AASSET_MODE_BUFFER
            );

            if (! asset) {
                LOGE("Failed to open asset: %s", assetPath);
                return nullptr;
            }

            off_t size = AAsset_getLength(asset);
            auto* data = (unsigned char*)malloc(size + 1);

            if (data) {
                int bytesRead = AAsset_read(asset, data, size);
                if (bytesRead == size) {
                    data[size] = '\0';
                    *fileSize = (int)size;
                    LOGD("Loaded asset: %s (%d bytes)", assetPath, *fileSize);
                } else {
                    LOGE("Failed to read asset completely: %s", assetPath);
                    free(data);
                    data = nullptr;
                }
            }

            AAsset_close(asset);
            return data;
        }

        const char* actualPath = path;
        char fullPath[1024];

        if (path[0] != '/') {
            InitStoragePaths();

            if (externalDataPath[0] != '\0') {
                snprintf(fullPath, sizeof(fullPath), "%s/%s", externalDataPath, path);
                if (RegularFileExists(fullPath)) {
                    actualPath = fullPath;
                }
            }

            if (actualPath == path && internalDataPath[0] != '\0') {
                snprintf(fullPath, sizeof(fullPath), "%s/%s", internalDataPath, path);
                if (RegularFileExists(fullPath)) {
                    actualPath = fullPath;
                }
            }
        }

        FILE* file = fopen(actualPath, "rb");
        if (!file) {
            LOGE("Failed to open file: %s (errno=%d)", actualPath, errno);
            return LoadFileData((std::string("assets/") + path).c_str(), fileSize);
        }

        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        auto* data = (unsigned char*)malloc(size + 1);
        if (data) {
            size_t bytesRead = fread(data, 1, size, file);
            if (bytesRead == (size_t)size) {
                data[size] = '\0';
                *fileSize = (int)size;
                LOGD("Loaded file: %s (%d bytes)", actualPath, *fileSize);
            } else {
                LOGE("Failed to read file completely: %s", actualPath);
                free(data);
                data = nullptr;
            }
        }

        fclose(file);
        return data;
    }

    char* LoadFileText(const char* path) {
        int fileSize = 0;
        unsigned char* data = LoadFileData(path, &fileSize);
        return (char*)data;
    }

    void UnloadFileData(unsigned char* data) {
        if (data) free(data);
    }

    void UnloadFileText(char* text) {
        if (text) free(text);
    }

    int GetMonitorCount() {
        return 1;
    }

    int GetCurrentMonitor() {
        return 0;
    }

    int GetMonitorRefreshRate(int monitor) {
        (void)monitor;
        return 60;
    }

    void InitWindow(int width, int height, const char* title) {
        :: InitWindow(width, height, title);
        InitStoragePaths();
    }

    bool WindowShouldClose() {
        return :: WindowShouldClose();
    }

    void CloseWindow() {
        ::CloseWindow();
    }

    void SetWindowSize(int width, int height) {
        ::SetWindowSize(width, height);
    }

    const char* GetWorkingDirectory() {
        InitStoragePaths();

        if (externalDataPath[0] != '\0') {
            return externalDataPath;
        }

        if (internalDataPath[0] != '\0') {
            return internalDataPath;
        }

        return "/data/local/tmp";
    }

    int GetFileLength(const char* fileName) {
        int fileSize = 0;
        unsigned char* data = LoadFileData(fileName, &fileSize);
        if (data) {
            UnloadFileData(data);
            return fileSize;
        }
        return 0;
    }

    int GetScreenWidth() {
        return ::GetScreenWidth();
    }

    int GetScreenHeight() {
        return ::GetScreenHeight();
    }

    void ToggleFullscreen() {
        ::ToggleFullscreen();
    }

    bool IsAnyKeyPressed() {
        return ::GetKeyPressed() != 0 || ::GetTouchPointCount() > 0;
    }

} // namespace Madokawaii::Platform:: Core

#endif // PLATFORM_ANDROID