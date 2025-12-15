//
// Created by madoka on 2025/12/14.
//

#ifndef MADOKAWAII_RES_PACK_H
#define MADOKAWAII_RES_PACK_H
#include <memory>
#include <string>

namespace Madokawaii::App::ResPack
{
    struct ResPackData
    {
        void* data;
        uint64_t size;
    };

    struct ResPack
    {
        std::string name, author;
        float holdAtlasHead{}, holdAtlasTail{}, holdAtlasMHHead{}, holdAtlasMHTail{};
        ResPackData* imageClick = nullptr, * imageHold = nullptr, * imageFlick = nullptr, * imageDrag = nullptr;
        ResPackData* imageClickMH = nullptr, * imageHoldMH = nullptr, * imageFlickMH = nullptr, * imageDragMH = nullptr;
        ResPackData* soundClick = nullptr, * soundFlick = nullptr, * soundDrag = nullptr;
        ResPackData* musicEnding = nullptr;
        int hitFxCount{}, hitFxWidth{}, hitFxHeight{};
        ResPackData* imageHitFx = nullptr;
    };

    std::shared_ptr<ResPack> LoadResPackFromMemoryStream(const unsigned char* data, size_t size);

    void ParseResPackInfo(const std::string& input, ResPack* out_data);

}

#endif //MADOKAWAII_RES_PACK_H