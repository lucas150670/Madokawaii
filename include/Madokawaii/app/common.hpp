//
// Created by madoka on 25-9-11.
//

#ifndef MADOKAWAII_COMMON_H
#define MADOKAWAII_COMMON_H

#include <array>
#include <future>
#include <map>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

#include "Madokawaii/app/app_config.hpp"
#include "Madokawaii/app/chart.hpp"
#include "Madokawaii/app/main_menu.hpp"
#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/app/res_pack.hpp"
#include "Madokawaii/platform/fonts.hpp"
#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/platform/texture.hpp"

namespace Madokawaii::App {

struct WarningState {
    float elapsedTime{0.0f};
    bool acknowledged{false};
    static constexpr float MIN_DISPLAY_TIME = 3.0f;
    static constexpr float AUTO_SKIP_TIME = 10.0f;
};

struct DisplayState {
    int screenWidth{1920};
    int screenHeight{1080};
};

struct LifecycleState {
    bool systemInitialized{false};
    bool gameInitialized{false};
    std::future<int> gameInitFuture;
    bool gameInitStarted{false};
    bool asyncDataReady{false};
};

struct AssetState {
    std::shared_ptr<ResPack::ResPack> resPack;
    Platform::Graphics::Texture::Texture2D backgroundTexture{};
    Platform::Graphics::Color perfectColor{};
    bool fontLoaded{false};
    Platform::Graphics::Fonts::Font chineseFont{};
};

struct GameplayState {
    Platform::Audio::Music music{};
    chart mainChart{};
    bool completed{false};
};

struct UiState {
    WarningState warning{};
    bool warningShown{false};
    MainMenu::MainMenuState menu{};
    bool menuCompleted{false};
};

struct NoteRendererResourceState {
    Platform::Graphics::Texture::Texture2D imageClick{};
    Platform::Graphics::Texture::Texture2D imageHold{};
    Platform::Graphics::Texture::Texture2D imageFlick{};
    Platform::Graphics::Texture::Texture2D imageDrag{};

    Platform::Graphics::Texture::Texture2D imageClickMH{};
    Platform::Graphics::Texture::Texture2D imageHoldMH{};
    Platform::Graphics::Texture::Texture2D imageFlickMH{};
    Platform::Graphics::Texture::Texture2D imageDragMH{};

    float holdAtlasHead{};
    float holdAtlasTail{};
    float holdAtlasMHHead{};
    float holdAtlasMHTail{};
};

struct NoteRendererState {
    NoteRendererResourceState resources{};
    std::vector<chart::judgeline::note> holdsToRender{};
    std::unordered_map<const chart::judgeline::note*, float> holdHitFxCounters{};
};

struct NoteHitSfxState {
    static constexpr int SOUND_POOL_SIZE = 20;

    std::array<Platform::Audio::Sound, SOUND_POOL_SIZE> clickSounds{};
    std::array<Platform::Audio::Sound, SOUND_POOL_SIZE> flickSounds{};
    std::array<Platform::Audio::Sound, SOUND_POOL_SIZE> dragSounds{};
    std::map<int, int> playMap{};
};

struct NoteHitFxResourceState {
    std::vector<Platform::Graphics::Texture::Texture2D> sprites{};
    float spriteUnitWidth{};
    float spriteUnitHeight{};
    Platform::Graphics::Color perfectColor{};
    int spriteFrameRate{};
    float spriteFrameTime{};
};

struct NoteHitFxInfo {
    float posX{};
    float posY{};
    float startTime{};
    int spriteIndex{};
    bool isDiscarded{};
    std::array<float, 4> direction{};
    std::array<float, 4> size{};
    std::array<float, 4> destination{};
};

struct NoteHitFxState {
    NoteHitFxResourceState resources{};
    std::vector<NoteHitFxInfo> activeEffects{};
    std::default_random_engine randomEngine{std::random_device{}()};
    std::uniform_real_distribution<float> unitDistribution{0.0f, 1.0f};
    std::uniform_int_distribution<int> displayDistribution{0, 10000};
};

struct EndingState {
    bool initialized{false};
    Platform::Audio::Music endingMusic{};
};

struct AppContext {
    AppConfig::ConfigManager config{};
    DisplayState display{};
    LifecycleState lifecycle{};
    AssetState assets{};
    GameplayState gameplay{};
    UiState ui{};
    NoteRendererState noteRenderer{};
    NoteHitSfxState noteHitSfx{};
    NoteHitFxState noteHitFx{};
    EndingState ending{};
};

namespace Lifecycle {
    void ResetGameToMainMenu(AppContext& context);
    int Initialize(void*& appstate);
    int Iterate(void* appstate);
    int Shutdown(void* appstate);
}

namespace Ending {
    void Reset(AppContext& context);
    int Iterate(AppContext& context);
}

namespace Warning {
    int Iterate(AppContext& context);
}

} // namespace Madokawaii::App

#endif //MADOKAWAII_COMMON_H
