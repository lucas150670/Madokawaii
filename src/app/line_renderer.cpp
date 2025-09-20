//
// Created by madoka on 2025/9/19.
//

#include <cmath>
#include <format>

#include "Madokawaii/app/chart.h"
#include "Madokawaii/app/line_operation.h"
#include "Madokawaii/platform/graphics.h"

void RenderJudgeline(const Madokawaii::Defs::chart::judgeline& judgeline, int screenWidth, int screenHeight) {
    auto c = Madokawaii::Platform::Graphics::M_RAYWHITE;
    c.a = static_cast<unsigned char>(judgeline.info.opacity * 255);
    auto screenX = static_cast<float>(judgeline.info.posX * screenWidth),
         screenY = static_cast<float>((1 - judgeline.info.posY) * screenHeight),
         aspectRatio = screenWidth * 1.0f / screenHeight; // NOLINT(*-narrowing-conversions)
    if (fabs(judgeline.info.rotateAngle) < 1e-6 || fabs(judgeline.info.rotateAngle - 180.0f) < 1e-6) {
        DrawLineEx(Madokawaii::Platform::Graphics::Vector2{screenX - 5000, screenY}, Madokawaii::Platform::Graphics::Vector2{screenX + 5000, screenY}, 10.0f, c);
    } else if (fabs(judgeline.info.rotateAngle - 90.0f) < 1e-6 || fabs(judgeline.info.rotateAngle - 270.0f) < 1e-6) {
        DrawLineEx(Madokawaii::Platform::Graphics::Vector2{screenX, screenY - 5000}, Madokawaii::Platform::Graphics::Vector2{screenX, screenY + 5000}, 10.0f, c);
    } else {
        float k, kx0, y0;
        k = static_cast<float>(tan(judgeline.info.rotateAngle / 180.0 * M_PI) * aspectRatio);
        kx0 = static_cast<float>(k * judgeline.info.posX),
        y0 = static_cast<float>(judgeline.info.posY);
        Madokawaii::Platform::Graphics::Vector2 p1{}, p2{};
        p1 = {-10.0f * screenWidth, screenHeight - (-10 * k - kx0 + y0) * screenHeight}; // NOLINT(*-narrowing-conversions)
        p2 = {10.0f * screenWidth, screenHeight - (10 * k - kx0 + y0) * screenHeight}; // NOLINT(*-narrowing-conversions)
        DrawLineEx(p1, p2, 10.0f, c);
    }

    // 画线ID
    auto idStr = std::format("{}", judgeline.id);
    Madokawaii::Platform::Graphics::DrawText(idStr.c_str(), static_cast<int>(screenX), static_cast<int>(screenY), 30, Madokawaii::Platform::Graphics::M_RED);
}

void RenderDebugInfo() {
    static auto glVersion = Madokawaii::Platform::Graphics::GetImplementationInfo(),
         glRenderer = Madokawaii::Platform::Graphics::GetImplementer();

    auto dbgStr = std::format("Implementer: {}", glRenderer);
    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190, 170, 20, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
    dbgStr = std::format("Implementation Version: {}", glVersion);
    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190, 200, 20, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
    dbgStr = std::format("FPS: {}, FrameTime: {}s", Madokawaii::Platform::Graphics::GetFPS(), Madokawaii::Platform::Graphics::GetFrameTime());
    Madokawaii::Platform::Graphics::DrawText(dbgStr.c_str(), 190, 230, 20, Madokawaii::Platform::Graphics::M_LIGHTGRAY);
}
