
#include <algorithm>
#include <ctime>
#include <format>
#include <string>
#include <vector>
#include <future>
#include <unordered_map>

#include "Madokawaii/app/app_config.hpp"
#include "Madokawaii/app/def.hpp"
#include "Madokawaii/app/chart.hpp"
#include "Madokawaii/app/line_operation.hpp"
#include "Madokawaii/app/note_hit.hpp"
#include "Madokawaii/app/note_operation.hpp"
#include "Madokawaii/app/res_pack.hpp"
#include "Madokawaii/app/common.hpp"
#include "Madokawaii/app/epilepsy_warning.hpp"
#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/platform/log.hpp"
#include "Madokawaii/platform/core.hpp"
#include "Madokawaii/platform/fonts.hpp"
#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/platform/texture.hpp"

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
    // 如果官谱格式加载失败，尝试 PEC 格式
    if (!Madokawaii::App::Chart::IsValidChart(ctx.mainChart)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_WARNING,
                                            "MAIN: Failed to load chart as official format, trying PEC format...");
        ctx.mainChart = Madokawaii::App::Chart::LoadChartFromPEC(chartPath.c_str());
    }

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
    const auto& resPackPath = danli.GetResPackPath();
    int dataSize;
    auto respack_mem_stream = Madokawaii::Platform::Core::LoadFileData(resPackPath.c_str(), &dataSize);
    if (!respack_mem_stream)
    {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load respack file into memory!");
        return false;
    }
    ctx.global_respack = Madokawaii::App::ResPack::LoadResPackFromMemoryStream(respack_mem_stream, dataSize);
    Madokawaii::Platform::Core::UnloadFileData(respack_mem_stream);
    if (ctx.global_respack->colorPerfect.r != 0
        || ctx.global_respack->colorPerfect.g != 0
        || ctx.global_respack->colorPerfect.b != 0) {
        ctx.perfectColor = ctx.global_respack->colorPerfect;
        ctx.perfectColor.a = 255;
    }

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
    if (ratio > 1)
        Madokawaii::Platform::Graphics::Texture::ImageResizeNN(copiedImage, ctx.screenHeight * ratio, ctx.screenHeight); // NOLINT(*-narrowing-conversions)
    Madokawaii::Platform::Graphics::Texture::MeasureImage(copiedImage, &bgImageDimension);
    float newStartX = (bgImageDimension.x - ctx.screenWidth) / 2.0f;
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Background image dimension: (%f, %f)", bgImageDimension.x, bgImageDimension.y);
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
    auto [r, g, b, a] = danli.GetPerfectColor();
    ctx.perfectColor = {r, g, b, a};


#if defined(PLATFORM_ANDROID)
    int screenHeight = Madokawaii::Platform::Core::GetScreenHeight();
    if (screenHeight <= 360) {
        ctx.screenWidth = 426; ctx.screenHeight = 240;
    }
    else if (screenHeight <= 640) {
        ctx.screenWidth = 854; ctx.screenHeight = 480;
    }
    else if (screenHeight <= 960) {
        ctx.screenWidth = 1280; ctx.screenHeight = 720;
    }
    else {
        ctx.screenWidth = 1920; ctx.screenHeight = 1080;
    }
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
                                        "MAIN: real resolution: %d, %d", Madokawaii::Platform::Core::GetScreenWidth(), Madokawaii::Platform::Core::GetScreenHeight());
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
                                        "MAIN: selected resolution: %d, %d", ctx.screenWidth, ctx.screenHeight);
    Madokawaii::Platform::Core::InitWindow(ctx.screenWidth, ctx.screenHeight, "Madokawaii");
#else
    Madokawaii::Platform::Core::InitWindow(ctx.screenWidth, ctx.screenHeight, "Madokawaii");
#endif
    /* Enable vertical sync by uncommenting this line
     */
#if defined(PLATFORM_ANDROID)
    int refresh_rate =
            Madokawaii::Platform::Core::GetMonitorRefreshRate(
                    Madokawaii::Platform::Core::GetCurrentMonitor()
            );
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
                                        "MAIN: screen refresh rate = %d", refresh_rate);
    Madokawaii::Platform::Graphics::SetTargetFPS(refresh_rate);
#endif

    if (!ctx.fontLoaded) {
        if (Madokawaii::Platform::Graphics::GetImplementer().find("Mali") == std::string::npos)
        ctx.chineseFont = Madokawaii::Platform::Graphics::Fonts::LoadFontWithChinese(
#if !defined(PLATFORM_ANDROID)
            "assets/font.ttf", 48);
#else
            "font.ttf", 48);
