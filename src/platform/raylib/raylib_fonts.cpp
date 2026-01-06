//
// Created by madoka on 2025/9/19.
//

#include <raylib.h>
#include <vector>
#include "Madokawaii/platform/fonts.h"

namespace Madokawaii::Platform::Graphics::Fonts {

    static ::Color RL(Color c) { return {c.r, c.g, c.b, c.a}; }

    Font LoadFont(const char* fontPath) {
        Font font{};
        auto* rlFont = new ::Font();
        *rlFont = ::LoadFont(fontPath);
        SetTextureFilter(rlFont->texture, TEXTURE_FILTER_BILINEAR);
        font.implementationDefined = rlFont;
        return font;
    }

    Font LoadFontEx(const char* fontPath, int fontSize, int* codepoints, int codepointCount) {
        Font font{};
        auto* rlFont = new ::Font();
        *rlFont = ::LoadFontEx(fontPath, fontSize, codepoints, codepointCount);
        SetTextureFilter(rlFont->texture, TEXTURE_FILTER_BILINEAR);
        font.implementationDefined = rlFont;
        return font;
    }

    Font LoadFontWithChinese(const char* fontPath, int fontSize) {
        std::vector<int> codepoints;

        for (int i = 32; i <= 126; i++) {
            codepoints.push_back(i);
        }

        const int chinesePunctuation[] = {
            0x3002, // 。
            0xFF0C, // ，
            0x3001, // 、
            0xFF1A, // ：
            0xFF1B, // ；
            0xFF1F, // ？
            0xFF01, // ！
            0x201C, // "
            0x201D, // "
            0x2018, // '
            0x2019, // '
            0x3010, // 【
            0x3011, // 】
            0xFF08, // （
            0xFF09, // ）
            0x2014, // —
            0x2026, // …
        };
        for (int cp : chinesePunctuation) {
            codepoints.push_back(cp);
        }

        // 加载常用汉字
        for (int i = 0x4E00; i <= 0x9FFF; i++) {
            codepoints.push_back(i);
        }

        return LoadFontEx(fontPath, fontSize, codepoints.data(), static_cast<int>(codepoints.size()));
    }

    void UnloadFont(Font font) {
        if (font.implementationDefined) {
            auto* rlFont = static_cast<::Font*>(font.implementationDefined);
            ::UnloadFont(*rlFont);
            delete rlFont;
        }
    }

    void DrawTextEx(Font font, const char* text, float x, float y, float fontSize, float spacing, Color color) {
        if (!font.implementationDefined) return;
        auto* rlFont = static_cast<::Font*>(font.implementationDefined);
        ::DrawTextEx(*rlFont, text, {x, y}, fontSize, spacing, RL(color));
    }

    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
        if (!font.implementationDefined) return {0, 0};
        auto* rlFont = static_cast<::Font*>(font.implementationDefined);
        auto size = ::MeasureTextEx(*rlFont, text, fontSize, spacing);
        return {size.x, size.y};
    }

    bool IsFontValid(Font font) {
        if (!font.implementationDefined) return false;
        auto* rlFont = static_cast<::Font*>(font.implementationDefined);
        return rlFont->glyphCount > 0;
    }

}