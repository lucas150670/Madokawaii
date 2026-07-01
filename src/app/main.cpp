
#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>
#include <string>
#include <thread>
#include <vector>
#include <future>
#include <unordered_map>

#include <fast_io.h>

#include "Madokawaii/app/app_config.hpp"
#include "Madokawaii/app/def.hpp"
#include "Madokawaii/app/chart.hpp"
#include "Madokawaii/app/line_operation.hpp"
#include "Madokawaii/app/note_hit.hpp"
#include "Madokawaii/app/note_operation.hpp"
#include "Madokawaii/app/res_pack.hpp"
#include "Madokawaii/app/common.hpp"
#include "Madokawaii/app/epilepsy_warning.hpp"
#include "Madokawaii/app/audio_engine_credit.hpp"
#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/platform/log.hpp"
#include "Madokawaii/platform/core.hpp"
#include "Madokawaii/platform/fonts.hpp"
#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/platform/texture.hpp"

namespace Madokawaii::App::Lifecycle {

namespace {
void UnloadGameResources(AppContext& ctx) {
    NoteHit::CleanupSfxManager(ctx);
    NoteRenderer::Unload(ctx);
    NoteHit::UnloadSfxManager(ctx);
    NoteHit::UnloadFxManager(ctx);

    if (ctx.gameplay.music.implementationDefined) {
        if (Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.gameplay.music)) {
            Madokawaii::Platform::Audio::StopMusicStream(ctx.gameplay.music);
        }
        Madokawaii::Platform::Audio::UnloadMusicStream(ctx.gameplay.music);
        ctx.gameplay.music = {};
    }

    if (ctx.assets.backgroundTexture.implementationDefinedData) {
        Madokawaii::Platform::Graphics::Texture::UnloadTexture(ctx.assets.backgroundTexture);
        ctx.assets.backgroundTexture = {};
    }
}

std::string MakePaddedScoreText(int score) {
    auto text = fast_io::concat(score);
    if (text.size() < 7) {
        text.insert(text.begin(), 7 - text.size(), '0');
    }
    return text;
}
}

void ResetGameToMainMenu(AppContext& ctx) {
    Ending::Reset(ctx);
    UnloadGameResources(ctx);

    ctx.gameplay.mainChart = {};
    ctx.assets.resPack.reset();
    ctx.lifecycle.gameInitFuture = {};
    ctx.lifecycle.gameInitStarted = false;
    ctx.lifecycle.asyncDataReady = false;
    ctx.lifecycle.gameInitialized = false;
    ctx.gameplay.completed = false;
    ctx.ui.menuCompleted = false;

    auto [r, g, b, a] = ctx.config.GetPerfectColor();
    ctx.assets.perfectColor = { r, g, b, a };

    Madokawaii::App::MainMenu::ResetMainMenu(ctx.ui.menu);
}

int LoadChartAsync(void* appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);

    auto& danli = ctx.config;
    const auto& musicPath = danli.GetMusicPath();
    const auto& chartPath = danli.GetChartPath();

    if (!Madokawaii::Platform::Core::FileExists(musicPath.c_str())) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Music file does not exist!");
        return -1;
    }

    const clock_t begin = clock();
    ctx.gameplay.mainChart = Madokawaii::App::Chart::LoadChartFromFile(chartPath.c_str());
    // 如果官谱格式加载失败，尝试 PEC 格式
    if (!Madokawaii::App::Chart::IsValidChart(ctx.gameplay.mainChart)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_WARNING,
                                            "MAIN: Failed to load chart as official format, trying PEC format...");
        ctx.gameplay.mainChart = Madokawaii::App::Chart::LoadChartFromPEC(chartPath.c_str());
    }

    if (!Madokawaii::App::Chart::IsValidChart(ctx.gameplay.mainChart)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load chart!");
        return -1;
    }
    const clock_t end = clock();

    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Chart Initialization Successful!");
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Format Version:         %d", ctx.gameplay.mainChart.formatVersion);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of notes:        %d", ctx.gameplay.mainChart.numOfNotes);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of judgelines:   %d", ctx.gameplay.mainChart.judgelineCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Elapsed time: %lf s", (end - begin) * 1.0 / CLOCKS_PER_SEC);

    Madokawaii::App::Chart::InitializeJudgelines(ctx.gameplay.mainChart);

    return 0;
}

