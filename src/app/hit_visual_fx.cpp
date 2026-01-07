//
// Created by madoka on 2025/12/29.
//
#include <cmath>
#include <random>
#include <vector>

#include "Madokawaii/app/def.h"
#include "Madokawaii/app/note_hit.h"
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/texture.h"

constexpr int HIT_FX_SPRITE_FRAME_RATE = 60;
constexpr float HIT_FX_SPRITE_FRAME_TIME = 1.0f / HIT_FX_SPRITE_FRAME_RATE;

struct Respack_HitFx_Decompressed
{
    std::vector<Madokawaii::Platform::Graphics::Texture::Texture2D> hitFxSprites;
    float sprite_unit_width, sprite_unit_height;
    Madokawaii::Platform::Graphics::Color perfectColor{};
};

Respack_HitFx_Decompressed hit_fx_decompressed{};

struct HitFxInfo
{
    float posX, posY;
    float startTime;
    int spriteIndex;
    bool isDiscarded;
    // for particle effects
    float direction[4]; // 4个例子特效
    float size[4];
    float destination[4];
};
std::vector<HitFxInfo> hitFx_list;

// pair<1>: destination distance
// pair<2>: destination direction
static std::pair<float, float> ParticleEffects_Destination_Gen()
{
    static std::random_device rd;
    static std::default_random_engine generator(rd());
    static std::uniform_real_distribution distribution(0.f, 1.f);
    const float rand1 = distribution(generator);
    const float rand2 = distribution(generator);
    return {rand1 * 80 + 185, static_cast<float>(rand2 * 2 * M_PI)};
}

int InitializeNoteHitFxManager(Madokawaii::App::ResPack::ResPack & pack, Madokawaii::Platform::Graphics::Color perfectColor) {
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
    hit_fx_decompressed.sprite_unit_width = dimension.x / pack.hitFxWidth;
    hit_fx_decompressed.sprite_unit_height = dimension.y / pack.hitFxHeight;
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Sliced sprite unit width, height = %f, %f",
        hit_fx_decompressed.sprite_unit_width,
        hit_fx_decompressed.sprite_unit_height);
    for (int i = 0; i < pack.hitFxHeight; i++) {
        for (int j = 0; j < pack.hitFxWidth; j++) {
            float sprite_start_x = hit_fx_decompressed.sprite_unit_width * j;
            float sprite_start_y = hit_fx_decompressed.sprite_unit_height * i;
            Madokawaii::Platform::Shape::Rectangle src_rect{};
            src_rect.x = sprite_start_x;
            src_rect.y = sprite_start_y;
            src_rect.width = hit_fx_decompressed.sprite_unit_width;
            src_rect.height = hit_fx_decompressed.sprite_unit_height;
            Madokawaii::Platform::Graphics::Texture::Image croped_image = Madokawaii::Platform::Graphics::Texture::ImageCopy(image);
            Madokawaii::Platform::Graphics::Texture::ImageCrop(croped_image, src_rect);
            hit_fx_decompressed.hitFxSprites.emplace_back(Madokawaii::Platform::Graphics::Texture::LoadTextureFromImage(croped_image));
            Madokawaii::Platform::Graphics::Texture::UnloadImage(croped_image);
        }
    }
    hit_fx_decompressed.perfectColor = perfectColor;
    return 0;
}

void RegisterNoteHitFx(float this_frame_time, float position_X, float position_Y) {
    HitFxInfo info{};
    info.posX = position_X;
    info.posY = position_Y;
    info.startTime = this_frame_time;
    info.isDiscarded = false;
    for (int i = 0; i < 4; i++)
    {
        auto genResult = ParticleEffects_Destination_Gen();
        info.destination[i] = genResult.first;
        info.direction[i] = genResult.second;
    }
//    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Register hit fx at (%f, %f)", position_X, position_Y);
    hitFx_list.push_back(info);
}

void UpdateNoteHitFx(float this_frameTime) {
    static const int spriteCount = static_cast<int>(hit_fx_decompressed.hitFxSprites.size());
    for (auto& hitFx: hitFx_list) {
        float elapsed_time = this_frameTime - hitFx.startTime;
        int frame_index = std::floor(elapsed_time / HIT_FX_SPRITE_FRAME_TIME);
        if (frame_index >= spriteCount)
        {
            hitFx.isDiscarded = true;
            continue;
        }
        float draw_pos_x = hitFx.posX - hit_fx_decompressed.sprite_unit_width / 2.f;
        float draw_pos_y = hitFx.posY - hit_fx_decompressed.sprite_unit_height / 2.f;
        Madokawaii::Platform::Graphics::Texture::DrawTextureEx(hit_fx_decompressed.hitFxSprites[frame_index], {draw_pos_x, draw_pos_y}, 0.f, 1.0f, hit_fx_decompressed.perfectColor);
        // draw particle effects

        float tick = elapsed_time / HIT_FX_SPRITE_FRAME_TIME / static_cast<float>(hit_fx_decompressed.hitFxSprites.size());
        float particle_size = 30.f * (((0.2078f * tick - 1.6524f) * tick + 1.6399f) * tick + 0.4988f);
        for (int i = 0; i < 4; i++) {
            float nowDirection_distance = hitFx.destination[i] * (9 * tick / (8 * tick + 1));
            float nowDirection_angleRad = hitFx.direction[i];
            float nowDirection_x = hitFx.posX + nowDirection_distance * cos(nowDirection_angleRad);
            float nowDirection_y = hitFx.posY + nowDirection_distance * sin(nowDirection_angleRad);
            // Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Particle effect at (%f, %f), size = %f, distance = %f, angle = %f",
            //     nowDirection_x, nowDirection_y, particle_size, nowDirection_distance, nowDirection_angleRad);
            const auto alpha_channel = static_cast<unsigned char>(226 * (1 - tick));
            Madokawaii::Platform::Graphics::DrawRectangle(
                static_cast<int>(nowDirection_x),
                static_cast<int>(nowDirection_y),
                static_cast<int>(particle_size),
                static_cast<int>(particle_size),
                {hit_fx_decompressed.perfectColor.r, hit_fx_decompressed.perfectColor.g, hit_fx_decompressed.perfectColor.b, alpha_channel});
        }
    }

    std::erase_if(hitFx_list, [](const HitFxInfo& hitFx) { return hitFx.isDiscarded; });
}

void UnloadNoteHitFxManager() {
    for (auto& sprite : hit_fx_decompressed.hitFxSprites) {
        Madokawaii::Platform::Graphics::Texture::UnloadTexture(sprite);
    }
}
