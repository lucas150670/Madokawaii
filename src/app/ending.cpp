//
// Created by madoka on 2026/1/10.
//

#include "Madokawaii/app/common.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/fonts.h"
#include "Madokawaii/platform/texture.h"
#include "Madokawaii/platform/core.h"
#include <format>

namespace {
    // Ending页面状态
    struct EndingState {
        bool initialized{ false };
        Madokawaii::Platform::Audio::Music endingMusic{};
    };

    EndingState endingState{};

    void DrawTrapezoid(float x, float y, float width, float height,
        float topOffset, float bottomOffset,
        Madokawaii::Platform::Graphics::Color color) {
        for (int i = 0; i < static_cast<int>(height); i++) {
            float t = static_cast<float>(i) / height;
            float lineX = x + topOffset * (1.0f - t) + bottomOffset * t;
            float lineWidth = width - topOffset * (1.0f - t) - bottomOffset * t + topOffset;
            Madokawaii::Platform::Graphics::DrawRectangle(
                static_cast<int>(lineX),
                static_cast<int>(y + i),
                static_cast<int>(lineWidth),
                1,
                color
            );
        }
    }

    void DrawJudgementItem(Madokawaii::Platform::Graphics::Fonts::Font font,
        const char* label, int value, float centerX, float y,
        float labelFontSize, float valueFontSize, float spacing,
        Madokawaii::Platform::Graphics::Color labelColor,
        Madokawaii::Platform::Graphics::Color valueColor) {
        auto labelSize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, label, labelFontSize, spacing);
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(
            font,
            label,
            centerX - labelSize.x / 2.0f,
            y,
            labelFontSize,
            spacing,
            labelColor
        );

        auto valueText = std::format("{}", value);
        auto valueSize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, valueText.c_str(), valueFontSize, spacing);
        float valueY = y + labelFontSize + 10.0f;
        Madokawaii::Platform::Graphics::Fonts::DrawTextEx(
            font,
            valueText.c_str(),
            centerX - valueSize.x / 2.0f,
            valueY,
            valueFontSize,
            spacing,
            valueColor
        );
    }

}

