//
// Created by madoka on 25-9-16.
//

#ifndef MADOKAWAII_TEXTURE_H
#define MADOKAWAII_TEXTURE_H
#include "graphics.h"
#include "shape.h"

namespace Madokawaii::Platform::Graphics::Texture {

    struct Image {
        void *implementationDefinedData;
    };
    struct Texture2D {
        void *implementationDefinedData;
    };
    Image LoadImage(const char *fileName);
    void UnLoadImage(Image Image);
    Image LoadImageFromMemory(const char *fileType, const unsigned char*fileData, int dataSize);
    Texture2D LoadTexture(const char *fileName);
    Texture2D LoadTextureFromImage(Image Image);
    void UnloadTexture(Texture2D texture);
    void DrawTextureEx(Texture2D texture, Vector2, float rotation, float scale, Color_ tint);
    void DrawTextureRec(Texture2D texture, Shape::Rectangle source, Vector2 position, Color_ tint);     //绘制由Rectangle 定义的Texture的一部分
    void DrawTexturePro(Texture2D texture, Shape::Rectangle source, Shape::Rectangle, Vector2 position, float rotation, Color tint);
    void MeasureTexture2D(Texture2D texture, Vector2* dimension);

}

#endif //MADOKAWAII_TEXTURE_H