int InitializeGameResources(void* appstate) {

    auto& ctx = *static_cast<AppContext*>(appstate);
    auto& danli = ctx.config;
    const auto& musicPath = danli.GetMusicPath();
    const auto& resPackPath = danli.GetResPackPath();
    int dataSize;
    auto respack_mem_stream = Madokawaii::Platform::Core::LoadFileData(resPackPath.c_str(), &dataSize);
    if (!respack_mem_stream)
    {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load respack file into memory!");
        return false;
    }
    ctx.assets.resPack = Madokawaii::App::ResPack::LoadResPackFromMemoryStream(respack_mem_stream, dataSize);
    Madokawaii::Platform::Core::UnloadFileData(respack_mem_stream);
    if (ctx.assets.resPack->colorPerfect.r != 0
        || ctx.assets.resPack->colorPerfect.g != 0
        || ctx.assets.resPack->colorPerfect.b != 0) {
        ctx.assets.perfectColor = ctx.assets.resPack->colorPerfect;
        ctx.assets.perfectColor.a = 255;
    }

    ctx.gameplay.music = Madokawaii::Platform::Audio::LoadMusicStream(musicPath.c_str());
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music stream loaded");

    NoteRenderer::Initialize(ctx, *ctx.assets.resPack);
    if (NoteHit::InitializeSfxManager(ctx, *ctx.assets.resPack)) return -1;
    if (NoteHit::InitializeFxManager(ctx, *ctx.assets.resPack, ctx.assets.perfectColor)) return -1;

    ctx.gameplay.music.looping = false;
    auto musicLength = Madokawaii::Platform::Audio::GetMusicTimeLength(ctx.gameplay.music);
    Madokawaii::Platform::Audio::SetMusicPitch(ctx.gameplay.music, 1.0f);
    Madokawaii::Platform::Audio::SetMusicVolume(ctx.gameplay.music, 0.5f);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music Length: %f", musicLength);

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
        Madokawaii::Platform::Graphics::Texture::ImageResizeNN(copiedImage, ctx.display.screenHeight * ratio, ctx.display.screenHeight); // NOLINT(*-narrowing-conversions)
    Madokawaii::Platform::Graphics::Texture::MeasureImage(copiedImage, &bgImageDimension);
    float newStartX = (bgImageDimension.x - ctx.display.screenWidth) / 2.0f;
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Background image dimension: (%f, %f)", bgImageDimension.x, bgImageDimension.y);
    Madokawaii::Platform::Shape::Rectangle srcRect = {newStartX, 0, bgImageDimension.x, bgImageDimension.y};
    Madokawaii::Platform::Graphics::Texture::ImageCrop(copiedImage, srcRect);
    Madokawaii::Platform::Graphics::Texture::ImageColorBrightness(copiedImage, -96.0f);
    Madokawaii::Platform::Graphics::Texture::ImageColorContrast(copiedImage, -16.0f);
    Madokawaii::Platform::Graphics::Texture::ImageBlurGaussian(copiedImage, 5.0f);
    ctx.assets.backgroundTexture = Madokawaii::Platform::Graphics::Texture::LoadTextureFromImage(copiedImage);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Game initialization completed!");

    return 0;
}

int Initialize(void*& appstate) {
    appstate = new AppContext;
    auto& ctx = *static_cast<AppContext*>(appstate);
    Madokawaii::Platform::Audio::InitAudioDevice();
    ctx.ui.audioEngineCreditCompleted = !Madokawaii::Platform::Audio::AudioEngineNeedAttribution();

    auto [r, g, b, a] = ctx.config.GetPerfectColor();
    ctx.assets.perfectColor = {r, g, b, a};


#if defined(PLATFORM_ANDROID)
    int screenHeight = Madokawaii::Platform::Core::GetScreenHeight();
    if (screenHeight <= 360) {
        ctx.display.screenWidth = 426; ctx.display.screenHeight = 240;
    }
    else if (screenHeight <= 640) {
        ctx.display.screenWidth = 854; ctx.display.screenHeight = 480;
    }
    else if (screenHeight <= 960) {
        ctx.display.screenWidth = 1280; ctx.display.screenHeight = 720;
    }
    else {
        ctx.display.screenWidth = 1920; ctx.display.screenHeight = 1080;
    }
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
                                        "MAIN: real resolution: %d, %d", Madokawaii::Platform::Core::GetScreenWidth(), Madokawaii::Platform::Core::GetScreenHeight());
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
                                        "MAIN: selected resolution: %d, %d", ctx.display.screenWidth, ctx.display.screenHeight);
    Madokawaii::Platform::Core::InitWindow(ctx.display.screenWidth, ctx.display.screenHeight, "Madokawaii");
