//
// Created by madoka on 2026/1/10.
//

#include "Madokawaii/app/common.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/graphics.h"
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

    void DrawJudgementItem(const char* label, int value, float centerX, float y,
        int labelFontSize, int valueFontSize,
        Madokawaii::Platform::Graphics::Color labelColor,
        Madokawaii::Platform::Graphics::Color valueColor) {
        int labelWidth = Madokawaii::Platform::Graphics::MeasureText(label, labelFontSize);
        Madokawaii::Platform::Graphics::DrawText(
            label,
            static_cast<int>(centerX - labelWidth / 2.0f),
            static_cast<int>(y),
            labelFontSize,
            labelColor
        );

        auto valueText = std::format("{}", value);
        int valueWidth = Madokawaii::Platform::Graphics::MeasureText(valueText.c_str(), valueFontSize);
        float valueY = y + static_cast<float>(labelFontSize) + 10.0f;
        Madokawaii::Platform::Graphics::DrawText(
            valueText.c_str(),
            static_cast<int>(centerX - valueWidth / 2.0f),
            static_cast<int>(valueY),
            valueFontSize,
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

    constexpr int titleFontSize = 48;
    constexpr int difficultyFontSize = 28;
    constexpr float rightPadding = 40.0f;
    constexpr float topPadding = 40.0f;

	// TODO: replace "untitled" with actual song title
    const char* songTitle = "untitled";
    int titleWidth = Madokawaii::Platform::Graphics::MeasureText(songTitle, titleFontSize);
    float titleX = screenWidth - titleWidth - rightPadding;
    float titleY = topPadding;
    Madokawaii::Platform::Graphics::DrawText(
        songTitle,
        static_cast<int>(titleX),
        static_cast<int>(titleY),
        titleFontSize,
        Madokawaii::Platform::Graphics::M_RAYWHITE
    );

	// TODO: replace sp lv.? with actual difficulty
    const char* difficulty = "SP Lv.?";
    int difficultyWidth = Madokawaii::Platform::Graphics::MeasureText(difficulty, difficultyFontSize);
    float difficultyX = screenWidth - difficultyWidth - rightPadding;
    float difficultyY = titleY + titleFontSize + 10.0f;
    Madokawaii::Platform::Graphics::DrawText(
        difficulty,
        static_cast<int>(difficultyX),
        static_cast<int>(difficultyY),
        difficultyFontSize,
        Madokawaii::Platform::Graphics::M_PURPLE
    );

    auto scoreText = std::format("{}", 1000000);
    int scoreFontSize = 72;
    int scoreTextWidth = Madokawaii::Platform::Graphics::MeasureText(scoreText.c_str(), scoreFontSize);
    float scoreX = textCenterX - scoreTextWidth / 2.0f;
    float scoreY = trapezoidY + 400;
    Madokawaii::Platform::Graphics::DrawText(
        scoreText.c_str(),
        static_cast<int>(scoreX),
        static_cast<int>(scoreY),
        scoreFontSize,
        Madokawaii::Platform::Graphics::M_GOLD
    );

    const char* autoPlayText = "AUTO PLAY";
    int autoPlayFontSize = 40;
    int autoPlayTextWidth = Madokawaii::Platform::Graphics::MeasureText(autoPlayText, autoPlayFontSize);
    float autoPlayX = textCenterX - autoPlayTextWidth / 2.0f;
    float autoPlayY = trapezoidY + 480;
    Madokawaii::Platform::Graphics::DrawText(
        autoPlayText,
        static_cast<int>(autoPlayX),
        static_cast<int>(autoPlayY),
        autoPlayFontSize,
        Madokawaii::Platform::Graphics::M_GOLD
    );

    constexpr int labelFontSize = 28;
    constexpr int valueFontSize = 36;
    constexpr float itemSpacing = 150.0f;
    float judgementY = autoPlayY + 80.0f; 

    float totalWidth = itemSpacing * 3; 
    float startX = textCenterX - totalWidth / 2.0f;

    DrawJudgementItem("Perfect", context->mainChart.numOfNotes, startX, judgementY,
        labelFontSize, valueFontSize,
        Madokawaii::Platform::Graphics::M_GOLD,
        Madokawaii::Platform::Graphics::M_RAYWHITE);

    DrawJudgementItem("Great", 0, startX + itemSpacing, judgementY,
        labelFontSize, valueFontSize,
        Madokawaii::Platform::Graphics::M_SKYBLUE,
        Madokawaii::Platform::Graphics::M_RAYWHITE);

    DrawJudgementItem("Bad", 0, startX + itemSpacing * 2, judgementY,
        labelFontSize, valueFontSize,
        Madokawaii::Platform::Graphics::M_GRAY,
        Madokawaii::Platform::Graphics::M_RAYWHITE);

    DrawJudgementItem("Miss", 0, startX + itemSpacing * 3, judgementY,
        labelFontSize, valueFontSize,
        Madokawaii::Platform::Graphics::M_RED,
        Madokawaii::Platform::Graphics::M_RAYWHITE);


    Madokawaii::Platform::Graphics::EndDrawing();

    return !Madokawaii::Platform::Core::WindowShouldClose();
}