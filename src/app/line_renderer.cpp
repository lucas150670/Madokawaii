//
// Created by madoka on 2025/9/19.
//

#include <algorithm>
#include <cmath>
#include <format>

#include "Madokawaii/app/chart.hpp"
#include "Madokawaii/app/common.hpp"
#include "Madokawaii/app/coordinate.hpp"
#include "Madokawaii/app/line_operation.hpp"
#include "Madokawaii/platform/graphics.hpp"

namespace Madokawaii::App::Line {

void RenderJudgeline(const AppContext& context, const Madokawaii::App::chart::judgeline& judgeline) {
    const auto screenWidth = context.display.screenWidth;
    const auto screenHeight = context.display.screenHeight;
    auto perfectColor = context.assets.perfectColor;
    auto scaleX = screenWidth / 1920.f;
    auto scaleY = screenHeight / 1080.f;
    auto scale = std::min(scaleX, scaleY);
    auto width = 10.0f * scale;
    const auto viewport = Madokawaii::App::Coordinate::MakeScreenViewport(screenWidth, screenHeight);
    perfectColor.a = static_cast<unsigned char>(judgeline.info.opacity * 255);
    const auto screenPosition = Madokawaii::App::Coordinate::ToScreenPoint(
        {judgeline.info.posX, judgeline.info.posY},
        viewport);
    auto screenX = screenPosition.x,
         screenY = screenPosition.y,
         aspectRatio = static_cast<float>(viewport.aspectRatio());
    if (fabs(judgeline.info.rotateAngle) < 1e-6 || fabs(judgeline.info.rotateAngle - 180.0f) < 1e-6) {
        auto p1 = Madokawaii::App::Coordinate::ToScreenPoint({-10.0, judgeline.info.posY}, viewport);
        auto p2 = Madokawaii::App::Coordinate::ToScreenPoint({10.0, judgeline.info.posY}, viewport);
        DrawLineEx(p1, p2, width, perfectColor);
    } else if (fabs(judgeline.info.rotateAngle - 90.0f) < 1e-6 || fabs(judgeline.info.rotateAngle - 270.0f) < 1e-6) {
        auto p1 = Madokawaii::App::Coordinate::ToScreenPoint({judgeline.info.posX, -10.0}, viewport);
        auto p2 = Madokawaii::App::Coordinate::ToScreenPoint({judgeline.info.posX, 10.0}, viewport);
        DrawLineEx(p1, p2, width, perfectColor);
    } else {
        float k, kx0, y0;
        k = static_cast<float>(tan(judgeline.info.rotateAngle / 180.0 * M_PI) * aspectRatio);
        kx0 = static_cast<float>(k * judgeline.info.posX),
        y0 = static_cast<float>(judgeline.info.posY);
        Madokawaii::Platform::Graphics::Vector2 p1{}, p2{};
        p1 = Madokawaii::App::Coordinate::ToScreenPoint({-10.0, -10.0 * k - kx0 + y0}, viewport);
        p2 = Madokawaii::App::Coordinate::ToScreenPoint({10.0, 10.0 * k - kx0 + y0}, viewport);
        DrawLineEx(p1, p2, width, perfectColor);
    }

    // 画线ID
    auto idStr = std::format("{}", judgeline.id);
    Madokawaii::Platform::Graphics::DrawText(idStr.c_str(), static_cast<int>(screenX), static_cast<int>(screenY), 30, Madokawaii::Platform::Graphics::M_RED);
}

void RenderDebugInfo(const AppContext& context) {
    const auto screenWidth = context.display.screenWidth;
    const auto screenHeight = context.display.screenHeight;
    auto scaleX = screenWidth / 1920.f;
    auto scaleY = screenHeight / 1080.f;
    auto scale = std::min(scaleX, scaleY);

    const auto glVersion = Madokawaii::Platform::Graphics::GetImplementationInfo();
    const auto glRenderer = Madokawaii::Platform::Graphics::GetImplementer();
    const auto audioRenderer = Madokawaii::Platform::Audio::GetAudioEngineImplementer();

    auto dbgStr = std::format("Implementer: {}", glRenderer);

    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190 * scale, 170 * scale, 20 * scale, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
    dbgStr = std::format("Implementation Version: {}", glVersion);
    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190 * scale, 200 * scale, 20 * scale, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
    dbgStr = std::format("Audio: {}", audioRenderer);
    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190 * scale, 230 * scale, 20 * scale, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
    dbgStr = std::format("FPS: {:8.2f}, FrameTime: {:10.6f}s, 1% Low: {:8.2f}", Madokawaii::Platform::Graphics::GetFPS(), Madokawaii::Platform::Graphics::GetFrameTime(), Madokawaii::Platform::Graphics::GetOnePercentLowFPS());
    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190 * scale, 260 * scale, 20 * scale, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
}

} // namespace Madokawaii::App::Line