#endif
        else {
            // fuck mali gpu
            // crash when codepoint is too large
            ctx.chineseFont = Madokawaii::Platform::Graphics::Fonts::LoadFontWithChinese("font.ttf", 16);
        }

        // Madokawaii::Platform::Graphics::SetTargetFPS(60);
        if (!Madokawaii::Platform::Graphics::Fonts::IsFontValid(ctx.chineseFont)) {
            Madokawaii::Platform::Log::TraceLog(
                Madokawaii::Platform::Log::TraceLogLevel::LOG_WARNING,
                "WARNING: Failed to load Chinese font!");
        }
        ctx.fontLoaded = true;
    }

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
        Madokawaii::Platform::Graphics::DrawText("Converting chart..", ctx.screenWidth / 2 - 100, ctx.screenHeight / 2 - 50, 20,
                                                 Madokawaii::Platform::Graphics::M_LIGHTGRAY);
        // add state description
        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }
    if (ctx.asyncDataReady && !ctx.game_initialized) {
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);
        Madokawaii::Platform::Graphics::DrawText("Setup scene..", ctx.screenWidth / 2 - 100, ctx.screenHeight / 2 - 50, 20,
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
    Madokawaii::Platform::Graphics::Vector2 texture_dimension{};
    Madokawaii::Platform::Graphics::Texture::MeasureTexture2D(ctx.backgroundTexture, &texture_dimension);

    DrawTexture(ctx.backgroundTexture, {(ctx.screenWidth - texture_dimension.x) / 2, 0}, {255, 255, 255, 255});

    auto thisFrameTime = Madokawaii::Platform::Audio::GetMusicTimePlayed(ctx.music) - ctx.mainChart.offset;
    if (!Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.music)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music playback end");
        Madokawaii::Platform::Graphics::EndDrawing(); 
        ctx.gameCompleted = true;
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }

    if (thisFrameTime < 0) {
        RenderDebugInfo(ctx.screenWidth, ctx.screenHeight);
        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }

    RenderHoldCallback(thisFrameTime, ctx.mainChart);
    auto noteRenderList = std::vector<Madokawaii::App::chart::judgeline::note *>();

    int played_note_count = 0;
    for (auto &judgeline: ctx.mainChart.judgelines) {
        int line_played_note = 0;
        UpdateJudgeline(judgeline, thisFrameTime, ctx.screenWidth, ctx.screenHeight, noteRenderList, &line_played_note);
        RenderJudgeline(judgeline, ctx.screenWidth, ctx.screenHeight, ctx.perfectColor);
        played_note_count += line_played_note;
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

    RenderDebugInfo(ctx.screenWidth, ctx.screenHeight);
    // render point
    char strPoint[8];
    sprintf(strPoint, "%07d", static_cast<int>(played_note_count * 1.0 / ctx.mainChart.numOfNotes * 1000000));
    auto scoreDimension = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(ctx.chineseFont, strPoint, 48.0f, 2.0f);
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(ctx.chineseFont, strPoint, ctx.screenWidth - 45 - scoreDimension.x, 83.0f - scoreDimension.y, 48.0f, 2.0f, Madokawaii::Platform::Graphics::M_RAYWHITE);

    if (played_note_count > 2) {
        char hitNoteCountStr[128];
        sprintf(hitNoteCountStr, "%d", played_note_count);
        auto hitNoteCountDimension = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(ctx.chineseFont, hitNoteCountStr, 64.0f, 2.0f);
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(ctx.chineseFont, hitNoteCountStr, ctx.screenWidth / 2 - hitNoteCountDimension.x / 2, 15.0f, 64.0f, 2.0f, Madokawaii::Platform::Graphics::M_RAYWHITE);

        constexpr char autoPlayText[] = "Autoplay";
        auto autoPlayDimension = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(ctx.chineseFont, autoPlayText, 24.0f, 2.0f);
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(ctx.chineseFont, autoPlayText, ctx.screenWidth / 2 - autoPlayDimension.x / 2, 75.0f, 24.0f, 2.0f, Madokawaii::Platform::Graphics::M_RAYWHITE);
    }

    UpdateNoteHitSfx();
    UpdateNoteHitFx(thisFrameTime, ctx.screenWidth, ctx.screenHeight);
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
    if (ctx.gameCompleted) return AppIterate_Ending(&ctx);
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