//
// Created by madoka on 2026/1/6.
//
#include "Madokawaii/app/epilepsy_warning.h"
#include "Madokawaii/app/common.h"
#include "Madokawaii/platform/core.h"
#include "Madokawaii/platform/log.h"

int AppIterate_Warning(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);

    float deltaTime = Madokawaii::Platform::Graphics::GetFrameTime();
    ctx.warningState.elapsedTime += deltaTime;

    bool canSkip = ctx.warningState.elapsedTime >= WarningState::MIN_DISPLAY_TIME;
    bool autoSkip = ctx.warningState.elapsedTime >= WarningState::AUTO_SKIP_TIME;

    if (autoSkip || (canSkip && Madokawaii::Platform::Core::IsAnyKeyPressed())) {
        ctx.warningShown = true;
        return !Madokawaii::Platform::Core::WindowShouldClose();
    }

    Madokawaii::Platform::Graphics::BeginDrawing();
    Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);

    constexpr auto title = "光敏性癫痫警告";
    const auto& font = ctx.chineseFont;
    const float centerX = ctx.screenWidth / 2.0f;

    float titleFontSize = 48.0f;
    auto titleSize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, title, titleFontSize, 1.0f);
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(font, title,
        centerX - titleSize.x / 2, 120, titleFontSize, 1.0f,
        Madokawaii::Platform::Graphics::M_RED);

    constexpr auto title_eng = "Epilepsy Warning";
    Madokawaii::Platform::Graphics::DrawText(title_eng, 1920 / 2 - 80, 180, 20, Madokawaii::Platform::Graphics::M_RED);


    constexpr char warningLines[14][256] = {
        "请在使用此软件(Madokawaii)前阅读:",
        "",
        "当暴露在特定光影图案或闪光光亮下时，有极小部分人群会引发癫痫。",
        "这种情形可能是由于某些未查出的癫痫症状引起，",
        "即使该人员并没有患癫痫病史也有可能造成此类病症。",
        "",
        "如果您的家人或任何家庭成员曾有过类似症状，",
        "请在使用此软件前咨询您的医生或医师。",
        "",
        "如果您在使用此软件过程中出现任何症状，",
        "包括头晕、目眩、眼部或肌肉抽搐、失去意识、失去方向感、抽搐、",
        "或出现任何自己无法控制的动作，",
        "请立即停止使用此软件并在继续游戏前咨询您的医生或医师。",
        "",
    };

    for (int i = 0; i < std::size(warningLines); i++) {
        constexpr int startY = 250;
        constexpr int lineHeight = 46;
        constexpr int textFontSize = 36;
        auto [x, y] = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(
            font, warningLines[i], textFontSize, 1.0f); // NOLINT(*-narrowing-conversions)
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(font, warningLines[i],
            centerX - x / 2, startY + i * lineHeight, textFontSize, 1.0f, // NOLINT(*-narrowing-conversions)
            Madokawaii::Platform::Graphics::M_LIGHTGRAY);
    }
    const char* skipHint = canSkip ? "按任意键继续..." : "请等待...";
    constexpr float hintFontSize = 28.0f;
    const auto [x, y] = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, skipHint, hintFontSize, 1.0f);
    const float hintY = ctx.screenHeight - 120.0f;
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(font, skipHint,
       centerX - x / 2, hintY, hintFontSize, 1.0f,
       Madokawaii::Platform::Graphics::M_GRAY);

    if (!canSkip) {
        const float remaining = WarningState::MIN_DISPLAY_TIME - ctx.warningState.elapsedTime;
        char timeText[32];
        snprintf(timeText, sizeof(timeText), "%.1f", remaining);
        Madokawaii::Platform::Graphics::DrawText(timeText, ctx.screenWidth / 2 - 20, hintY + 40, 24, // NOLINT(*-narrowing-conversions)
                                                 Madokawaii::Platform::Graphics::M_DARKGRAY);
    }

    Madokawaii::Platform::Graphics::EndDrawing();
    return !Madokawaii::Platform::Core::WindowShouldClose();
}
