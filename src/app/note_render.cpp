//
// Created by madoka on 2025/12/15.
//

#include "Madokawaii/app/note_operation.h"
#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/texture.h"

using ResTexture2D =
    Madokawaii::Platform::Graphics::Texture::Texture2D;
struct ResPackDecompressed
{
    ResTexture2D imageClick;
    ResTexture2D imageHold;
    ResTexture2D imageFlick;
    ResTexture2D imageDrag;

    ResTexture2D imageClickMH;
    ResTexture2D imageHoldMH;
    ResTexture2D imageFlickMH;
    ResTexture2D imageDragMH;
};

ResPackDecompressed respack_decompressed{};

void InitializeNoteRenderer(const Madokawaii::App::ResPack::ResPack& respack_raw)
{
    // decompress respack images into textures
    using namespace Madokawaii::Platform::Graphics::Texture;

    auto loadTextureFromResData = [](const Madokawaii::App::ResPack::ResPackData* resData) -> Texture2D
    {
        // load image from memory
        Image img = LoadImageFromMemory(".png",
            static_cast<const unsigned char*>(resData->data),
            static_cast<int>(resData->size));
        // load texture from image
        Texture2D tex = LoadTextureFromImage(img);
        // unload image
        UnLoadImage(img);
        return tex;
    };

    respack_decompressed.imageClick = loadTextureFromResData(respack_raw.imageClick);
    respack_decompressed.imageHold = loadTextureFromResData(respack_raw.imageHold);
    respack_decompressed.imageFlick = loadTextureFromResData(respack_raw.imageFlick);
    respack_decompressed.imageDrag = loadTextureFromResData(respack_raw.imageDrag);

    respack_decompressed.imageClickMH = loadTextureFromResData(respack_raw.imageClickMH);
    respack_decompressed.imageHoldMH = loadTextureFromResData(respack_raw.imageHoldMH);
    respack_decompressed.imageFlickMH = loadTextureFromResData(respack_raw.imageFlickMH);
    respack_decompressed.imageDragMH = loadTextureFromResData(respack_raw.imageDragMH);
}

void RenderNote(const Madokawaii::App::chart::judgeline::note& note)
{
    ResTexture2D texture;
    if  (note.type == Madokawaii::App::NoteType::hold) {
        // hold单独处理

        return;
    }
    switch (note.type)
    {
    case Madokawaii::App::NoteType::tap:
        texture = note.isMultipleNote ? respack_decompressed.imageClickMH : respack_decompressed.imageClick;
        break;
    case Madokawaii::App::NoteType::drag:
        texture = note.isMultipleNote ? respack_decompressed.imageDragMH : respack_decompressed.imageDrag;
        break;
    case Madokawaii::App::NoteType::flick:
        texture = note.isMultipleNote ? respack_decompressed.imageFlickMH : respack_decompressed.imageFlick;
        break;
    default:
        return;
    }
    Madokawaii::Platform::Graphics::Vector2 pos{}, texture_dimension{};
    Madokawaii::Platform::Graphics::Texture::MeasureTexture2D(texture, &texture_dimension);
    Madokawaii::Platform::Graphics::Color_ tint{255, 255, 255, 255};
    const auto rotateAngle = static_cast<float>(360 - note.rotateAngle), rotateAngleRad = rotateAngle * static_cast<float>(M_PI) / 180.f;
    float xOffset = texture_dimension.x / 2.f * 0.2f, yOffset = texture_dimension.y / 2.f * 0.2f;
    pos.x = static_cast<float>(note.coordinateX - cos(rotateAngleRad) * xOffset + sin(rotateAngleRad) * yOffset);
    pos.y = static_cast<float>(note.coordinateY - cos(rotateAngleRad) * yOffset - sin(rotateAngleRad) * xOffset);
    Madokawaii::Platform::Graphics::Texture::DrawTextureEx(texture, pos, rotateAngle, 0.2f, tint);
}
