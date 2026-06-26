//
// Direct2D backend texture and image implementation.
//

#include "direct2d_platform.hpp"

#include "Madokawaii/platform/texture.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <d2d1helper.h>

namespace Madokawaii::Platform::Graphics::Texture {
    using Direct2D::ImageData;
    using Direct2D::TextureData;

    namespace {
        constexpr Color WhiteTint{255, 255, 255, 255};

        std::uint8_t ClampByte(int value) {
            return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
        }

        Image Wrap(ImageData* data) {
            return Image{data};
        }

        Texture2D Wrap(TextureData* data) {
            return Texture2D{data};
        }

        ImageData* AsImageData(Image image) {
            return static_cast<ImageData*>(image.implementationDefinedData);
        }

        TextureData* AsTextureData(Texture2D texture) {
            return static_cast<TextureData*>(texture.implementationDefinedData);
        }

        ImageData* DecodeImage(IWICBitmapDecoder* decoder) {
            if (!decoder) return nullptr;

            auto* wicFactory = Direct2D::WicFactory();
            if (!wicFactory) return nullptr;

            IWICBitmapFrameDecode* frame{};
            IWICFormatConverter* converter{};
            UINT width = 0;
            UINT height = 0;

            if (FAILED(decoder->GetFrame(0, &frame))
                || FAILED(frame->GetSize(&width, &height))
                || FAILED(wicFactory->CreateFormatConverter(&converter))
                || FAILED(converter->Initialize(
                    frame,
                    GUID_WICPixelFormat32bppBGRA,
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0,
                    WICBitmapPaletteTypeCustom))) {
                Direct2D::SafeRelease(converter);
                Direct2D::SafeRelease(frame);
                return nullptr;
            }

            auto* image = new ImageData;
            image->width = width;
            image->height = height;
            image->pixelsBgra.resize(static_cast<std::size_t>(width) * height * 4);

            const auto stride = width * 4;
            const auto byteCount = stride * height;
            if (FAILED(converter->CopyPixels(nullptr, stride, byteCount, image->pixelsBgra.data()))) {
                delete image;
                image = nullptr;
            }

            Direct2D::SafeRelease(converter);
            Direct2D::SafeRelease(frame);
            return image;
        }

        ImageData* DecodeImageFromFile(const char* fileName) {
            auto* wicFactory = Direct2D::WicFactory();
            if (!wicFactory || !fileName) return nullptr;

            IWICBitmapDecoder* decoder{};
            const auto widePath = Direct2D::Utf8ToWide(fileName);
            if (FAILED(wicFactory->CreateDecoderFromFilename(
                    widePath.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnLoad,
                    &decoder))) {
                return nullptr;
            }

            auto* image = DecodeImage(decoder);
            Direct2D::SafeRelease(decoder);
            return image;
        }

        ImageData* DecodeImageFromMemory(const unsigned char* fileData, int dataSize) {
            auto* wicFactory = Direct2D::WicFactory();
            if (!wicFactory || !fileData || dataSize <= 0) return nullptr;

            IWICStream* stream{};
            IWICBitmapDecoder* decoder{};
            if (FAILED(wicFactory->CreateStream(&stream))
                || FAILED(stream->InitializeFromMemory(
                    const_cast<BYTE*>(reinterpret_cast<const BYTE*>(fileData)),
                    static_cast<DWORD>(dataSize)))
                || FAILED(wicFactory->CreateDecoderFromStream(
                    stream,
                    nullptr,
                    WICDecodeMetadataCacheOnLoad,
                    &decoder))) {
                Direct2D::SafeRelease(decoder);
                Direct2D::SafeRelease(stream);
                return nullptr;
            }

            auto* image = DecodeImage(decoder);
            Direct2D::SafeRelease(decoder);
            Direct2D::SafeRelease(stream);
            return image;
        }

