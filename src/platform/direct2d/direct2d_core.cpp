//
// Direct2D backend core and Win32 file/window helpers.
//

#include "direct2d_platform.hpp"

#include "Madokawaii/platform/core.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <limits>

#include <fast_io.h>

namespace Madokawaii::Platform::Core {
    bool FileExists(const char* path) {
        return path && std::filesystem::exists(std::filesystem::u8path(path));
    }

    unsigned char* LoadFileData(const char* path, int* fileSize) {
        if (fileSize) *fileSize = 0;
        if (!path) return nullptr;

        fast_io::native_file_loader file;
        try {
            file = fast_io::native_file_loader(fast_io::manipulators::os_c_str(path),
                                               fast_io::open_mode::in);
        } catch (...) {
            return nullptr;
        }

        const auto size = file.size();
        if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) return nullptr;
        auto* data = static_cast<unsigned char*>(std::malloc(static_cast<std::size_t>(size)));
        if (!data && size > 0) return nullptr;

        if (size > 0) {
            std::copy(file.begin(), file.end(), data);
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
        try {
            fast_io::native_file_loader file(fast_io::manipulators::os_c_str(fileName),
                                             fast_io::open_mode::in);
            const auto size = file.size();
            if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) return 0;
            return static_cast<int>(size);
        } catch (...) {
            return 0;
        }
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
