// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/platform/platform_common.h
// Usage: Common interfaces used by main app.
// ============================================================

#ifndef MADOKAWAII_PLATFORM_COMMON_H
#define MADOKAWAII_PLATFORM_COMMON_H

#include <cstdint>

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

    int GetFileLength(const char *fileName);

    // Define a hash function for compile-time string hashing
    constexpr std::uint64_t prime = 0x100000001B3ull;
    constexpr std::uint64_t basis = 0xCBF29CE484222325ull;

    constexpr std::uint64_t hash_compile_time(const char* str, std::uint64_t last_value = basis)
    {
        return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
    }

    // Overload operator for user-defined literal
    constexpr std::uint64_t operator""_hash(const char* str, size_t)
    {
        return hash_compile_time(str);
    }
}

extern "C" {
int AppInit(void *& appstate);

/**
 * @brief App Iterate Callback
 * @param appstate Global AppState
 * @return 0... breaks the iteration.
 */
int AppIterate(void *appstate);

int AppExit(void *appstate);
}

#endif //MADOKAWAII_PLATFORM_COMMON_H
