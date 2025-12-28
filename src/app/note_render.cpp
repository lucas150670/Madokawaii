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
    ResTexture2D imageClick{};
    ResTexture2D imageHold{};
    ResTexture2D imageFlick{};
    ResTexture2D imageDrag{};

    ResTexture2D imageClickMH{};
    ResTexture2D imageHoldMH{};
    ResTexture2D imageFlickMH{};
    ResTexture2D imageDragMH{};
    float holdAtlasHead{}, holdAtlasTail{}, holdAtlasMHHead{}, holdAtlasMHTail{};
};

std::vector<Madokawaii::App::chart::judgeline::note> holds_to_render{};
float screenWidth{}, screenHeight{};

ResPackDecompressed respack_decompressed{};

void InitializeNoteRenderer(const Madokawaii::App::ResPack::ResPack& respack_raw, float screenWidthIn, float screenHeightIn)
{
    screenWidth = screenWidthIn;
    screenHeight = screenHeightIn;
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
    respack_decompressed.holdAtlasHead = respack_raw.holdAtlasHead;
    respack_decompressed.holdAtlasTail = respack_raw.holdAtlasTail;
    respack_decompressed.holdAtlasMHHead = respack_raw.holdAtlasMHHead;
    respack_decompressed.holdAtlasMHTail = respack_raw.holdAtlasMHTail;
}

void RenderHoldNote(const Madokawaii::App::chart::judgeline::note& note)
{
    auto noteHoldLength = note.speed * note.realHoldTime * screenHeight * 0.6;
    auto texture = note.isMultipleNote ? respack_decompressed.imageHoldMH : respack_decompressed.imageHold;
    // 在这里的逻辑里，head在尾部绘制，tail在头部绘制
    auto holdAtlasTail = note.isMultipleNote ? respack_decompressed.holdAtlasMHHead : respack_decompressed.holdAtlasHead;
    auto holdAtlasHead = note.isMultipleNote ? respack_decompressed.holdAtlasMHTail : respack_decompressed.holdAtlasTail;

    Madokawaii::Platform::Graphics::Vector2 texture_dimension{};
    Madokawaii::Platform::Graphics::Texture::MeasureTexture2D(texture, &texture_dimension);
    Madokawaii::Platform::Graphics::Color_ tint{ 255, 255, 255, 255 };

    const auto rotateAngle = static_cast<float>(360 - note.rotateAngle);
    constexpr float scale = 0.2f;

    float holdBodyTextureHeight = texture_dimension.y - holdAtlasHead - holdAtlasTail;
    auto bodyHeight = static_cast<float>(noteHoldLength);

    if (note.isNoteBelow)
    Madokawaii::Platform::Graphics::SetTransform(
        note.coordinateX, note.coordinateY, rotateAngle + 180.0f, scale, scale);
    else
        Madokawaii::Platform::Graphics::SetTransform(
            note.coordinateX, note.coordinateY,  rotateAngle, scale, scale);

    Madokawaii::Platform::Shape::Rectangle srcHead{};
    srcHead.x = 0;
    srcHead.y = texture_dimension.y - holdAtlasHead;
    srcHead.width = texture_dimension.x;
    srcHead.height = holdAtlasHead;

    if (note.state != Madokawaii::App::NoteState::holding)
    {
        Madokawaii::Platform::Graphics::Texture::DrawTextureRec(
            texture, srcHead,
            { -srcHead.width / 2.f, -srcHead.height / 2.f },
            tint);
    } else
    {
        holdAtlasHead = 0;
    }

    if (bodyHeight > 0) {
        Madokawaii::Platform::Shape::Rectangle srcBody{};
        srcBody.x = 0;
        srcBody.y = holdAtlasTail;
        srcBody.width = texture_dimension.x;
        srcBody.height = holdBodyTextureHeight;

        Madokawaii::Platform::Shape::Rectangle destBody{};
        destBody.width = texture_dimension.x;
        destBody.height = bodyHeight / scale;

        float bodyStartY = -holdAtlasHead / 2.f - destBody.height;

        destBody.x = -destBody.width / 2.f;
        destBody.y = bodyStartY;

        Madokawaii::Platform::Graphics::Texture::DrawTexturePro(
            texture, srcBody, destBody,
            { 0, 0 }, // controls by destBody
            0, tint);

        Madokawaii::Platform::Shape::Rectangle srcTail{};
        srcTail.x = 0;
        srcTail.y = 0;
        srcTail.width = texture_dimension.x;
        srcTail.height = holdAtlasTail;

        float tailOffsetY = -holdAtlasHead / 2.f - destBody.height - holdAtlasTail;

        Madokawaii::Platform::Graphics::Texture::DrawTextureRec(
            texture, srcTail,
            { -srcTail.width / 2.f, tailOffsetY },
            tint);
    }

    // 恢复变换矩阵
    Madokawaii::Platform::Graphics::PopTransform();
}

void RenderNote(const Madokawaii::App::chart::judgeline::note& note)
{
    ResTexture2D texture;
    if (note.type == Madokawaii::App::NoteType::hold) {
        RenderHoldNote(note);
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

void AddHoldNoteClickingRender(const Madokawaii::App::chart::judgeline::note &note) {
    holds_to_render.push_back(note);
}

void RenderHoldCallback(float thisFrameTime, const Madokawaii::App::chart& thisChart) {
    for (auto &hold : holds_to_render) {
		if (hold.realHoldTime + hold.realTime < thisFrameTime) {
		    hold.state = Madokawaii::App::NoteState::finished;
            continue;
		}
        hold.state = Madokawaii::App::NoteState::holding;
        hold.realHoldTime -= (thisFrameTime - hold.realTime);
        hold.realTime = thisFrameTime;

        auto& judgeline = thisChart.judgelines[hold.parent_line_id];
        hold.rotateAngle = judgeline.info.rotateAngle;
        // realholdtime = speed * (holdTime -> real)
        if (fabs(hold.rotateAngle - 360.0) < 1e-6)
            hold.rotateAngle = 0.0f;
        auto note_rotate_angle_rad =  hold.rotateAngle * M_PI / 180.0;
        const double judgelineScreenX = judgeline.info.posX * screenWidth;
        const double judgelineScreenY = judgeline.info.posY * screenHeight;
        const double posX = hold.positionX * screenWidth * 0.05625;

        const double diffX = cos(note_rotate_angle_rad) * posX;
        const double diffY = sin(note_rotate_angle_rad) * posX;

        hold.coordinateX = judgelineScreenX + diffX;
        const double centralY = judgelineScreenY + diffY;
        hold.coordinateY = screenHeight - centralY;
        // Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "NOTE: Rendering Holding Note, this frame time = %f, hold time = %f, real hold time = %f", thisFrameTime, hold.realTime, hold.realHoldTime);
        RenderHoldNote(hold);
    }
    std::erase_if(holds_to_render, [](const auto& note) { return note.state == Madokawaii::App::NoteState::finished; });
}

void UnloadNoteRenderer()
{
    auto unloadTexture = [](ResTexture2D& texture) { Madokawaii::Platform::Graphics::Texture::UnloadTexture(texture); };
    unloadTexture(respack_decompressed.imageClick);
    unloadTexture(respack_decompressed.imageHold);
    unloadTexture(respack_decompressed.imageFlick);
    unloadTexture(respack_decompressed.imageDrag);
    unloadTexture(respack_decompressed.imageClickMH);
    unloadTexture(respack_decompressed.imageHoldMH);
    unloadTexture(respack_decompressed.imageFlickMH);
    unloadTexture(respack_decompressed.imageDragMH);
    respack_decompressed = {};
}
