//
// Created by madoka on 25-9-11.
//

#ifndef MADOKAWAII_COMMON_H
#define MADOKAWAII_COMMON_H

#include <future>
#include <memory>
#include "Madokawaii/app/chart.h"
#include "Madokawaii/app/res_pack.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/texture.h"
#include "Madokawaii/platform/fonts.h"
#include "Madokawaii/app/main_menu.h"

struct WarningState {
    float elapsedTime{0.0f};
    bool acknowledged{false};
    bool fontLoaded{false};
    Madokawaii::Platform::Graphics::Fonts::Font chineseFont{};
    static constexpr float MIN_DISPLAY_TIME = 3.0f;  // 最少显示3秒
    static constexpr float AUTO_SKIP_TIME = 10.0f;   // 10秒后自动跳过
};

struct AppContext {
    int screenWidth{1920};
    int screenHeight{1080};
    Madokawaii::Platform::Audio::Music music{};
    Madokawaii::App::chart mainChart{};
    bool sys_initialized{false}, game_initialized{false};
    std::shared_ptr<Madokawaii::App::ResPack::ResPack> global_respack;
    Madokawaii::Platform::Graphics::Texture::Texture2D backgroundTexture{};
    Madokawaii::Platform::Graphics::Color perfectColor{};

    std::future<int> gameInitFuture;
    bool gameInitStarted{false};
    bool asyncDataReady{false};
    WarningState warningState{};
    bool warningShown{false};
    // 主菜单状态
    Madokawaii::App::MainMenu::MainMenuState menuState{};
    bool menuCompleted{ false }; 
    bool gameCompleted{ false };
};

int AppIterate_Ending(AppContext* context);

#endif //MADOKAWAII_COMMON_H
