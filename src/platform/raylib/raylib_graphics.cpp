//
// Created by madoka on 25-9-16.
//
//
// Created by madoka on 25-9-16.
//
#include <raylib.h>
#include "Madokawaii/platform/graphics.h"
#include <rlgl.h>

extern char gVendor[256];
extern char gRenderer[256];
extern char gVersion[256];

namespace Madokawaii::Platform::Graphics {

    static ::Color RL(Color_ c) { return {c.r, c.g, c.b, c.a}; }

    std::string GetImplementer() {
        return gRenderer;
    }

    std::string GetImplementationInfo(){
        return gVersion;
    }

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

    void SetTransform(float x, float y, float rotate, float scaleX, float scaleY)
    {
        rlPushMatrix();
        rlTranslatef(x, y, 0);
        rlRotatef(rotate, 0, 0, 1);
        rlScalef(scaleX, scaleY, 1);
    }

    void PopTransform()
    {
        rlPopMatrix();
    }
}