        ID2D1Bitmap* GetBitmapForTint(TextureData* texture, Color tint) {
            if (!texture) return nullptr;

            const auto key = Direct2D::ColorKey(tint);
            if (key == Direct2D::ColorKey(WhiteTint)) {
                if (!texture->bitmap) {
                    Direct2D::CreateBitmapFromPixels(
                        texture->pixelsBgra.data(),
                        texture->width,
                        texture->height,
                        WhiteTint,
                        &texture->bitmap);
                }
                return texture->bitmap;
            }

            if (const auto found = texture->tintedBitmaps.find(key); found != texture->tintedBitmaps.end()) {
                return found->second;
            }

            ID2D1Bitmap* tintedBitmap{};
            if (SUCCEEDED(Direct2D::CreateBitmapFromPixels(
                    texture->pixelsBgra.data(),
                    texture->width,
                    texture->height,
                    tint,
                    &tintedBitmap))) {
                texture->tintedBitmaps[key] = tintedBitmap;
            }
            return tintedBitmap;
        }

        D2D1_RECT_F SourceRect(Shape::Rectangle source) {
            auto left = source.x;
            auto top = source.y;
            auto right = source.x + source.width;
            auto bottom = source.y + source.height;
            if (right < left) std::swap(left, right);
            if (bottom < top) std::swap(top, bottom);
            return D2D1::RectF(left, top, right, bottom);
        }

        void DrawTextureInternal(
            TextureData* texture,
            Shape::Rectangle source,
            Shape::Rectangle dest,
            Color tint,
            D2D1_BITMAP_INTERPOLATION_MODE interpolation) {
            auto* renderTarget = Direct2D::RenderTarget();
            auto* bitmap = GetBitmapForTint(texture, tint);
            if (!renderTarget || !bitmap) return;

            renderTarget->DrawBitmap(
                bitmap,
                D2D1::RectF(dest.x, dest.y, dest.x + dest.width, dest.y + dest.height),
                1.0f,
                interpolation,
                SourceRect(source));
        }
    }

    Image LoadImage(const char* fileName) {
        return Wrap(DecodeImageFromFile(fileName));
    }

    void UnloadImage(Image image) {
        delete AsImageData(image);
    }

    Image LoadImageFromMemory(const char*, const unsigned char* fileData, int dataSize) {
        return Wrap(DecodeImageFromMemory(fileData, dataSize));
    }

    Texture2D LoadTexture(const char* fileName) {
        auto* image = DecodeImageFromFile(fileName);
        if (!image) return {};

        auto* texture = new TextureData;
        texture->width = image->width;
        texture->height = image->height;
        texture->pixelsBgra = std::move(image->pixelsBgra);
        Direct2D::CreateBitmapFromPixels(
            texture->pixelsBgra.data(),
            texture->width,
            texture->height,
            WhiteTint,
            &texture->bitmap);
        delete image;
        return Wrap(texture);
    }

    Texture2D LoadTextureFromImage(Image image) {
        auto* source = AsImageData(image);
        if (!source) return {};

        auto* texture = new TextureData;
        texture->width = source->width;
        texture->height = source->height;
        texture->pixelsBgra = source->pixelsBgra;
        Direct2D::CreateBitmapFromPixels(
            texture->pixelsBgra.data(),
            texture->width,
            texture->height,
            WhiteTint,
            &texture->bitmap);
        return Wrap(texture);
    }

    void UnloadTexture(Texture2D texture) {
        delete AsTextureData(texture);
    }

