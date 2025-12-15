//
// Created by madoka on 2025/12/14.
//
#include <zip.h>
#include <yaml.h>
#include <libzippp/libzippp.h>
#include <stdexcept>
#include "Madokawaii/platform/core.h"
#include "Madokawaii/app/res_pack.h"

#include "Madokawaii/platform/log.h"
#include <cassert>


namespace Madokawaii::App::ResPack
{
    std::shared_ptr<ResPack> LoadResPackFromMemoryStream(
        const unsigned char* data, size_t size)
    {
        zip_error_t error;
        zip_error_init(&error);

        zip_source_t* src = zip_source_buffer_create(data, size, 0, &error);
        if (!src) {
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR, "zip_source_buffer_create failed");
            abort();
        }

        libzippp::ZipArchive* zf = libzippp::ZipArchive::fromSource(src, libzippp::ZipArchive::OpenMode::ReadOnly);
        if (!zf) {
            zip_source_free(src);
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR, "ZipArchive::fromSource failed");
            abort();
        }

        if (!zf->open(libzippp::ZipArchive::ReadOnly)) {
            delete zf;
            zip_source_free(src);
            Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_ERROR, "ZipArchive open failed");
            abort();
        }

        using Platform::Core::operator ""_hash;
        using Platform::Core::hash_compile_time;
        auto* res_pack_ptr = new ResPack;
        ResPack& res_pack = *res_pack_ptr;
        auto loadResource = [](const libzippp::ZipEntry& entry, ResPackData** outData) {
            *outData = new ResPackData{
                nullptr,
                entry.getSize()
            };
            (*outData)->data = malloc(entry.getSize());
            assert((*outData)->data);
            memcpy((*outData)->data, entry.readAsBinary(), entry.getSize());
        };

        for (const auto& entry : zf->getEntries()) {
            switch (hash_compile_time(entry.getName().c_str()))
            {
            case "info.yml"_hash:
                ParseResPackInfo(entry.readAsText(), &res_pack);
                break;
            case "click.png"_hash:
                loadResource(entry, &res_pack.imageClick);
                break;
            case "flick.png"_hash:
                loadResource(entry, &res_pack.imageFlick);
                break;
            case "drag.png"_hash:
                loadResource(entry, &res_pack.imageDrag);
                break;
            case "hold.png"_hash:
                loadResource(entry, &res_pack.imageHold);
                break;
            case "click_mh.png"_hash:
                loadResource(entry, &res_pack.imageClickMH);
                break;
            case "drag_mh.png"_hash:
                loadResource(entry, &res_pack.imageDragMH);
                break;
            case "flick_mh.png"_hash:
                loadResource(entry, &res_pack.imageFlickMH);
                break;
            case "hold_mh.png"_hash:
                loadResource(entry, &res_pack.imageHoldMH);
                break;
            case "click.ogg"_hash:
                loadResource(entry, &res_pack.soundClick);
                break;
            case "flick.ogg"_hash:
                loadResource(entry, &res_pack.soundFlick);
                break;
            case "drag.ogg"_hash:
                loadResource(entry, &res_pack.soundDrag);
                break;
            case "ending.mp3"_hash:
                loadResource(entry, &res_pack.musicEnding);
                break;
            case "hitFx.png"_hash:
                res_pack.hitFxCount = res_pack.hitFxWidth * res_pack.hitFxHeight;
                loadResource(entry, &res_pack.imageHitFx);
                break;
            default:
                break;
            }
        }

        delete zf;
        zip_source_free(src);
        auto ret_respack = std::shared_ptr<ResPack>(res_pack_ptr,
            [](const ResPack* res_pack_delete) noexcept
        {
            auto freeResource = [](const ResPackData* resData)
            {
                if (resData)
                {
                    if (resData->data)
                        free(resData->data);
                    delete resData;
                }
            };
            freeResource(res_pack_delete->imageClick);
            freeResource(res_pack_delete->imageFlick);
            freeResource(res_pack_delete->imageDrag);
            freeResource(res_pack_delete->imageHold);
            freeResource(res_pack_delete->imageClickMH);
            freeResource(res_pack_delete->imageFlickMH);
            freeResource(res_pack_delete->imageDragMH);
            freeResource(res_pack_delete->imageHoldMH);
            freeResource(res_pack_delete->soundClick);
            freeResource(res_pack_delete->soundFlick);
            freeResource(res_pack_delete->soundDrag);
            freeResource(res_pack_delete->musicEnding);
            freeResource(res_pack_delete->imageHitFx);
            delete res_pack_delete;
        });
        Platform::Log::TraceLog(Platform::Log::TraceLogLevel::LOG_INFO, "RESPACK: initialize successful!");
        return ret_respack;
    }


    void ParseResPackInfo(const std::string& input, ResPack* out_data)
    {
        if (!out_data) return;

        yaml_parser_t parser;
        yaml_event_t event;

        if (!yaml_parser_initialize(&parser)) return;

        yaml_parser_set_input_string(&parser,
            reinterpret_cast<const unsigned char*>(input.c_str()),
            input.size());

        std::string currentKey;
        int arrayIndex = 0;
        bool inArray = false;
        int arrayValues[2] = {0, 0};

        bool done = false;
        while (!done)
        {
            if (!yaml_parser_parse(&parser, &event))
            {
                yaml_parser_delete(&parser);
                return;
            }

            switch (event.type)
            {
            case YAML_SCALAR_EVENT:
            {
                std::string value(reinterpret_cast<char*>(event.data.scalar.value));

                if (inArray)
                {
                    if (arrayIndex < 2)
                        arrayValues[arrayIndex++] = std::stoi(value);
                }
                else if (currentKey.empty())
                {
                    currentKey = value;
                }
                else
                {
                    if (currentKey == "name")
                        out_data->name = value;
                    else if (currentKey == "author")
                        out_data->author = value;
                    currentKey.clear();
                }
                break;
            }
            case YAML_SEQUENCE_START_EVENT:
                inArray = true;
                arrayIndex = 0;
                break;

            case YAML_SEQUENCE_END_EVENT:
                if (currentKey == "hitFx")
                {
                    out_data->hitFxWidth = arrayValues[0];
                    out_data->hitFxHeight = arrayValues[1];
                }
                else if (currentKey == "holdAtlas")
                {
                    out_data->holdAtlasHead = static_cast<float>(arrayValues[0]);
                    out_data->holdAtlasTail = static_cast<float>(arrayValues[1]);
                }
                else if (currentKey == "holdAtlasMH")
                {
                    out_data->holdAtlasMHHead = static_cast<float>(arrayValues[0]);
                    out_data->holdAtlasMHTail = static_cast<float>(arrayValues[1]);
                }
                inArray = false;
                currentKey.clear();
                break;

            case YAML_STREAM_END_EVENT:
                done = true;
                break;

            default:
                break;
            }

            yaml_event_delete(&event);
        }

        yaml_parser_delete(&parser);
    }
}