#else
    Madokawaii::Platform::Core::InitWindow(ctx.display.screenWidth, ctx.display.screenHeight, "Madokawaii");
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

    if (!ctx.assets.fontLoaded) {
        if (Madokawaii::Platform::Graphics::GetImplementer().find("Mali") == std::string::npos)
        ctx.assets.chineseFont = Madokawaii::Platform::Graphics::Fonts::LoadFontWithChinese(
#if !defined(PLATFORM_ANDROID)
            "assets/font.ttf", 48);
#else
            "font.ttf", 48);
#endif
        else {
            // fuck mali gpu
            // crash when codepoint is too large
            ctx.assets.chineseFont = Madokawaii::Platform::Graphics::Fonts::LoadFontWithChinese("font.ttf", 16);
        }

        // Madokawaii::Platform::Graphics::SetTargetFPS(60);
        if (!Madokawaii::Platform::Graphics::Fonts::IsFontValid(ctx.assets.chineseFont)) {
            Madokawaii::Platform::Log::TraceLog(
                Madokawaii::Platform::Log::TraceLogLevel::LOG_WARNING,
                "WARNING: Failed to load Chinese font!");
        }
        ctx.assets.fontLoaded = true;
    }

    ctx.lifecycle.systemInitialized = true;
    return ctx.lifecycle.systemInitialized;
}

int InitializeGame(void *appstate) {
    auto &ctx = *static_cast<AppContext *>(appstate);
    if (!ctx.lifecycle.gameInitStarted) {
        ctx.lifecycle.gameInitStarted = true;

        std::promise<int> initPromise;
        ctx.lifecycle.gameInitFuture = initPromise.get_future();
        std::thread([appstate, promise = std::move(initPromise)]() mutable {
            int result = LoadChartAsync(appstate);
            promise.set_value(result);
        }).detach();

        Madokawaii::Platform::Log::TraceLog(
            Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
            "MAIN: Started async chart loading..."
        );
    }

    if (ctx.lifecycle.gameInitFuture.valid() &&
        ctx.lifecycle.gameInitFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        int initResult = ctx.lifecycle.gameInitFuture.get();
        if (initResult == 0) {
            ctx.lifecycle.asyncDataReady = true;
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
        Madokawaii::Platform::Graphics::DrawText("Converting chart..", ctx.display.screenWidth / 2 - 100, ctx.display.screenHeight / 2 - 50, 20,
                                                 Madokawaii::Platform::Graphics::M_LIGHTGRAY);
        // add state description
        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }
    if (ctx.lifecycle.asyncDataReady && !ctx.lifecycle.gameInitialized) {
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);
        Madokawaii::Platform::Graphics::DrawText("Setup scene..", ctx.display.screenWidth / 2 - 100, ctx.display.screenHeight / 2 - 50, 20,
                                                 Madokawaii::Platform::Graphics::M_LIGHTGRAY);
        Madokawaii::Platform::Graphics::EndDrawing();
        if (InitializeGameResources(appstate) == 0) {
            ctx.lifecycle.gameInitialized = true;
            ctx.lifecycle.isFirstRenderFrame = true;
        } else {
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR,
                                                "MAIN: Failed to initialize game!");
            return false;
        }
    }
    return true;
}

