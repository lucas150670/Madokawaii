//
// Created by madoka on 2026/1/9.
//

#ifndef MADOKAWAII_MAIN_MENU_H
#define MADOKAWAII_MAIN_MENU_H

#include <string>

namespace Madokawaii::App::MainMenu {

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
    };

    // 初始化主菜单，从 ConfigManager 加载当前配置
    void InitMainMenu(MainMenuState& state);

    // 渲染主菜单，返回 true 表示用户点击了 Start 按钮
    bool RenderMainMenu(MainMenuState& state, int screenWidth, int screenHeight);

    // 将主菜单中的配置保存到 ConfigManager
    void ApplyMenuConfig(const MainMenuState& state);

}

#endif //MADOKAWAII_MAIN_MENU_H