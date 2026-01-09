
#include <algorithm>
#include <ctime>
#include <format>
#include <string>
#include <vector>
#include <future>
#include <unordered_map>

#include "Madokawaii/app/app_config.h"
#include "Madokawaii/app/def.h"
#include "Madokawaii/app/chart.h"
#include "Madokawaii/app/line_operation.h"
#include "Madokawaii/app/note_hit.h"
#include "Madokawaii/app/note_operation.h"
#include "Madokawaii/app/res_pack.h"
#include "Madokawaii/app/common.h"
#include "Madokawaii/app/epilepsy_warning.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/core.h"
#include "Madokawaii/platform/fonts.h"
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/texture.h"

extern "C" {

int GameInit_Async(void* appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);

    auto& danli = Madokawaii::AppConfig::ConfigManager::Instance();
    const auto& musicPath = danli.GetMusicPath();
    const auto& chartPath = danli.GetChartPath();

    if (!Madokawaii::Platform::Core::FileExists(musicPath.c_str())) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Music file does not exist!");
        return -1;
    }

    const clock_t begin = clock();
    ctx.mainChart = Madokawaii::App::Chart::LoadChartFromFile(chartPath.c_str());
    if (!Madokawaii::App::Chart::IsValidChart(ctx.mainChart)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load chart!");
        return -1;
    }
    const clock_t end = clock();

    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Chart Initialization Successful!");
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Format Version:         %d", ctx.mainChart.formatVersion);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of notes:        %d", ctx.mainChart.numOfNotes);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of judgelines:   %d", ctx.mainChart.judgelineCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Elapsed time: %lf s", (end - begin) * 1.0 / CLOCKS_PER_SEC);

    Madokawaii::App::Chart::InitializeJudgelines(ctx.mainChart);

    return 0;
}

int GameInit_Main_Thrd(void* appstate) {

    auto& ctx = *static_cast<AppContext*>(appstate);
    auto& danli = Madokawaii::AppConfig::ConfigManager::Instance();
    const auto& musicPath = danli.GetMusicPath();

    ctx.music = Madokawaii::Platform::Audio::LoadMusicStream(musicPath.c_str());
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music stream loaded");

    InitializeNoteRenderer(*ctx.global_respack, ctx.screenWidth, ctx.screenHeight);
    if (InitializeNoteHitSfxManager(*ctx.global_respack)) return -1;
    if (InitializeNoteHitFxManager(*ctx.global_respack, ctx.perfectColor)) return -1;

    ctx.music.looping = false;
    auto musicLength = Madokawaii::Platform::Audio::GetMusicTimeLength(ctx.music);
    Madokawaii::Platform::Audio::SetMusicPitch(ctx.music, 1.0f);
    Madokawaii::Platform::Audio::SetMusicVolume(ctx.music, 0.5f);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music Length: %f", musicLength);
    Madokawaii::Platform::Audio::PlayMusicStream(ctx.music);

    if (!Madokawaii::Platform::Core::FileExists(danli.GetBackgroundPath().c_str()))
    {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Background image does not exist!");
        return -1;
    }
    Madokawaii::Platform::Graphics::Texture::Image background_image = Madokawaii::Platform::Graphics::Texture::LoadImage(danli.GetBackgroundPath().c_str());
    auto copiedImage = Madokawaii::Platform::Graphics::Texture::ImageCopy(background_image);
    Madokawaii::Platform::Graphics::Texture::UnloadImage(background_image);
    Madokawaii::Platform::Graphics::Vector2 bgImageDimension{};
    Madokawaii::Platform::Graphics::Texture::MeasureImage(copiedImage, &bgImageDimension);
    auto ratio = bgImageDimension.x / bgImageDimension.y;
    Madokawaii::Platform::Graphics::Texture::ImageResizeNN(copiedImage, ctx.screenHeight * ratio, ctx.screenHeight); // NOLINT(*-narrowing-conversions)
    Madokawaii::Platform::Graphics::Texture::MeasureImage(copiedImage, &bgImageDimension);
    float newStartX = (bgImageDimension.x - ctx.screenWidth) / 2.0f;
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Background image dimension: ({}, {})", bgImageDimension.x, bgImageDimension.y);
    Madokawaii::Platform::Shape::Rectangle srcRect = {newStartX, 0, bgImageDimension.x, bgImageDimension.y};
    Madokawaii::Platform::Graphics::Texture::ImageCrop(copiedImage, srcRect);
    Madokawaii::Platform::Graphics::Texture::ImageColorBrightness(copiedImage, -96.0f);
    Madokawaii::Platform::Graphics::Texture::ImageColorContrast(copiedImage, -16.0f);
    Madokawaii::Platform::Graphics::Texture::ImageBlurGaussian(copiedImage, 5.0f);
    ctx.backgroundTexture = Madokawaii::Platform::Graphics::Texture::LoadTextureFromImage(copiedImage);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Game initialization completed!");

    return 0;
}

