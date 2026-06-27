//
// Created by madoka on 2026/1/9.
//
#include "Madokawaii/app/main_menu.hpp"
#include "Madokawaii/app/app_config.hpp"
#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/platform/gui.hpp"
#include "Madokawaii/platform/core.hpp"

#include <algorithm>
#include <cstring>

namespace Madokawaii::App::MainMenu {

    namespace {
        void CopyPathToBuffer(std::array<char, PATH_BUFFER_SIZE>& buffer, const std::string& path) {
            std::memset(buffer.data(), 0, buffer.size());
            std::strncpy(buffer.data(), path.c_str(), buffer.size() - 1);
        }
    }

    void InitMainMenu(MainMenuState& state, const Madokawaii::AppConfig::ConfigManager& config) {
        if (state.initialized) return;

        state.chartPath = config.GetChartPath();
        state.musicPath = config.GetMusicPath();
        state.resPackPath = config.GetResPackPath();
        state.backgroundPath = config.GetBackgroundPath();

        CopyPathToBuffer(state.chartPathBuffer, state.chartPath);
        CopyPathToBuffer(state.musicPathBuffer, state.musicPath);
        CopyPathToBuffer(state.resPackPathBuffer, state.resPackPath);
        CopyPathToBuffer(state.backgroundPathBuffer, state.backgroundPath);

        if (!state.dialogsInitialized) {
            Madokawaii::Platform::Gui::InitGui();
#if !defined(PLATFORM_ANDROID)
            const char* workDir = Madokawaii::Platform::Core::GetWorkingDirectory();
#else
            const char* workDir = "/storage/emulated/0/";
#endif
            for (auto& fileDialog : state.fileDialogs) {
                fileDialog = Madokawaii::Platform::Gui::InitFileDialog(workDir);
            }
            state.dialogsInitialized = true;
        }

        state.initialized = true;
    }

    static void DrawFileSelector(MainMenuState& state, int x, int y, int width, int height,
        const char* label, char* pathBuf, int bufSize,
        bool& editing, int dialogIndex, bool fileExists) {
        using namespace Madokawaii::Platform;

        // 根据文件是否存在选择标签颜色
        Graphics::Color_ labelColor = fileExists ?
            Graphics::Color_{ 200, 200, 200, 255 } :  // 正常灰色
            Graphics::Color_{ 255, 100, 100, 255 };   // 红色警告

        Graphics::DrawText(label, x, y - 25, 18, labelColor);

        // 如果文件不存在，在标签后显示警告
        if (!fileExists && pathBuf[0] != '\0') {
            const char* warning = " [File not found!]";
            int labelWidth = Graphics::MeasureText(label, 18);
            Graphics::DrawText(warning, x + labelWidth, y - 25, 18, { 255, 80, 80, 255 });
        }

        int textBoxWidth = width - 120;

        Shape::Rectangle textBoxRect = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(textBoxWidth),
            static_cast<float>(height)
        };

        if (Gui::TextBox(textBoxRect, pathBuf, bufSize, editing)) {
            editing = !editing;
        }

        Shape::Rectangle btnRect = {
            static_cast<float>(x + textBoxWidth + 10),
            static_cast<float>(y),
            100.0f,
            static_cast<float>(height)
        };

        if (Gui::Button(btnRect, "Browse...")) {
            state.activeDialogIndex = dialogIndex;
            Gui::OpenFileDialog(state.fileDialogs[dialogIndex]);
        }

