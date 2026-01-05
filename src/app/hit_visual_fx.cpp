//
// Created by madoka on 2025/12/29.
//
#include <vector>

#include "Madokawaii/app/note_hit.h"
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/texture.h"

struct Respack_HitFx_Decompressed
{
    std::vector<Madokawaii::Platform::Graphics::Texture::Texture2D> hitFxSprites;
};

Respack_HitFx_Decompressed hit_fx_decompressed{};

struct HitFxInfo
{
    float posX, posY;
    int spriteIndex;
};

int InitializeNoteHitFxManager(Madokawaii::App::ResPack::ResPack & pack) {
    pack.hitFxCount = pack.hitFxHeight * pack.hitFxWidth;
    // load hit fx image from stream
    Madokawaii::Platform::Graphics::Texture::Image image =
        Madokawaii::Platform::Graphics::Texture::LoadImageFromMemory(".png",
            static_cast<unsigned char*>(pack.imageHitFx->data),
            static_cast<int>(pack.imageHitFx->size));
    if (!image.implementationDefinedData) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "HITFX: Invalid hit fx image");
        return -1;
    }
    Madokawaii::Platform::Graphics::Vector2 dimension{};
    Madokawaii::Platform::Graphics::Texture::MeasureImage(image, &dimension);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Hit fx image dimension = %f, %f", dimension.x, dimension.y);
    float sprite_unit_width = dimension.x / pack.hitFxWidth;
    float sprite_unit_height = dimension.y / pack.hitFxHeight;
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Sliced sprite unit width, height = %f, %f", sprite_unit_width, sprite_unit_height);
    for (int i = 0; i < pack.hitFxHeight; i++) {
        for (int j = 0; j < pack.hitFxWidth; j++) {
            float sprite_start_x = sprite_unit_width * j;
            float sprite_start_y = sprite_unit_height * i;
            Madokawaii::Platform::Shape::Rectangle src_rect{};
            src_rect.x = sprite_start_x;
            src_rect.y = sprite_start_y;
            src_rect.width = sprite_unit_width;
            src_rect.height = sprite_unit_height;
            Madokawaii::Platform::Graphics::Texture::Image croped_image = Madokawaii::Platform::Graphics::Texture::ImageCopy(image);
            Madokawaii::Platform::Graphics::Texture::ImageCrop(croped_image, src_rect);
            hit_fx_decompressed.hitFxSprites.emplace_back(Madokawaii::Platform::Graphics::Texture::LoadTextureFromImage(croped_image));
            Madokawaii::Platform::Graphics::Texture::UnloadImage(croped_image);
        }
    }
    return 0;
}