int AppInit(void*& appstate) {
    appstate = new AppContext;
    auto& ctx = *static_cast<AppContext*>(appstate);
    Madokawaii::Platform::Audio::InitAudioDevice();

    auto& danli = Madokawaii::AppConfig::ConfigManager::Instance();
    const auto& resPackPath = danli.GetResPackPath();

    int dataSize = 0;
    auto respack_mem_stream = Madokawaii::Platform::Core::LoadFileData(resPackPath.c_str(), &dataSize);
    if (!respack_mem_stream)
    {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load respack file into memory!");
        return false;
    }
    ctx.global_respack = Madokawaii::App::ResPack::LoadResPackFromMemoryStream(respack_mem_stream, dataSize);
    Madokawaii::Platform::Core::UnloadFileData(respack_mem_stream);
    auto [r, g, b, a] = danli.GetPerfectColor();
    ctx.perfectColor = {r, g, b, a};

    Madokawaii::Platform::Core::InitWindow(ctx.screenWidth, ctx.screenHeight, "Madokawaii");
    // Madokawaii::Platform::Core::ToggleFullscreen();
    // ctx.screenWidth = Madokawaii::Platform::Core::GetScreenWidth();
    // ctx.screenHeight = Madokawaii::Platform::Core::GetScreenHeight();
    /* Enable vertical sync by uncommenting this line
    Madokawaii::Platform::Graphics::SetTargetFPS(
        Madokawaii::Platform::Core::GetMonitorRefreshRate(
            Madokawaii::Platform::Core::GetCurrentMonitor()
            )
    );*/

    ctx.sys_initialized = true;
    return ctx.sys_initialized;
}

int GameInit(void *appstate) {
    auto &ctx = *static_cast<AppContext *>(appstate);
    if (!ctx.gameInitStarted) {
        ctx.gameInitStarted = true;

        std::promise<int> initPromise;
        ctx.gameInitFuture = initPromise.get_future();
        std::thread([appstate, promise = std::move(initPromise)]() mutable {
            int result = GameInit_Async(appstate);
            promise.set_value(result);
        }).detach();

        Madokawaii::Platform::Log::TraceLog(
            Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
            "MAIN: Started async chart loading..."
        );
    }

    if (ctx.gameInitFuture.valid() &&
        ctx.gameInitFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        int initResult = ctx.gameInitFuture.get();
        if (initResult == 0) {
            ctx.asyncDataReady = true;
            Madokawaii::Platform::Log::TraceLog(
                Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
                "MAIN: Async initialization completed!"
            );
        } else {
            Madokawaii::Platform::Log::TraceLog(
                Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR,
                "MAIN: Async initialization failed!"
            );
            return false;
        }
    } else {
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);
        Madokawaii::Platform::Graphics::DrawText("Converting chart..", 1920 / 2 - 100, 1080 / 2 - 50, 20,
                                                 Madokawaii::Platform::Graphics::M_LIGHTGRAY);
        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }
    if (ctx.asyncDataReady && !ctx.game_initialized) {
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);
        Madokawaii::Platform::Graphics::DrawText("Setup scene..", 1920 / 2 - 100, 1080 / 2 - 50, 20,
                                                 Madokawaii::Platform::Graphics::M_LIGHTGRAY);
        Madokawaii::Platform::Graphics::EndDrawing();
        if (GameInit_Main_Thrd(appstate) == 0) {
            ctx.game_initialized = true;
        } else {
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR,
                                                "MAIN: Failed to initialize game!");
            return false;
        }
    }
    return true;
}

