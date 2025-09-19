//
// Created by madoka on 2025/9/19.
//

//
// Created by madoka on 2025/9/19.
//
#include <raylib.h>
#include "Madokawaii/platform/texture.h"


namespace Madokawaii::Platform::Graphics::Texture {

    static ::Color RL(Color_ c) { return {c.r, c.g, c.b, c.a}; }

    Image LoadImage(const char *fileName) {
        Image img{};
        img.implementationDefinedData = new ::Image(::LoadImage(fileName));
        return img;
    }

    void UnLoadImage(Image image) {
        if (image.implementationDefinedData) {
            auto& rl = *static_cast<::Image*>(image.implementationDefinedData);
            ::UnloadImage(rl);
            delete &rl;
        }
    }

    Texture2D LoadTexture(const char *fileName) {
        Texture2D t{};
        t.implementationDefinedData = new ::Texture2D(::LoadTexture(fileName));
        return t;
    }

    Texture2D LoadTextureFromImage(Image image) {
        Texture2D t{};
        auto& rlImg = *static_cast<::Image*>(image.implementationDefinedData);
        t.implementationDefinedData = new ::Texture2D(::LoadTextureFromImage(rlImg));
        return t;
    }

    void UnloadTexture(Texture2D texture) {
        if (texture.implementationDefinedData) {
            auto& rl = *static_cast<::Texture2D*>(texture.implementationDefinedData);
            ::UnloadTexture(rl);
            delete &rl;
        }
    }

    void DrawTextureEx(Texture2D texture, Madokawaii::Platform::Graphics::Vector2 pos, float rotation, float scale, Madokawaii::Platform::Graphics::Color_ tint) {
        auto& rl = *static_cast<::Texture2D*>(texture.implementationDefinedData);
        ::DrawTextureEx(rl, {pos.x, pos.y}, rotation, scale, RL(tint));
    }
}