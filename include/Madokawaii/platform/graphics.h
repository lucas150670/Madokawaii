//
// Created by madoka on 25-9-11.
//

#ifndef MADOKAWAII_GRAPHICS_H
#define MADOKAWAII_GRAPHICS_H
#pragma warning (disable: 4576)

#include <string>

namespace Madokawaii::Platform::Graphics {
    typedef struct Color_ {
        unsigned char r, g, b, a;
    } Color;
    struct Vector2 {
        float x,y;
    };
    std::string GetImplementer();
    std::string GetImplementationInfo();
    float GetFPS();
    float GetFrameTime();
    void SetTargetFPS(int);

    void BeginDrawing();
    void EndDrawing();
    void ClearBackground(Color);
    void DrawText(const char*, int, int, int, Color);
    void DrawLineEx(Vector2, Vector2, float, Color);
	int MeasureText(const char* text, int fontSize);
    void DrawRectangle(int posX, int posY, int width, int height, Color Color);
    void SetTransform(float x, float y, float rotate, float scaleX, float scaleY);
    void PopTransform();


    constexpr Color M_LIGHTGRAY = {200, 200, 200, 255}; //浅灰色
    constexpr Color M_GRAY = {130, 130, 130, 255}; //灰色
    constexpr Color M_DARKGRAY = {80, 80, 80, 255}; //深灰色
    constexpr Color M_YELLOW = {253, 249, 0, 255}; //黄色
    constexpr Color M_GOLD = {255, 203, 0, 255}; //金色
    constexpr Color M_ORANGE = {255, 161, 0, 255}; //橙色
    constexpr Color M_PINK = {255, 109, 194, 255}; //粉色
    constexpr Color M_RED = {230, 41, 55, 255}; //红色
    constexpr Color M_MAROON = {190, 33, 55, 255}; //褐红色
    constexpr Color M_GREEN = {0, 228, 48, 255}; //绿色
    constexpr Color M_LIME = {0, 158, 47, 255}; //石灰
    constexpr Color M_DARKGREEN = {0, 117, 44, 255}; //深绿色
    constexpr Color M_SKYBLUE = {102, 191, 255, 255}; //天空蓝
    constexpr Color M_BLUE = {0, 121, 241, 255}; //蓝色
    constexpr Color M_DARKBLUE = {0, 82, 172, 255}; //深蓝色
    constexpr Color M_PURPLE = {200, 122, 255, 255}; //紫色
    constexpr Color M_VIOLET = {135, 60, 190, 255}; //紫罗兰色
    constexpr Color M_DARKPURPLE = {112, 31, 126, 255}; //深紫色
    constexpr Color M_BEIGE = {211, 176, 131, 255}; //米色
    constexpr Color M_BROWN = {127, 106, 79, 255}; //棕色
    constexpr Color M_DARKBROWN = {76, 63, 47, 255}; //深棕色

    constexpr Color M_WHITE = {255, 255, 255}; //白色
    constexpr Color M_BLACK = {0, 0, 0, 255}; //黑色
    constexpr Color M_BLANK = {0, 0, 0, 0}; //透明
    constexpr Color M_MAGENTA = {255, 0, 255, 255}; //洋红色
    constexpr Color M_RAYWHITE = {245, 245, 245, 255}; //射线白, 作者送的颜色
}

#endif //MADOKAWAII_GRAPHICS_H