int AppIterate_Game(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);
    if (!ctx.sys_initialized)
        return -1;

    CleanupNoteHitSfxManager();
    Madokawaii::Platform::Audio::UpdateMusicStream(ctx.music);
    Madokawaii::Platform::Graphics::BeginDrawing();
    Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);
    DrawTexture(ctx.backgroundTexture, {0, 0}, {255, 255, 255, 255});

    auto thisFrameTime = Madokawaii::Platform::Audio::GetMusicTimePlayed(ctx.music);
    if (!Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.music)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music playback end");
        Madokawaii::Platform::Graphics::EndDrawing();
        return false;
    }

    RenderHoldCallback(thisFrameTime, ctx.mainChart);
    auto noteRenderList = std::vector<Madokawaii::App::chart::judgeline::note *>();

    for (auto &judgeline: ctx.mainChart.judgelines) {
        UpdateJudgeline(judgeline, thisFrameTime, ctx.screenWidth, ctx.screenHeight, noteRenderList);
        RenderJudgeline(judgeline, ctx.screenWidth, ctx.screenHeight, ctx.perfectColor);

    }

    // 多押标记优化:O(n^2)->O(n)
    auto timeToKey = [](double time) {
        auto keyStr = std::to_string(static_cast<long long>(std::round(time * 1e6)));
        return Madokawaii::Platform::Core::hash_compile_time(keyStr.c_str());
    };

    std::unordered_map<uint64_t, int> timeCount;
    for (const auto& notePtr : noteRenderList) {
        timeCount[timeToKey(notePtr->realTime)]++;
    }

    for (auto& notePtr : noteRenderList) {
        if (timeCount[timeToKey(notePtr->realTime)] >= 2) {
            notePtr->isMultipleNote = true;
        }
    }


    for (auto notePtr: noteRenderList) {
        // TODO: 实现note渲染
        RenderNote(*notePtr);
    }

    RenderDebugInfo();
    UpdateNoteHitSfx();
    UpdateNoteHitFx(thisFrameTime);
    Madokawaii::Platform::Graphics::EndDrawing();

    return !Madokawaii::Platform::Core::WindowShouldClose();
}


int AppIterate(void * appstate) {
    // 扩展 留下放结算画面和开始画面的接口
    auto& ctx = *static_cast<AppContext*>(appstate);
    if (!ctx.sys_initialized) return -1;
    // 先显示警告页面
    if (!ctx.warningShown) return AppIterate_Warning(appstate);// 显示主菜单
    if (!ctx.menuCompleted) {
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);

        if (Madokawaii::App::MainMenu::RenderMainMenu(ctx.menuState, ctx.screenWidth, ctx.screenHeight)) {
            ctx.menuCompleted = true;
        }

        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }
    if (!ctx.game_initialized) return GameInit(appstate);

    return AppIterate_Game(appstate);
}

int AppExit(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);
    Madokawaii::Platform::Core::CloseWindow();
    UnloadNoteRenderer();
    UnloadNoteHitSfxManager();
    UnloadNoteHitFxManager();
    if (ctx.music.implementationDefined) {
        if (Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.music))
            Madokawaii::Platform::Audio::StopMusicStream(ctx.music);
        Madokawaii::Platform::Audio::UnloadMusicStream(ctx.music);
    }
    if (ctx.backgroundTexture.implementationDefinedData) {
        Madokawaii::Platform::Graphics::Texture::UnloadTexture(ctx.backgroundTexture);
    }
    Madokawaii::Platform::Audio::CloseAudioDevice();
    delete static_cast<AppContext*>(appstate);
    return 0;
}

}