        auto& fileDialog = state.fileDialogs[dialogIndex];
        if (fileDialog.selectFilePressed) {
            std::string selectedPath = Gui::GetSelectedFilePath(fileDialog);
            strncpy(pathBuf, selectedPath.c_str(), bufSize - 1);
            pathBuf[bufSize - 1] = '\0';
            fileDialog.selectFilePressed = false;
        }
    }

    bool RenderMainMenu(MainMenuState& state, Madokawaii::AppConfig::ConfigManager& config,
                        int screenWidth, int screenHeight) {
        using namespace Madokawaii::Platform;

        InitMainMenu(state, config);
        const float scaleX = screenWidth / 1280.0f;
        const float scaleY = screenHeight / 720.0f;
        const float scale = std::min(scaleX, scaleY);

        const char* title = "Madokawaii";
        int titleWidth = Madokawaii::Platform::Graphics::MeasureText(title, 48 * scale);
        Graphics::DrawText(title, (screenWidth - titleWidth) / 2, 80 * scale, 48 * scale, { 255, 255, 255, 255 });

        const char* subtitle = "select chart and respack here";
        int subtitleWidth = Madokawaii::Platform::Graphics::MeasureText(subtitle, 20 * scale);
        Graphics::DrawText(subtitle, (screenWidth - subtitleWidth) / 2, 140 * scale, 20 * scale, { 180, 180, 180, 255 });

        int panelWidth = 700 * scale;
        int panelX = (screenWidth - panelWidth) / 2;
        int startY = 220 * scale;
        int rowHeight = 36 * scale;
        int rowSpacing = 70 * scale;

        // 检查各文件是否存在
        bool chartExists = Core::FileExists(state.chartPathBuffer.data());
        bool musicExists = Core::FileExists(state.musicPathBuffer.data());
        bool resPackExists = Core::FileExists(state.resPackPathBuffer.data());
        bool backgroundExists = Core::FileExists(state.backgroundPathBuffer.data());

        DrawFileSelector(state, panelX, startY, panelWidth, rowHeight,
            "Chart File (.json):", state.chartPathBuffer.data(), static_cast<int>(state.chartPathBuffer.size()),
            state.chartPathEditing, 0, chartExists);

        DrawFileSelector(state, panelX, startY + rowSpacing, panelWidth, rowHeight,
            "Music File (.wav/.ogg):", state.musicPathBuffer.data(), static_cast<int>(state.musicPathBuffer.size()),
            state.musicPathEditing, 1, musicExists);

        DrawFileSelector(state, panelX, startY + rowSpacing * 2, panelWidth, rowHeight,
            "Resource Pack (.zip):", state.resPackPathBuffer.data(), static_cast<int>(state.resPackPathBuffer.size()),
            state.resPackPathEditing, 2, resPackExists);

        DrawFileSelector(state, panelX, startY + rowSpacing * 3, panelWidth, rowHeight,
            "Background Image:", state.backgroundPathBuffer.data(), static_cast<int>(state.backgroundPathBuffer.size()),
            state.backgroundPathEditing, 3, backgroundExists);

        int btnWidth = 200 * scale;
        int btnHeight = 50 * scale;
        int btnX = (screenWidth - btnWidth) / 2;
        int btnY = startY + rowSpacing * 4 + 40;

        Shape::Rectangle startBtn = {
            static_cast<float>(btnX),
            static_cast<float>(btnY),
            static_cast<float>(btnWidth),
            static_cast<float>(btnHeight)
        };

        bool canStart = (state.activeDialogIndex == -1 || !state.fileDialogs[state.activeDialogIndex].windowActive);

        // 检查所有必需文件是否存在
        bool allFilesExist = chartExists && musicExists && resPackExists && backgroundExists;

        // 显示文件缺失警告
        if (!allFilesExist) {
            const char* errorMsg = "Please select all required files before starting";
            int errorWidth = Graphics::MeasureText(errorMsg, 16);
            Graphics::DrawText(errorMsg, (screenWidth - errorWidth) / 2, btnY - 30, 16, { 255, 100, 100, 255 });
        }

        if (canStart && allFilesExist && Gui::Button(startBtn, "START GAME")) {
            state.chartPath = state.chartPathBuffer.data();
            state.musicPath = state.musicPathBuffer.data();
            state.resPackPath = state.resPackPathBuffer.data();
            state.backgroundPath = state.backgroundPathBuffer.data();

            ApplyMenuConfig(state, config);
            state.startRequested = true;
        }
        else if (canStart && !allFilesExist) {
            // 文件不完整时显示禁用状态的按钮
            Gui::Button(startBtn, "START GAME");
        }

        if (state.activeDialogIndex >= 0) {
            Gui::UpdateFileDialog(state.fileDialogs[state.activeDialogIndex]);

            if (!state.fileDialogs[state.activeDialogIndex].windowActive) {
                state.activeDialogIndex = -1;
            }
        }

        const char* hint = "Click 'Browse...' to select files, or type path directly";
        int hintWidth = Graphics::MeasureText(hint, 14);
        Graphics::DrawText(hint, (screenWidth - hintWidth) / 2, screenHeight - 50, 14, { 100, 100, 100, 255 });

        return state.startRequested;
    }

    void ApplyMenuConfig(const MainMenuState& state, Madokawaii::AppConfig::ConfigManager& config) {
        config.SetChartPath(state.chartPath);
        config.SetMusicPath(state.musicPath);
        config.SetResPackPath(state.resPackPath);
        config.SetBackgroundPath(state.backgroundPath);
    }

    void ResetMainMenu(MainMenuState& state) {
        if (state.dialogsInitialized) {
            for (auto& fileDialog : state.fileDialogs) {
                Madokawaii::Platform::Gui::UnloadFileDialog(fileDialog);
            }
            state.dialogsInitialized = false;
        }

        state.activeDialogIndex = -1;
        std::memset(state.chartPathBuffer.data(), 0, state.chartPathBuffer.size());
        std::memset(state.musicPathBuffer.data(), 0, state.musicPathBuffer.size());
        std::memset(state.resPackPathBuffer.data(), 0, state.resPackPathBuffer.size());
        std::memset(state.backgroundPathBuffer.data(), 0, state.backgroundPathBuffer.size());
        state = {};
    }

}
