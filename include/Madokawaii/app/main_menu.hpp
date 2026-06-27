//
// Created by madoka on 2026/1/9.
//

#ifndef MADOKAWAII_MAIN_MENU_H
#define MADOKAWAII_MAIN_MENU_H

#include <array>
#include <cstddef>
#include <string>

#include "Madokawaii/app/app_config.hpp"
#include "Madokawaii/platform/gui.hpp"

namespace Madokawaii::App::MainMenu {

    constexpr std::size_t FILE_DIALOG_COUNT = 4;
    constexpr std::size_t PATH_BUFFER_SIZE = 512;

    struct MainMenuState {
        std::string chartPath;
        std::string musicPath;
        std::string resPackPath;
        std::string backgroundPath;

        bool chartPathEditing{ false };
        bool musicPathEditing{ false };
        bool resPackPathEditing{ false };
        bool backgroundPathEditing{ false };

        bool startRequested{ false };
        bool initialized{ false };

        std::array<Madokawaii::Platform::Gui::FileDialogState, FILE_DIALOG_COUNT> fileDialogs{};
        int activeDialogIndex{ -1 };
        bool dialogsInitialized{ false };

        std::array<char, PATH_BUFFER_SIZE> chartPathBuffer{};
        std::array<char, PATH_BUFFER_SIZE> musicPathBuffer{};
        std::array<char, PATH_BUFFER_SIZE> resPackPathBuffer{};
        std::array<char, PATH_BUFFER_SIZE> backgroundPathBuffer{};
    };

    // 初始化主菜单，从 ConfigManager 加载当前配置
    void InitMainMenu(MainMenuState& state, const Madokawaii::AppConfig::ConfigManager& config);

    // 渲染主菜单，返回 true 表示用户点击了 Start 按钮
    bool RenderMainMenu(MainMenuState& state, Madokawaii::AppConfig::ConfigManager& config,
                        int screenWidth, int screenHeight);

    // 将主菜单中的配置保存到 ConfigManager
    void ApplyMenuConfig(const MainMenuState& state, Madokawaii::AppConfig::ConfigManager& config);

    // 重置主菜单运行状态
    void ResetMainMenu(MainMenuState& state);

}

#endif //MADOKAWAII_MAIN_MENU_H