int IterateGame(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);
    if (!ctx.lifecycle.systemInitialized)
        return -1;

    NoteHit::CleanupSfxManager(ctx);
    Madokawaii::Platform::Audio::UpdateMusicStream(ctx.gameplay.music);
    Madokawaii::Platform::Graphics::BeginDrawing();
    Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);
    Madokawaii::Platform::Graphics::Vector2 texture_dimension{};
    Madokawaii::Platform::Graphics::Texture::MeasureTexture2D(ctx.assets.backgroundTexture, &texture_dimension);

    Madokawaii::Platform::Graphics::Texture::DrawTexture(
        ctx.assets.backgroundTexture,
        {(ctx.display.screenWidth - texture_dimension.x) / 2, 0},
        {255, 255, 255, 255});

    auto thisFrameTime = Madokawaii::Platform::Audio::GetMusicTimePlayed(ctx.gameplay.music) - ctx.gameplay.mainChart.offset;
    if (ctx.lifecycle.isFirstRenderFrame) {
        Madokawaii::Platform::Audio::PlayMusicStream(ctx.gameplay.music);
        ctx.lifecycle.isFirstRenderFrame = false;
    }
    else if (!Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.gameplay.music)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music playback end");
        Madokawaii::Platform::Graphics::EndDrawing(); 
        ctx.gameplay.completed = true;
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }

    if (thisFrameTime < 0) {
        Line::RenderDebugInfo(ctx);
        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }

    NoteRenderer::RenderHoldCallback(ctx, thisFrameTime);
    auto noteRenderList = std::vector<Madokawaii::App::chart::judgeline::note *>();

    int played_note_count = 0;
    for (auto &judgeline: ctx.gameplay.mainChart.judgelines) {
        int line_played_note = 0;
        Line::UpdateJudgeline(ctx, judgeline, thisFrameTime, noteRenderList, &line_played_note);
        Line::RenderJudgeline(ctx, judgeline);
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
        NoteRenderer::RenderNote(ctx, *notePtr);
    }

    Line::RenderDebugInfo(ctx);
    // render point
    const auto scoreText = MakePaddedScoreText(
        static_cast<int>(played_note_count * 1.0 / ctx.gameplay.mainChart.numOfNotes * 1000000));
    auto scoreDimension = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(ctx.assets.chineseFont, scoreText.c_str(), 48.0f, 2.0f);
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(ctx.assets.chineseFont, scoreText.c_str(), ctx.display.screenWidth - 45 - scoreDimension.x, 83.0f - scoreDimension.y, 48.0f, 2.0f, Madokawaii::Platform::Graphics::M_RAYWHITE);

    if (played_note_count > 2) {
        const auto hitNoteCountText = fast_io::concat(played_note_count);
        auto hitNoteCountDimension = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(ctx.assets.chineseFont, hitNoteCountText.c_str(), 64.0f, 2.0f);
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(ctx.assets.chineseFont, hitNoteCountText.c_str(), ctx.display.screenWidth / 2 - hitNoteCountDimension.x / 2, 15.0f, 64.0f, 2.0f, Madokawaii::Platform::Graphics::M_RAYWHITE);

        constexpr char autoPlayText[] = "Autoplay";
        auto autoPlayDimension = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(ctx.assets.chineseFont, autoPlayText, 24.0f, 2.0f);
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(ctx.assets.chineseFont, autoPlayText, ctx.display.screenWidth / 2 - autoPlayDimension.x / 2, 75.0f, 24.0f, 2.0f, Madokawaii::Platform::Graphics::M_RAYWHITE);
    }

    NoteHit::UpdateSfx(ctx);
    NoteHit::UpdateFx(ctx, thisFrameTime);
    Madokawaii::Platform::Graphics::EndDrawing();

    return !Madokawaii::Platform::Core::WindowShouldClose();
}


int Iterate(void * appstate) {
    // 扩展 留下放结算画面和开始画面的接口
    auto& ctx = *static_cast<AppContext*>(appstate);
    if (!ctx.lifecycle.systemInitialized) return -1;
    if (!ctx.ui.audioEngineCreditCompleted) return AudioEngineCredit::Iterate(ctx);
    // 先显示警告页面
    if (!ctx.ui.warningShown) return Warning::Iterate(ctx);// 显示主菜单
    if (!ctx.ui.menuCompleted) {
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);

        if (Madokawaii::App::MainMenu::RenderMainMenu(ctx.ui.menu, ctx.config,
                                                       ctx.display.screenWidth, ctx.display.screenHeight)) {
            ctx.ui.menuCompleted = true;
        }

        Madokawaii::Platform::Graphics::EndDrawing();
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }
    if (!ctx.lifecycle.gameInitialized) return InitializeGame(appstate);
    if (ctx.gameplay.completed) return Ending::Iterate(ctx);
    return IterateGame(appstate);
}

int Shutdown(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);
    Ending::Reset(ctx);
    AudioEngineCredit::Unload(ctx);
    Madokawaii::Platform::Core::CloseWindow();
    NoteRenderer::Unload(ctx);
    NoteHit::UnloadSfxManager(ctx);
    NoteHit::UnloadFxManager(ctx);
    if (ctx.gameplay.music.implementationDefined) {
        if (Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.gameplay.music))
            Madokawaii::Platform::Audio::StopMusicStream(ctx.gameplay.music);
        Madokawaii::Platform::Audio::UnloadMusicStream(ctx.gameplay.music);
    }
    if (ctx.assets.backgroundTexture.implementationDefinedData) {
        Madokawaii::Platform::Graphics::Texture::UnloadTexture(ctx.assets.backgroundTexture);
    }
    Madokawaii::Platform::Audio::CloseAudioDevice();
    delete static_cast<AppContext*>(appstate);
    return 0;
}

} // namespace Madokawaii::App::Lifecycle

extern "C" {
int AppInit(void*& appstate) {
    return Madokawaii::App::Lifecycle::Initialize(appstate);
}

int AppIterate(void* appstate) {
    return Madokawaii::App::Lifecycle::Iterate(appstate);
}

int AppExit(void* appstate) {
    return Madokawaii::App::Lifecycle::Shutdown(appstate);
}

}

