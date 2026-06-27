//
// Created by madoka on 2025/12/29.
//
#include <cmath>
#include <vector>

#include "Madokawaii/app/common.hpp"
#include "Madokawaii/app/def.hpp"
#include "Madokawaii/app/coordinate.hpp"
#include "Madokawaii/app/note_hit.hpp"
#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/platform/log.hpp"
#include "Madokawaii/platform/texture.hpp"

namespace Madokawaii::App::NoteHit {

// pair<1>: destination distance
// pair<2>: destination direction
namespace {
std::pair<float, float> ParticleEffects_Destination_Gen(NoteHitFxState& state)
{
    const float rand1 = state.unitDistribution(state.randomEngine);
    const float rand2 = state.unitDistribution(state.randomEngine);
    return {rand1 * 80 + 185, static_cast<float>(rand2 * 2 * M_PI)};
}
}

int InitializeFxManager(AppContext& context, ResPack::ResPack & pack, Madokawaii::Platform::Graphics::Color perfectColor) {
    auto& fx = context.noteHitFx.resources;
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
    fx.spriteUnitWidth = dimension.x / pack.hitFxWidth;
    fx.spriteUnitHeight = dimension.y / pack.hitFxHeight;
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Sliced sprite unit width, height = %f, %f",
        fx.spriteUnitWidth,
        fx.spriteUnitHeight);
    for (int i = 0; i < pack.hitFxHeight; i++) {
        for (int j = 0; j < pack.hitFxWidth; j++) {
            float sprite_start_x = fx.spriteUnitWidth * j;
            float sprite_start_y = fx.spriteUnitHeight * i;
            Madokawaii::Platform::Shape::Rectangle src_rect{};
            src_rect.x = sprite_start_x;
            src_rect.y = sprite_start_y;
            src_rect.width = fx.spriteUnitWidth;
            src_rect.height = fx.spriteUnitHeight;
            Madokawaii::Platform::Graphics::Texture::Image croped_image = Madokawaii::Platform::Graphics::Texture::ImageCopy(image);
            Madokawaii::Platform::Graphics::Texture::ImageCrop(croped_image, src_rect);
            fx.sprites.emplace_back(Madokawaii::Platform::Graphics::Texture::LoadTextureFromImage(croped_image));
            Madokawaii::Platform::Graphics::Texture::UnloadImage(croped_image);
        }
    }
    Madokawaii::Platform::Graphics::Texture::UnloadImage(image);
    fx.perfectColor = perfectColor;
    fx.spriteFrameRate = 2 * static_cast<int>(fx.sprites.size());
    fx.spriteFrameTime = 1.0f / fx.spriteFrameRate;
    return 0;
}

void RegisterFx(AppContext& context, float this_frame_time, float position_X, float position_Y) {
    NoteHitFxInfo info{};
    info.posX = position_X;
    info.posY = position_Y;
    info.startTime = this_frame_time;
    info.isDiscarded = false;
    for (int i = 0; i < 4; i++)
    {
        auto genResult = ParticleEffects_Destination_Gen(context.noteHitFx);
        info.destination[i] = genResult.first;
        info.direction[i] = genResult.second;
    }
//    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Register hit fx at (%f, %f)", position_X, position_Y);
    context.noteHitFx.activeEffects.push_back(info);
}

void UpdateFx(AppContext& context, float this_frameTime) {
    auto& state = context.noteHitFx;
    auto& fx = state.resources;
    const float screen_X = static_cast<float>(context.display.screenWidth);
    const float screen_Y = static_cast<float>(context.display.screenHeight);
    const int spriteCount = static_cast<int>(fx.sprites.size());
    if (spriteCount == 0) return;
    const auto viewport = Madokawaii::App::Coordinate::MakeScreenViewport(screen_X, screen_Y);

    auto hitFx_size = state.activeEffects.size();
    for (auto& hitFx: state.activeEffects) {
        bool draw_this_hitFx = true; // indicating this hitFx will be drawn
        float elapsed_time = this_frameTime - hitFx.startTime;
        int frame_index = std::floor(elapsed_time / fx.spriteFrameTime);
        if (frame_index >= spriteCount)
        {
            hitFx.isDiscarded = true;
            continue;
        }
        if (frame_index < 0) frame_index = 0;

        if (hitFx_size > 2000)
        {
            // 用一点障眼法，确保屏幕上只出现2000个（左右）打击特效避免卡顿
            auto display_proportion = 2000.0 / hitFx_size;
            int random_number = state.displayDistribution(state.randomEngine);
            if (random_number / 10000.0 > display_proportion)
                draw_this_hitFx = false;
        }

        float scale_Ratio = screen_Y / 1080.0f;
        const auto screenPosition = Madokawaii::App::Coordinate::ToScreenPoint({hitFx.posX, hitFx.posY}, viewport);
        float draw_pos_x = screenPosition.x - fx.spriteUnitWidth * scale_Ratio / 2.f;
        float draw_pos_y = screenPosition.y - fx.spriteUnitHeight * scale_Ratio / 2.f;
        if (draw_this_hitFx)
        {
            Madokawaii::Platform::Graphics::Texture::DrawTextureEx(fx.sprites[frame_index], {draw_pos_x, draw_pos_y}, 0.f, scale_Ratio, fx.perfectColor);
        }
        // draw particle effects

        float tick = elapsed_time / fx.spriteFrameTime / static_cast<float>(fx.sprites.size());
        float particle_size = 30.f * (((0.2078f * tick - 1.6524f) * tick + 1.6399f) * tick + 0.4988f) * scale_Ratio;
        for (int i = 0; i < 4; i++) {
            float nowDirection_distance = hitFx.destination[i] * (9 * tick / (8 * tick + 1)) * scale_Ratio;
            float nowDirection_angleRad = hitFx.direction[i];
            float nowDirection_x = screenPosition.x + nowDirection_distance * cos(nowDirection_angleRad);
            float nowDirection_y = screenPosition.y + nowDirection_distance * sin(nowDirection_angleRad);
            // Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "HITFX: Particle effect at (%f, %f), size = %f, distance = %f, angle = %f",
            //     nowDirection_x, nowDirection_y, particle_size, nowDirection_distance, nowDirection_angleRad);
            const auto alpha_channel = static_cast<unsigned char>(226 * (1 - tick));
            if (draw_this_hitFx)
                Madokawaii::Platform::Graphics::DrawRectangle(
                    static_cast<int>(nowDirection_x),
                    static_cast<int>(nowDirection_y),
                    static_cast<int>(particle_size),
                    static_cast<int>(particle_size),
                    {fx.perfectColor.r, fx.perfectColor.g, fx.perfectColor.b, alpha_channel});
        }
    }

    std::erase_if(state.activeEffects, [](const NoteHitFxInfo& hitFx) { return hitFx.isDiscarded; });
}

void UnloadFxManager(AppContext& context) {
    auto& state = context.noteHitFx;
    state.activeEffects.clear();
    for (auto& sprite : state.resources.sprites) {
        Madokawaii::Platform::Graphics::Texture::UnloadTexture(sprite);
    }
    state = {};
}

} // namespace Madokawaii::App::NoteHit