    void DrawTexture(Texture2D texture, Vector2 position, Color_ tint) {
        auto* data = AsTextureData(texture);
        if (!data) return;
        DrawTextureInternal(
            data,
            Shape::Rectangle{0.0f, 0.0f, static_cast<float>(data->width), static_cast<float>(data->height)},
            Shape::Rectangle{position.x, position.y, static_cast<float>(data->width), static_cast<float>(data->height)},
            tint,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    void DrawTextureEx(Texture2D texture, Vector2 pos, float rotation, float scale, Color_ tint) {
        auto* data = AsTextureData(texture);
        auto* renderTarget = Direct2D::RenderTarget();
        if (!data || !renderTarget) return;

        auto& state = Direct2D::GetState();
        const auto previousTransform = state.currentTransform;
        const auto localTransform =
            D2D1::Matrix3x2F::Scale(scale, scale)
            * D2D1::Matrix3x2F::Rotation(rotation)
            * D2D1::Matrix3x2F::Translation(pos.x, pos.y);

        state.currentTransform = state.currentTransform * localTransform;
        renderTarget->SetTransform(state.currentTransform);
        DrawTextureInternal(
            data,
            Shape::Rectangle{0.0f, 0.0f, static_cast<float>(data->width), static_cast<float>(data->height)},
            Shape::Rectangle{0.0f, 0.0f, static_cast<float>(data->width), static_cast<float>(data->height)},
            tint,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        state.currentTransform = previousTransform;
        renderTarget->SetTransform(state.currentTransform);
    }

    void DrawTextureRec(Texture2D texture, Shape::Rectangle source, Vector2 position, Color_ tint) {
        auto* data = AsTextureData(texture);
        if (!data) return;
        DrawTextureInternal(
            data,
            source,
            Shape::Rectangle{position.x, position.y, std::fabs(source.width), std::fabs(source.height)},
            tint,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

    void DrawTexturePro(Texture2D texture, Shape::Rectangle source, Shape::Rectangle dest, Vector2 origin, float rotation, Color tint) {
        auto* data = AsTextureData(texture);
        auto* renderTarget = Direct2D::RenderTarget();
        if (!data || !renderTarget) return;

        auto& state = Direct2D::GetState();
        const auto previousTransform = state.currentTransform;
        if (std::fabs(rotation) > 0.0001f) {
            const auto pivot = D2D1::Point2F(dest.x, dest.y);
            state.currentTransform = state.currentTransform * D2D1::Matrix3x2F::Rotation(rotation, pivot);
            renderTarget->SetTransform(state.currentTransform);
        }

        DrawTextureInternal(
            data,
            source,
            Shape::Rectangle{dest.x - origin.x, dest.y - origin.y, dest.width, dest.height},
            tint,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

        if (std::fabs(rotation) > 0.0001f) {
            state.currentTransform = previousTransform;
            renderTarget->SetTransform(state.currentTransform);
        }
    }

    void MeasureTexture2D(Texture2D texture, Vector2* dimension) {
        if (!dimension) return;
        auto* data = AsTextureData(texture);
        dimension->x = data ? static_cast<float>(data->width) : 0.0f;
        dimension->y = data ? static_cast<float>(data->height) : 0.0f;
    }

    void ImageResizeNN(Image image, int newWidth, int newHeight) {
        auto* data = AsImageData(image);
        if (!data || newWidth <= 0 || newHeight <= 0 || data->width == 0 || data->height == 0) return;

        std::vector<std::uint8_t> resized(static_cast<std::size_t>(newWidth) * newHeight * 4);
        for (int y = 0; y < newHeight; ++y) {
            const auto srcY = std::min<std::uint32_t>(
                data->height - 1,
                static_cast<std::uint32_t>(static_cast<std::uint64_t>(y) * data->height / newHeight));
            for (int x = 0; x < newWidth; ++x) {
                const auto srcX = std::min<std::uint32_t>(
                    data->width - 1,
                    static_cast<std::uint32_t>(static_cast<std::uint64_t>(x) * data->width / newWidth));
                std::memcpy(
                    &resized[(static_cast<std::size_t>(y) * newWidth + x) * 4],
                    &data->pixelsBgra[(static_cast<std::size_t>(srcY) * data->width + srcX) * 4],
                    4);
            }
        }

        data->width = static_cast<std::uint32_t>(newWidth);
        data->height = static_cast<std::uint32_t>(newHeight);
        data->pixelsBgra = std::move(resized);
    }

    void MeasureImage(Image image, Vector2* dimension) {
        if (!dimension) return;
        auto* data = AsImageData(image);
        dimension->x = data ? static_cast<float>(data->width) : 0.0f;
        dimension->y = data ? static_cast<float>(data->height) : 0.0f;
    }

    Image ImageCopy(Image image) {
        auto* data = AsImageData(image);
        if (!data) return {};

        auto* copy = new ImageData;
        copy->width = data->width;
        copy->height = data->height;
        copy->pixelsBgra = data->pixelsBgra;
        return Wrap(copy);
    }

    void ImageCrop(Image image, Shape::Rectangle crop) {
        auto* data = AsImageData(image);
        if (!data || data->width == 0 || data->height == 0) return;

        const auto left = std::clamp(static_cast<int>(std::floor(crop.x)), 0, static_cast<int>(data->width));
        const auto top = std::clamp(static_cast<int>(std::floor(crop.y)), 0, static_cast<int>(data->height));
        const auto right = std::clamp(static_cast<int>(std::ceil(crop.x + crop.width)), left, static_cast<int>(data->width));
        const auto bottom = std::clamp(static_cast<int>(std::ceil(crop.y + crop.height)), top, static_cast<int>(data->height));
        const auto newWidth = right - left;
        const auto newHeight = bottom - top;
        if (newWidth <= 0 || newHeight <= 0) return;

        std::vector<std::uint8_t> cropped(static_cast<std::size_t>(newWidth) * newHeight * 4);
        for (int y = 0; y < newHeight; ++y) {
            std::memcpy(
                &cropped[static_cast<std::size_t>(y) * newWidth * 4],
                &data->pixelsBgra[(static_cast<std::size_t>(top + y) * data->width + left) * 4],
                static_cast<std::size_t>(newWidth) * 4);
        }

        data->width = static_cast<std::uint32_t>(newWidth);
        data->height = static_cast<std::uint32_t>(newHeight);
        data->pixelsBgra = std::move(cropped);
    }

    void ImageColorBrightness(Image image, int brightness) {
        auto* data = AsImageData(image);
        if (!data) return;

        for (std::size_t i = 0; i < data->pixelsBgra.size(); i += 4) {
            data->pixelsBgra[i + 0] = ClampByte(static_cast<int>(data->pixelsBgra[i + 0]) + brightness);
            data->pixelsBgra[i + 1] = ClampByte(static_cast<int>(data->pixelsBgra[i + 1]) + brightness);
            data->pixelsBgra[i + 2] = ClampByte(static_cast<int>(data->pixelsBgra[i + 2]) + brightness);
        }
    }

    void ImageBlurGaussian(Image image, int blurSize) {
        auto* data = AsImageData(image);
        if (!data || blurSize <= 0 || data->width == 0 || data->height == 0) return;

        const auto radius = std::max(1, blurSize);
        auto temp = data->pixelsBgra;
        auto output = data->pixelsBgra;

        const auto sample = [&](const std::vector<std::uint8_t>& source, int x, int y, int channel) -> int {
            x = std::clamp(x, 0, static_cast<int>(data->width) - 1);
            y = std::clamp(y, 0, static_cast<int>(data->height) - 1);
            return source[(static_cast<std::size_t>(y) * data->width + x) * 4 + channel];
        };

        for (std::uint32_t y = 0; y < data->height; ++y) {
            for (std::uint32_t x = 0; x < data->width; ++x) {
                for (int channel = 0; channel < 4; ++channel) {
                    int sum = 0;
                    int count = 0;
                    for (int offset = -radius; offset <= radius; ++offset) {
                        sum += sample(data->pixelsBgra, static_cast<int>(x) + offset, static_cast<int>(y), channel);
                        count++;
                    }
                    temp[(static_cast<std::size_t>(y) * data->width + x) * 4 + channel] = static_cast<std::uint8_t>(sum / count);
                }
            }
        }

        for (std::uint32_t y = 0; y < data->height; ++y) {
            for (std::uint32_t x = 0; x < data->width; ++x) {
                for (int channel = 0; channel < 4; ++channel) {
                    int sum = 0;
                    int count = 0;
                    for (int offset = -radius; offset <= radius; ++offset) {
                        sum += sample(temp, static_cast<int>(x), static_cast<int>(y) + offset, channel);
                        count++;
                    }
                    output[(static_cast<std::size_t>(y) * data->width + x) * 4 + channel] = static_cast<std::uint8_t>(sum / count);
                }
            }
        }

        data->pixelsBgra = std::move(output);
    }

    void ImageColorContrast(Image image, float contrast) {
        auto* data = AsImageData(image);
        if (!data) return;

        auto factor = (100.0f + contrast) / 100.0f;
        factor *= factor;
        for (std::size_t i = 0; i < data->pixelsBgra.size(); i += 4) {
            for (int channel = 0; channel < 3; ++channel) {
                auto value = data->pixelsBgra[i + channel] / 255.0f;
                value = ((value - 0.5f) * factor + 0.5f) * 255.0f;
                data->pixelsBgra[i + channel] = ClampByte(static_cast<int>(std::round(value)));
            }
        }
    }
}
