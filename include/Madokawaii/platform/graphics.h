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

    void BeginDrawing();
    void EndDrawing();
    void ClearBackground(Color);
    void DrawText(const char*, int, int, int, Color);
    void DrawLineEx(Vector2, Vector2, float, Color);
    void SetTransform(float x, float y, float rotate, float scaleX, float scaleY);
    void PopTransform();


    constexpr auto M_LIGHTGRAY = (Color){200, 200, 200, 255}; //浅灰色
    constexpr auto M_GRAY = (Color){130, 130, 130, 255}; //灰色
    constexpr auto M_DARKGRAY = (Color){80, 80, 80, 255}; //深灰色
    constexpr auto M_YELLOW = (Color){253, 249, 0, 255}; //黄色
    constexpr auto M_GOLD = (Color){255, 203, 0, 255}; //金色
    constexpr auto M_ORANGE = (Color){255, 161, 0, 255}; //橙色
    constexpr auto M_PINK = (Color){255, 109, 194, 255}; //粉色
    constexpr auto M_RED = (Color){230, 41, 55, 255}; //红色
    constexpr auto M_MAROON = (Color){190, 33, 55, 255}; //褐红色
    constexpr auto M_GREEN = (Color){0, 228, 48, 255}; //绿色
    constexpr auto M_LIME = (Color){0, 158, 47, 255}; //石灰
    constexpr auto M_DARKGREEN = (Color){0, 117, 44, 255}; //深绿色
    constexpr auto M_SKYBLUE = (Color){102, 191, 255, 255}; //天空蓝
    constexpr auto M_BLUE = (Color){0, 121, 241, 255}; //蓝色
    constexpr auto M_DARKBLUE = (Color){0, 82, 172, 255}; //深蓝色
    constexpr auto M_PURPLE = (Color){200, 122, 255, 255}; //紫色
    constexpr auto M_VIOLET = (Color){135, 60, 190, 255}; //紫罗兰色
    constexpr auto M_DARKPURPLE = (Color){112, 31, 126, 255}; //深紫色
    constexpr auto M_BEIGE = (Color){211, 176, 131, 255}; //米色
    constexpr auto M_BROWN = (Color){127, 106, 79, 255}; //棕色
    constexpr auto M_DARKBROWN = (Color){76, 63, 47, 255}; //深棕色

    constexpr auto M_WHITE = (Color){255, 255, 255}; //白色
    constexpr auto M_BLACK = (Color){0, 0, 0, 255}; //黑色
    constexpr auto M_BLANK = (Color){0, 0, 0, 0}; //透明
    constexpr auto M_MAGENTA = (Color){255, 0, 255, 255}; //洋红色
    constexpr auto M_RAYWHITE = (Color){245, 245, 245, 255}; //射线白, 作者送的颜色
}

#endif //MADOKAWAII_GRAPHICS_H
