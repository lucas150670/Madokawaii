//
// 你，你不会真的要用这个东西吧？
//

#include "gdiplus_platform.hpp"

#include "Madokawaii/platform/graphics.hpp"

#include <algorithm>
#include <format>

namespace Madokawaii::Platform::Graphics {
    namespace {
        std::unique_ptr<Gdiplus::FontFamily> CreateFontFamily(const wchar_t* name) {
            auto family = std::make_unique<Gdiplus::FontFamily>(name);
            if (family->IsAvailable()) return family;
            return std::make_unique<Gdiplus::FontFamily>(L"Arial");
        }

        Gdiplus::Graphics* GraphicsForMeasure(
            std::unique_ptr<Gdiplus::Bitmap>& bitmap,
            std::unique_ptr<Gdiplus::Graphics>& fallback) {
            if (auto* graphics = GdiPlusBackend::ActiveGraphics()) return graphics;
            bitmap = std::make_unique<Gdiplus::Bitmap>(1, 1, PixelFormat32bppPARGB);
            fallback = std::make_unique<Gdiplus::Graphics>(bitmap.get());
            return fallback.get();
        }
    }

    std::string GetImplementer() {
        return "GDI+(CPU)";
    }

    std::string GetImplementationInfo() {
        return "我操不会真有人用这个后端吧";
    }

    float GetFPS() {
        return GdiPlusBackend::GetState().fps;
    }

    float GetFrameTime() {
        return GdiPlusBackend::GetState().frameTime;
    }

    void SetTargetFPS(int target) {
        GdiPlusBackend::SetTargetFrameRate(target);
    }

    void BeginDrawing() {
        GdiPlusBackend::BeginFrame();
    }

    void EndDrawing() {
        GdiPlusBackend::EndFrame();
    }

    void ClearBackground(Color color) {
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics) return;
        graphics->Clear(GdiPlusBackend::ToGdiColor(color));
    }

    void DrawText(const char* text, int x, int y, int fontSize, Color color) {
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics || !text) return;

        const auto wideText = Direct2D::Utf8ToWide(text);
        if (wideText.empty()) return;

        auto family = CreateFontFamily(L"Segoe UI");
        Gdiplus::Font font(family.get(), static_cast<Gdiplus::REAL>(std::max(1, fontSize)), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brush(GdiPlusBackend::ToGdiColor(color));
        graphics->DrawString(
            wideText.c_str(),
            static_cast<INT>(wideText.size()),
            &font,
            Gdiplus::PointF(static_cast<Gdiplus::REAL>(x), static_cast<Gdiplus::REAL>(y)),
            &brush);
    }

    void DrawLineEx(Vector2 start, Vector2 end, float thick, Color color) {
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics) return;

        Gdiplus::Pen pen(GdiPlusBackend::ToGdiColor(color), std::max(1.0f, thick));
        graphics->DrawLine(
            &pen,
            Gdiplus::PointF(start.x, start.y),
            Gdiplus::PointF(end.x, end.y));
    }

    int MeasureText(const char* text, int fontSize) {
        if (!text) return 0;
        const auto wideText = Direct2D::Utf8ToWide(text);
        if (wideText.empty()) return 0;

        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        std::unique_ptr<Gdiplus::Graphics> fallback;
        auto* graphics = GraphicsForMeasure(bitmap, fallback);
        if (!graphics) return 0;

        auto family = CreateFontFamily(L"Segoe UI");
        Gdiplus::Font font(family.get(), static_cast<Gdiplus::REAL>(std::max(1, fontSize)), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::RectF bounds{};
        graphics->MeasureString(
            wideText.c_str(),
            static_cast<INT>(wideText.size()),
            &font,
            Gdiplus::PointF(0.0f, 0.0f),
            &bounds);
        return static_cast<int>(bounds.Width + 0.5f);
    }

    void DrawRectangle(int posX, int posY, int width, int height, Color color) {
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics || width <= 0 || height <= 0 || color.a == 0) return;

        Gdiplus::SolidBrush brush(GdiPlusBackend::ToGdiColor(color));
        graphics->FillRectangle(
            &brush,
            static_cast<Gdiplus::REAL>(posX),
            static_cast<Gdiplus::REAL>(posY),
            static_cast<Gdiplus::REAL>(width),
            static_cast<Gdiplus::REAL>(height));
    }

    void SetTransform(float x, float y, float rotate, float scaleX, float scaleY) {
        auto& state = GdiPlusBackend::GetState();
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics) return;

        state.transformStack.push_back(state.currentTransform);
        state.currentTransform = GdiPlusBackend::ComposeTransform(
            state.currentTransform,
            GdiPlusBackend::TransformState{x, y, rotate, scaleX, scaleY});
        graphics->ResetTransform();
    }

    void PopTransform() {
        auto& state = GdiPlusBackend::GetState();
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics || state.transformStack.empty()) return;

        state.currentTransform = state.transformStack.back();
        state.transformStack.pop_back();
        graphics->ResetTransform();
    }
}