int AppIterate_Ending(AppContext* context) {
    if (!context) return -1;

    if (!endingState.initialized) {
        if (context->global_respack && context->global_respack->musicEnding) {
            endingState.endingMusic = Madokawaii::Platform::Audio::LoadMusicStreamFromMemory(
                ".mp3",
                static_cast<const unsigned char*>(context->global_respack->musicEnding->data),
                static_cast<int>(context->global_respack->musicEnding->size)
            );
            endingState.endingMusic.looping = true;
            Madokawaii::Platform::Audio::SetMusicVolume(endingState.endingMusic, 0.7f);
            Madokawaii::Platform::Audio::PlayMusicStream(endingState.endingMusic);
        }
        endingState.initialized = true;
    }

    if (endingState.endingMusic.implementationDefined) {
        Madokawaii::Platform::Audio::UpdateMusicStream(endingState.endingMusic);
    }
    Madokawaii::Platform::Graphics::BeginDrawing();
    Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);

    if (context->backgroundTexture.implementationDefinedData) {
        Madokawaii::Platform::Graphics::Texture::DrawTexture(
            context->backgroundTexture,
            { 0, 0 },
            { 255, 255, 255, 255 }
        );
    }

    float screenWidth = static_cast<float>(context->screenWidth);
    float screenHeight = static_cast<float>(context->screenHeight);

    const float trapezoidHeight = context->screenHeight;
    constexpr float topOffset = 120.0f;
    constexpr float bottomOffset = 0.0f;

    float trapezoidX = screenWidth * 0.35f;
    float trapezoidWidth = screenWidth - trapezoidX + topOffset;
    float trapezoidY = screenHeight / 2.0f - trapezoidHeight / 2.0f;

    Madokawaii::Platform::Graphics::Color trapezoidColor = { 80, 80, 80, 200 };
    DrawTrapezoid(trapezoidX, trapezoidY, trapezoidWidth, trapezoidHeight,
        topOffset, bottomOffset, trapezoidColor);

    float textCenterX = trapezoidX + topOffset / 2.0f + (trapezoidWidth - topOffset) / 2.0f;

    constexpr float titleFontSize = 48.0f;
    constexpr float difficultyFontSize = 28.0f;
    constexpr float rightPadding = 40.0f;
    constexpr float topPadding = 40.0f;
    constexpr float fontSpacing = 2.0f;

    auto& font = context->chineseFont;

    // TODO: replace "untitled" with actual song title
    const char* songTitle = "untitled";
    auto titleSize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, songTitle, titleFontSize, fontSpacing);
    float titleX = screenWidth - titleSize.x - rightPadding;
    float titleY = topPadding;
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(
        font,
        songTitle,
        titleX,
        titleY,
        titleFontSize,
        fontSpacing,
        Madokawaii::Platform::Graphics::M_RAYWHITE
    );

    // TODO: replace sp lv.? with actual difficulty
    const char* difficulty = "SP Lv.?";
    auto difficultySize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, difficulty, difficultyFontSize, fontSpacing);
    float difficultyX = screenWidth - difficultySize.x - rightPadding;
    float difficultyY = titleY + titleFontSize + 10.0f;
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(
        font,
        difficulty,
        difficultyX,
        difficultyY,
        difficultyFontSize,
        fontSpacing,
        Madokawaii::Platform::Graphics::M_PURPLE
    );

    auto scoreText = std::format("{}", 1000000);
    constexpr float scoreFontSize = 72.0f;
    auto scoreSize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, scoreText.c_str(), scoreFontSize, fontSpacing);
    float scoreX = textCenterX - scoreSize.x / 2.0f;
    float scoreY = trapezoidY + 400;
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(
        font,
        scoreText.c_str(),
        scoreX,
        scoreY,
        scoreFontSize,
        fontSpacing,
        Madokawaii::Platform::Graphics::M_GOLD
    );

    const char* autoPlayText = "AUTO PLAY";
    constexpr float autoPlayFontSize = 40.0f;
    auto autoPlaySize = Madokawaii::Platform::Graphics::Fonts::MeasureTextEx(font, autoPlayText, autoPlayFontSize, fontSpacing);
    float autoPlayX = textCenterX - autoPlaySize.x / 2.0f;
    float autoPlayY = trapezoidY + 480;
    Madokawaii::Platform::Graphics::Fonts::DrawTextEx(
        font,
        autoPlayText,
        autoPlayX,
        autoPlayY,
        autoPlayFontSize,
        fontSpacing,
        Madokawaii::Platform::Graphics::M_GOLD
    );

    constexpr float labelFontSize = 28.0f;
    constexpr float valueFontSize = 36.0f;
    constexpr float itemSpacing = 150.0f;
    float judgementY = autoPlayY + 80.0f;

    float totalWidth = itemSpacing * 3;
    float startX = textCenterX - totalWidth / 2.0f;

    DrawJudgementItem(font, "Perfect", context->mainChart.numOfNotes, startX, judgementY,
        labelFontSize, valueFontSize, fontSpacing,
        Madokawaii::Platform::Graphics::M_GOLD,
        Madokawaii::Platform::Graphics::M_RAYWHITE);

    DrawJudgementItem(font, "Great", 0, startX + itemSpacing, judgementY,
        labelFontSize, valueFontSize, fontSpacing,
        Madokawaii::Platform::Graphics::M_SKYBLUE,
        Madokawaii::Platform::Graphics::M_RAYWHITE);

    DrawJudgementItem(font, "Bad", 0, startX + itemSpacing * 2, judgementY,
        labelFontSize, valueFontSize, fontSpacing,
        Madokawaii::Platform::Graphics::M_GRAY,
        Madokawaii::Platform::Graphics::M_RAYWHITE);

    DrawJudgementItem(font, "Miss", 0, startX + itemSpacing * 3, judgementY,
        labelFontSize, valueFontSize, fontSpacing,
        Madokawaii::Platform::Graphics::M_RED,
        Madokawaii::Platform::Graphics::M_RAYWHITE);


    Madokawaii::Platform::Graphics::EndDrawing();

    return !Madokawaii::Platform::Core::WindowShouldClose();
}