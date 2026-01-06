//
// Created by madoka on 25-9-16.
//

#ifndef MADOKAWAII_FONTS_H
#define MADOKAWAII_FONTS_H

#include "Madokawaii/platform/graphics.h"

namespace Madokawaii::Platform::Graphics::Fonts {

    // 字体句柄类型
    struct Font {
        void* implementationDefined{nullptr};
    };

    Font LoadFont(const char* fontPath);
    Font LoadFontEx(const char* fontPath, int fontSize, int* codepoints, int codepointCount);
    Font LoadFontWithChinese(const char* fontPath, int fontSize);

    void UnloadFont(Font font);

    void DrawTextEx(Font font, const char* text, float x, float y, float fontSize, float spacing, Color color);

    Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing);

    bool IsFontValid(Font font);

}

#endif //MADOKAWAII_FONTS_H