//
// Direct2D backend font implementation.
//

#include "direct2d_platform.hpp"

#include "Madokawaii/platform/fonts.hpp"

#include <algorithm>
#include <vector>

#include <d2d1helper.h>

namespace Madokawaii::Platform::Graphics::Fonts {
    using Direct2D::FontData;

    namespace {
        FontData* AsFontData(Font font) {
            return static_cast<FontData*>(font.implementationDefined);
        }

        float FontScale(IDWriteFontFace* face, float fontSize) {
            if (!face) return 0.0f;
            DWRITE_FONT_METRICS metrics{};
            face->GetMetrics(&metrics);
            return fontSize / static_cast<float>(metrics.designUnitsPerEm);
        }

        std::vector<UINT32> ToCodePoints(const std::wstring& text) {
            std::vector<UINT32> codePoints;
            codePoints.reserve(text.size());
            for (std::size_t i = 0; i < text.size(); ++i) {
                const auto current = static_cast<UINT32>(text[i]);
                if (current >= 0xD800 && current <= 0xDBFF && i + 1 < text.size()) {
                    const auto next = static_cast<UINT32>(text[i + 1]);
                    if (next >= 0xDC00 && next <= 0xDFFF) {
                        codePoints.push_back(0x10000 + ((current - 0xD800) << 10) + (next - 0xDC00));
                        ++i;
                        continue;
                    }
                }
                codePoints.push_back(current);
            }
            return codePoints;
        }

        Vector2 MeasureGlyphs(IDWriteFontFace* face, const std::wstring& text, float fontSize, float spacing) {
            if (!face || text.empty()) return {0.0f, 0.0f};

            const auto codePoints = ToCodePoints(text);
            std::vector<UINT16> glyphs(codePoints.size());
            std::vector<DWRITE_GLYPH_METRICS> glyphMetrics(codePoints.size());
            if (FAILED(face->GetGlyphIndices(codePoints.data(), static_cast<UINT32>(codePoints.size()), glyphs.data()))) {
                return {0.0f, 0.0f};
            }
            if (FAILED(face->GetDesignGlyphMetrics(glyphs.data(), static_cast<UINT32>(glyphs.size()), glyphMetrics.data(), FALSE))) {
                return {0.0f, 0.0f};
            }

            const auto scale = FontScale(face, fontSize);
            float width = 0.0f;
            for (std::size_t i = 0; i < glyphMetrics.size(); ++i) {
                width += glyphMetrics[i].advanceWidth * scale;
                if (i + 1 < glyphMetrics.size()) width += spacing;
            }

            DWRITE_FONT_METRICS metrics{};
            face->GetMetrics(&metrics);
            const auto height = (metrics.ascent + metrics.descent + metrics.lineGap) * scale;
            return {width, height};
        }
    }

    Font LoadFont(const char* fontPath) {
        return LoadFontEx(fontPath, 48, nullptr, 0);
    }

    Font LoadFontEx(const char* fontPath, int, int*, int) {
        Font font{};
        auto* factory = Direct2D::WriteFactory();
        if (!factory || !fontPath) return font;

        auto* fontData = new FontData;
        IDWriteFontFile* fontFile{};
        BOOL supported = FALSE;
        DWRITE_FONT_FILE_TYPE fileType{};
        DWRITE_FONT_FACE_TYPE faceType{};
        UINT32 faceCount = 0;

        const auto path = Direct2D::Utf8ToWide(fontPath);
        if (SUCCEEDED(factory->CreateFontFileReference(path.c_str(), nullptr, &fontFile))
            && SUCCEEDED(fontFile->Analyze(&supported, &fileType, &faceType, &faceCount))
            && supported
            && faceCount > 0
            && SUCCEEDED(factory->CreateFontFace(
                faceType,
                1,
                &fontFile,
                0,
                DWRITE_FONT_SIMULATIONS_NONE,
                &fontData->face))) {
            fontData->valid = true;
        }

        Direct2D::SafeRelease(fontFile);
        if (!fontData->valid) {
            delete fontData;
            return font;
        }

        font.implementationDefined = fontData;
        return font;
    }

    Font LoadFontWithChinese(const char* fontPath, int fontSize) {
        return LoadFontEx(fontPath, fontSize, nullptr, 0);
    }

    void UnloadFont(Font font) {
        delete AsFontData(font);
    }

    void DrawTextEx(Font font, const char* text, float x, float y, float fontSize, float spacing, Color color) {
        auto* fontData = AsFontData(font);
        auto* renderTarget = Direct2D::RenderTarget();
        auto* brush = Direct2D::CreateBrush(color);
        if (!fontData || !fontData->face || !renderTarget || !brush || !text) {
            Direct2D::SafeRelease(brush);
            return;
        }

        const auto wideText = Direct2D::Utf8ToWide(text);
        if (wideText.empty()) {
            Direct2D::SafeRelease(brush);
            return;
        }

        const auto codePoints = ToCodePoints(wideText);
        std::vector<UINT16> glyphs(codePoints.size());
        std::vector<DWRITE_GLYPH_METRICS> glyphMetrics(codePoints.size());
        std::vector<FLOAT> advances(codePoints.size(), 0.0f);
        std::vector<DWRITE_GLYPH_OFFSET> offsets(codePoints.size(), DWRITE_GLYPH_OFFSET{0.0f, 0.0f});

        if (FAILED(fontData->face->GetGlyphIndices(codePoints.data(), static_cast<UINT32>(codePoints.size()), glyphs.data()))
            || FAILED(fontData->face->GetDesignGlyphMetrics(glyphs.data(), static_cast<UINT32>(glyphs.size()), glyphMetrics.data(), FALSE))) {
            Direct2D::SafeRelease(brush);
            return;
        }

        const auto scale = FontScale(fontData->face, fontSize);
        for (std::size_t i = 0; i < glyphMetrics.size(); ++i) {
            advances[i] = glyphMetrics[i].advanceWidth * scale;
            if (i + 1 < glyphMetrics.size()) advances[i] += spacing;
        }

        DWRITE_FONT_METRICS metrics{};
        fontData->face->GetMetrics(&metrics);
        const auto baseline = y + metrics.ascent * scale;

        DWRITE_GLYPH_RUN glyphRun{};
        glyphRun.fontFace = fontData->face;
        glyphRun.fontEmSize = fontSize;
        glyphRun.glyphCount = static_cast<UINT32>(glyphs.size());
        glyphRun.glyphIndices = glyphs.data();
        glyphRun.glyphAdvances = advances.data();
        glyphRun.glyphOffsets = offsets.data();
        glyphRun.isSideways = FALSE;
        glyphRun.bidiLevel = 0;

        renderTarget->DrawGlyphRun(
            D2D1::Point2F(x, baseline),
            &glyphRun,
            brush,
            DWRITE_MEASURING_MODE_NATURAL);

        Direct2D::SafeRelease(brush);
    }

    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
        auto* fontData = AsFontData(font);
        if (!fontData || !fontData->face || !text) return {0.0f, 0.0f};
        return MeasureGlyphs(fontData->face, Direct2D::Utf8ToWide(text), fontSize, spacing);
    }

    bool IsFontValid(Font font) {
        auto* fontData = AsFontData(font);
        return fontData && fontData->valid && fontData->face;
    }
}
