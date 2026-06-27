//
// 你，你不会真的要用这个东西吧？
//

#include "gdiplus_platform.hpp"

#include "Madokawaii/platform/fonts.hpp"

#include <algorithm>

namespace Madokawaii::Platform::Graphics::Fonts {
    using GdiPlusBackend::FontData;

    namespace {
        FontData* AsFontData(Font font) {
            return static_cast<FontData*>(font.implementationDefined);
        }

        Font Wrap(FontData* data) {
            Font font{};
            font.implementationDefined = data;
            return font;
        }

        std::wstring FamilyNameFromCollection(Gdiplus::PrivateFontCollection* collection) {
            if (!collection || collection->GetFamilyCount() <= 0) return {};

            INT found = 0;
            std::vector<Gdiplus::FontFamily> families(static_cast<std::size_t>(collection->GetFamilyCount()));
            if (collection->GetFamilies(collection->GetFamilyCount(), families.data(), &found) != Gdiplus::Ok || found <= 0) {
                return {};
            }

            wchar_t name[LF_FACESIZE]{};
            if (families[0].GetFamilyName(name) != Gdiplus::Ok) return {};
            return name;
        }

        std::unique_ptr<Gdiplus::FontFamily> CreateFamily(const FontData* data) {
            if (data && data->valid && !data->familyName.empty()) {
                auto family = std::make_unique<Gdiplus::FontFamily>(data->familyName.c_str(), data->collection.get());
                if (family->IsAvailable()) return family;
            }

            auto fallback = std::make_unique<Gdiplus::FontFamily>(L"Segoe UI");
            if (fallback->IsAvailable()) return fallback;
            return std::make_unique<Gdiplus::FontFamily>(L"Arial");
        }

        Gdiplus::Graphics* GraphicsForMeasure(std::unique_ptr<Gdiplus::Bitmap>& bitmap, std::unique_ptr<Gdiplus::Graphics>& graphics) {
            if (auto* active = GdiPlusBackend::ActiveGraphics()) return active;
            bitmap = std::make_unique<Gdiplus::Bitmap>(1, 1, PixelFormat32bppPARGB);
            graphics = std::make_unique<Gdiplus::Graphics>(bitmap.get());
            return graphics.get();
        }
    }

    Font LoadFont(const char* fontPath) {
        return LoadFontEx(fontPath, 48, nullptr, 0);
    }

    Font LoadFontEx(const char* fontPath, int, int*, int) {
        GdiPlusBackend::Startup();

        auto* fontData = new FontData;
        if (fontPath) {
            fontData->collection = std::make_unique<Gdiplus::PrivateFontCollection>();
            const auto path = Direct2D::Utf8ToWide(fontPath);
            if (!path.empty() && fontData->collection->AddFontFile(path.c_str()) == Gdiplus::Ok) {
                fontData->familyName = FamilyNameFromCollection(fontData->collection.get());
                fontData->valid = !fontData->familyName.empty();
            }
        }

        if (!fontData->valid) {
            fontData->collection.reset();
            fontData->familyName = L"Segoe UI";
            fontData->valid = true;
        }

        return Wrap(fontData);
    }

    Font LoadFontWithChinese(const char* fontPath, int fontSize) {
        return LoadFontEx(fontPath, fontSize, nullptr, 0);
    }

    void UnloadFont(Font font) {
        delete AsFontData(font);
    }

    void DrawTextEx(Font font, const char* text, float x, float y, float fontSize, float spacing, Color color) {
        auto* fontData = AsFontData(font);
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!graphics || !text) return;

        const auto wideText = Direct2D::Utf8ToWide(text);
        if (wideText.empty()) return;

        auto family = CreateFamily(fontData);
        Gdiplus::Font gdiplusFont(family.get(), std::max(1.0f, fontSize), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brush(GdiPlusBackend::ToGdiColor(color));

        if (spacing == 0.0f || wideText.size() <= 1) {
            graphics->DrawString(
                wideText.c_str(),
                static_cast<INT>(wideText.size()),
                &gdiplusFont,
                Gdiplus::PointF(x, y),
                &brush);
            return;
        }

        float cursor = x;
        for (wchar_t ch : wideText) {
            const wchar_t oneChar[] = {ch, L'\0'};
            graphics->DrawString(oneChar, 1, &gdiplusFont, Gdiplus::PointF(cursor, y), &brush);

            Gdiplus::RectF bounds{};
            graphics->MeasureString(oneChar, 1, &gdiplusFont, Gdiplus::PointF(0.0f, 0.0f), &bounds);
            cursor += bounds.Width + spacing;
        }
    }

    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
        auto* fontData = AsFontData(font);
        if (!text) return {0.0f, 0.0f};

        const auto wideText = Direct2D::Utf8ToWide(text);
        if (wideText.empty()) return {0.0f, 0.0f};

        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        std::unique_ptr<Gdiplus::Graphics> fallbackGraphics;
        auto* graphics = GraphicsForMeasure(bitmap, fallbackGraphics);
        if (!graphics) return {0.0f, 0.0f};

        auto family = CreateFamily(fontData);
        Gdiplus::Font gdiplusFont(family.get(), std::max(1.0f, fontSize), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::RectF bounds{};
        graphics->MeasureString(
            wideText.c_str(),
            static_cast<INT>(wideText.size()),
            &gdiplusFont,
            Gdiplus::PointF(0.0f, 0.0f),
            &bounds);

        const auto extraSpacing = wideText.size() > 1
            ? spacing * static_cast<float>(wideText.size() - 1)
            : 0.0f;
        return {bounds.Width + extraSpacing, bounds.Height};
    }

    bool IsFontValid(Font font) {
        auto* fontData = AsFontData(font);
        return fontData && fontData->valid;
    }
}
