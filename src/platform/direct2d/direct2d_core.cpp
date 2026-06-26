//
// Direct2D backend core and Win32 file/window helpers.
//

#include "direct2d_platform.hpp"

#include "Madokawaii/platform/core.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace Madokawaii::Platform::Core {
    bool FileExists(const char* path) {
        return path && std::filesystem::exists(std::filesystem::u8path(path));
    }

    unsigned char* LoadFileData(const char* path, int* fileSize) {
        if (fileSize) *fileSize = 0;
        if (!path) return nullptr;

        std::ifstream file(std::filesystem::u8path(path), std::ios::binary | std::ios::ate);
        if (!file) return nullptr;

        const auto size = file.tellg();
        if (size < 0) return nullptr;
        file.seekg(0, std::ios::beg);

        auto* data = static_cast<unsigned char*>(std::malloc(static_cast<std::size_t>(size)));
        if (!data && size > 0) return nullptr;

        if (size > 0) {
            file.read(reinterpret_cast<char*>(data), size);
            if (!file) {
                std::free(data);
                return nullptr;
            }
        }

        if (fileSize) *fileSize = static_cast<int>(size);
        return data;
    }

    char* LoadFileText(const char* path) {
        int fileSize = 0;
        auto* data = LoadFileData(path, &fileSize);
        if (!data && fileSize > 0) return nullptr;

        auto* text = static_cast<char*>(std::malloc(static_cast<std::size_t>(fileSize) + 1));
        if (!text) {
            std::free(data);
            return nullptr;
        }

        if (fileSize > 0) {
            std::copy(data, data + fileSize, text);
        }
        text[fileSize] = '\0';
        std::free(data);
        return text;
    }

    void UnloadFileData(unsigned char* data) {
        std::free(data);
    }

    void UnloadFileText(char* text) {
        std::free(text);
    }

    int GetMonitorCount() {
        return GetSystemMetrics(SM_CMONITORS);
    }

    int GetCurrentMonitor() {
        return 0;
    }

    int GetMonitorRefreshRate(int) {
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1) {
            return static_cast<int>(mode.dmDisplayFrequency);
        }
        return 60;
    }

    void InitWindow(int width, int height, const char* title) {
        Direct2D::InitializeWindow(width, height, title);
    }

    bool WindowShouldClose() {
        Direct2D::PumpMessages();
        return Direct2D::GetState().shouldClose;
    }

    void CloseWindow() {
        Direct2D::ShutdownWindow();
    }

    void SetWindowSize(int width, int height) {
        auto& state = Direct2D::GetState();
        if (state.window) {
            RECT rect{0, 0, width, height};
            AdjustWindowRect(&rect, GetWindowLongW(state.window, GWL_STYLE), FALSE);
            SetWindowPos(
                state.window,
                nullptr,
                0,
                0,
                rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_NOMOVE | SWP_NOZORDER);
        }
        Direct2D::ResizeRenderTarget(width, height);
    }

    const char* GetWorkingDirectory() {
        static std::string workingDirectory;
        workingDirectory = std::filesystem::current_path().string();
        return workingDirectory.c_str();
    }

    int GetFileLength(const char* fileName) {
        if (!fileName) return 0;
        std::error_code error;
        const auto size = std::filesystem::file_size(std::filesystem::u8path(fileName), error);
        if (error) return 0;
        return static_cast<int>(size);
    }

    int GetScreenWidth() {
        return Direct2D::GetState().screenWidth;
    }

    int GetScreenHeight() {
        return Direct2D::GetState().screenHeight;
    }

    void ToggleFullscreen() {
        Direct2D::ToggleFullscreenWindow();
    }

    bool IsAnyKeyPressed() {
        Direct2D::PumpMessages();
        const auto& state = Direct2D::GetState();
        return state.anyKeyPressed || state.mouseLeftPressed;
    }

    std::string GetInternalCachePath() {
        return std::filesystem::temp_directory_path().string();
    }
}
