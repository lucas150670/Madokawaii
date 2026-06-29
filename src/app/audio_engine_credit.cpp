//
// Created by madoka on 2026/6/29.
//

#include "Madokawaii/app/audio_engine_credit.hpp"

#include "Madokawaii/app/common.hpp"
#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/platform/core.hpp"
#include "Madokawaii/platform/log.hpp"

#include <algorithm>
#include <cstdint>

namespace Madokawaii::App::AudioEngineCredit {

namespace {
    Platform::Graphics::Color WithAlpha(Platform::Graphics::Color color, float alpha) {
        color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
        return color;
    }

    void Initialize(AppContext& ctx) {
        auto& state = ctx.ui.fmodCredit;
        if (state.initialized) return;

        state.initialized = true;
        state.attributionText = Platform::Audio::GetAudioEngineAttributionInfo();

        const auto logoPath = Platform::Audio::GetAudioEngineLogoPath();
        if (logoPath.empty()) {
            Platform::Log::TraceLog(
                Platform::Log::TraceLogLevel::LOG_WARNING,
                "FMOD_CREDIT: Audio engine attribution is required, but logo path is empty.");
            return;
        }

        if (!Platform::Core::FileExists(logoPath.c_str())) {
            Platform::Log::TraceLog(
                Platform::Log::TraceLogLevel::LOG_WARNING,
                "FMOD_CREDIT: Audio engine logo does not exist: %s",
                logoPath.c_str());
            return;
        }

        state.logoTexture = Platform::Graphics::Texture::LoadTexture(logoPath.c_str());
        Platform::Graphics::Vector2 logoDimension{};
        Platform::Graphics::Texture::MeasureTexture2D(state.logoTexture, &logoDimension);
        if (logoDimension.x <= 0.0f || logoDimension.y <= 0.0f) {
            Platform::Log::TraceLog(
                Platform::Log::TraceLogLevel::LOG_WARNING,
                "FMOD_CREDIT: Failed to load audio engine logo: %s",
                logoPath.c_str());
            Unload(ctx);
        }
    }

    void Complete(AppContext& ctx) {
        Unload(ctx);
        ctx.ui.fmodCreditCompleted = true;
    }
}

void Unload(AppContext& ctx) {
    auto& state = ctx.ui.fmodCredit;
    if (state.logoTexture.implementationDefinedData) {
        Platform::Graphics::Texture::UnloadTexture(state.logoTexture);
        state.logoTexture = {};
    }
}

int Iterate(AppContext& ctx) {
    if (ctx.ui.fmodCreditCompleted) {
        return !Platform::Core::WindowShouldClose();
    }

    if (!Platform::Audio::AudioEngineNeedAttribution()) {
        ctx.ui.fmodCreditCompleted = true;
        return !Platform::Core::WindowShouldClose();
    }

    auto& state = ctx.ui.fmodCredit;
    Initialize(ctx);

    state.elapsedTime += Platform::Graphics::GetFrameTime();
    constexpr float totalDisplayTime = AudioEngineCreditState::FADE_IN_TIME  + AudioEngineCreditState::DISPLAY_TIME + AudioEngineCreditState::FADE_OUT_TIME;
    if (state.elapsedTime >= totalDisplayTime) {
        Complete(ctx);
        return !Platform::Core::WindowShouldClose();
    }

    float alpha = 1.0f;
    if (state.elapsedTime < AudioEngineCreditState::FADE_IN_TIME) {
        alpha = state.elapsedTime / AudioEngineCreditState::FADE_IN_TIME;
    } else if (state.elapsedTime < AudioEngineCreditState::FADE_IN_TIME + AudioEngineCreditState::DISPLAY_TIME) {
        alpha = 1.0f;
    }
    else {
        alpha = 1.0f - (state.elapsedTime - AudioEngineCreditState::FADE_IN_TIME - AudioEngineCreditState::DISPLAY_TIME) / AudioEngineCreditState::FADE_OUT_TIME;
    }

    Platform::Graphics::BeginDrawing();
    Platform::Graphics::ClearBackground(Platform::Graphics::M_BLACK);

    const float scaleX = ctx.display.screenWidth / 1920.0f;
    const float scaleY = ctx.display.screenHeight / 1080.0f;
    const float scale = std::min(scaleX, scaleY);
    const float centerX = ctx.display.screenWidth / 2.0f;
    const float centerY = ctx.display.screenHeight / 2.0f;

    Platform::Graphics::Vector2 logoDimension{};
    float logoScale = 1.0f;
    bool hasLogo = state.logoTexture.implementationDefinedData != nullptr;
    if (hasLogo) {
        Platform::Graphics::Texture::MeasureTexture2D(state.logoTexture, &logoDimension);
        hasLogo = logoDimension.x > 0.0f && logoDimension.y > 0.0f;
    }

    if (hasLogo) {
        const float maxLogoWidth = ctx.display.screenWidth * 0.48f;
        const float maxLogoHeight = ctx.display.screenHeight * 0.24f;
        logoScale = std::min(maxLogoWidth / logoDimension.x, maxLogoHeight / logoDimension.y);
        logoScale = std::min(logoScale, 1.0f);
    }

    const auto& font = ctx.assets.chineseFont;
    const bool hasAttributionText = !state.attributionText.empty();
    float textFontSize = 28.0f * scale;
    Platform::Graphics::Vector2 textDimension{};
    if (hasAttributionText) {
        textDimension = Platform::Graphics::Fonts::MeasureTextEx(
            font,
            state.attributionText.c_str(),
            textFontSize,
            1.0f);
        const float maxTextWidth = ctx.display.screenWidth * 0.86f;
        if (textDimension.x > maxTextWidth && textDimension.x > 0.0f) {
            textFontSize *= maxTextWidth / textDimension.x;
            textDimension = Platform::Graphics::Fonts::MeasureTextEx(
                font,
                state.attributionText.c_str(),
                textFontSize,
                1.0f);
        }
    }

    const float logoRenderedWidth = hasLogo ? logoDimension.x * logoScale : 0.0f;
    const float logoRenderedHeight = hasLogo ? logoDimension.y * logoScale : 0.0f;
    const float gap = hasLogo && hasAttributionText ? 36.0f * scale : 0.0f;
    const float totalContentHeight = logoRenderedHeight + gap + (hasAttributionText ? textDimension.y : 0.0f);
    float contentY = centerY - totalContentHeight / 2.0f;

    const auto tint = WithAlpha(Platform::Graphics::M_WHITE, alpha);
    if (hasLogo) {
        Platform::Graphics::Texture::DrawTextureEx(
            state.logoTexture,
            {centerX - logoRenderedWidth / 2.0f, contentY},
            0.0f,
            logoScale,
            tint);
        contentY += logoRenderedHeight + gap;
    }

    if (hasAttributionText) {
        Platform::Graphics::Fonts::DrawTextEx(
            font,
            state.attributionText.c_str(),
            centerX - textDimension.x / 2.0f,
            contentY,
            textFontSize,
            1.0f,
            WithAlpha(Platform::Graphics::M_LIGHTGRAY, alpha));
    }

    Platform::Graphics::EndDrawing();
    return !Platform::Core::WindowShouldClose();
}

} // namespace Madokawaii::App::FmodCredit
