//
// Created by madoka on 25-9-16.
//
//
// Created by madoka on 25-9-16.
//
#include <raylib.h>
#include <glad/glad.h>
#include "Madokawaii/platform/graphics.h"

namespace Madokawaii::Platform::Graphics {

    static ::Color RL(Color_ c) { return {c.r, c.g, c.b, c.a}; }

    std::string GetImplementer() { return reinterpret_cast<const char*>(glGetString(GL_RENDERER)); }

    std::string GetImplementationInfo() { return reinterpret_cast<const char*>(glGetString(GL_VERSION)); }

    float GetFPS() { return static_cast<float>(::GetFPS()); }

    float GetFrameTime() { return ::GetFrameTime(); }

    void BeginDrawing() { ::BeginDrawing(); }

    void EndDrawing() { ::EndDrawing(); }

    void ClearBackground(Color_ c) { ::ClearBackground(RL(c)); }

    void DrawText(const char* text, int x, int y, int fontSize, Color_ color) {
        ::DrawText(text, x, y, fontSize, RL(color));
    }

    void DrawLineEx(Vector2 start, Vector2 end, float thick, Color_ color) {
        ::DrawLineEx({start.x, start.y}, {end.x, end.y}, thick, RL(color));
    }
}

