//
// 你，你不会真的要用这个东西吧？
//

#include "gdiplus_platform.hpp"

#include "Madokawaii/platform/texture.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace Madokawaii::Platform::Graphics::Texture {
    using GdiPlusBackend::ImageData;
    using GdiPlusBackend::TextureData;

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

        TextureData* CreateTextureFromImageData(const ImageData* source) {
            if (!source || source->width == 0 || source->height == 0) return nullptr;

            auto* texture = new TextureData;
            texture->width = source->width;
            texture->height = source->height;
            texture->pixelsBgra = source->pixelsBgra;
            texture->bitmap = GdiPlusBackend::CreateBitmapFromPixels(
                texture->pixelsBgra.data(),
                texture->width,
                texture->height,
                WhiteTint);
            if (!texture->bitmap) {
                delete texture;
                return nullptr;
            }
            return texture;
        }

        Gdiplus::Bitmap* GetBitmapForTint(TextureData* texture, Color tint) {
            if (!texture) return nullptr;

            const auto key = GdiPlusBackend::ColorKey(tint);
            if (key == GdiPlusBackend::ColorKey(WhiteTint)) {
                return texture->bitmap.get();
            }

            if (const auto found = texture->tintedBitmaps.find(key); found != texture->tintedBitmaps.end()) {
                return found->second.get();
            }

            auto tintedBitmap = GdiPlusBackend::CreateBitmapFromPixels(
                texture->pixelsBgra.data(),
                texture->width,
                texture->height,
                tint);
            auto* bitmap = tintedBitmap.get();
            if (bitmap) {
                texture->tintedBitmaps.emplace(key, std::move(tintedBitmap));
            }
            return bitmap;
        }

        Gdiplus::RectF SourceRect(Shape::Rectangle source) {
            auto left = source.x;
            auto top = source.y;
            auto right = source.x + source.width;
            auto bottom = source.y + source.height;
            if (right < left) std::swap(left, right);
            if (bottom < top) std::swap(top, bottom);
            return Gdiplus::RectF(left, top, right - left, bottom - top);
        }

        void DrawTextureInternal(
            TextureData* texture,
            Shape::Rectangle source,
            Shape::Rectangle dest,
            Color tint) {
            auto* graphics = GdiPlusBackend::ActiveGraphics();
            auto* bitmap = GetBitmapForTint(texture, tint);
            if (!graphics || !bitmap) return;

            const auto src = SourceRect(source);
            auto& state = GdiPlusBackend::GetState();
            std::array<Gdiplus::PointF, 3> points{
                GdiPlusBackend::ApplyTransform(state.currentTransform, dest.x, dest.y),
                GdiPlusBackend::ApplyTransform(state.currentTransform, dest.x + dest.width, dest.y),
                GdiPlusBackend::ApplyTransform(state.currentTransform, dest.x, dest.y + dest.height)
            };
            graphics->ResetTransform();
            graphics->DrawImage(
                bitmap,
                points.data(),
                static_cast<INT>(points.size()),
                src.X,
                src.Y,
                src.Width,
                src.Height,
                Gdiplus::UnitPixel);
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
        auto* texture = CreateTextureFromImageData(image);
        delete image;
        return Wrap(texture);
    }

    Texture2D LoadTextureFromImage(Image image) {
        return Wrap(CreateTextureFromImageData(AsImageData(image)));
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
            tint);
    }

    void DrawTextureEx(Texture2D texture, Vector2 pos, float rotation, float scale, Color_ tint) {
        auto* data = AsTextureData(texture);
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!data || !graphics) return;

        auto& state = GdiPlusBackend::GetState();
        const auto previousTransform = state.currentTransform;
        state.currentTransform = GdiPlusBackend::ComposeTransform(
            state.currentTransform,
            GdiPlusBackend::TransformState{pos.x, pos.y, rotation, scale, scale});

        DrawTextureInternal(
            data,
            Shape::Rectangle{0.0f, 0.0f, static_cast<float>(data->width), static_cast<float>(data->height)},
            Shape::Rectangle{0.0f, 0.0f, static_cast<float>(data->width), static_cast<float>(data->height)},
            tint);

        state.currentTransform = previousTransform;
        graphics->ResetTransform();
    }

    void DrawTextureRec(Texture2D texture, Shape::Rectangle source, Vector2 position, Color_ tint) {
        auto* data = AsTextureData(texture);
        if (!data) return;
        DrawTextureInternal(
            data,
            source,
            Shape::Rectangle{position.x, position.y, std::fabs(source.width), std::fabs(source.height)},
            tint);
    }

    void DrawTexturePro(Texture2D texture, Shape::Rectangle source, Shape::Rectangle dest, Vector2 origin, float rotation, Color tint) {
        auto* data = AsTextureData(texture);
        auto* graphics = GdiPlusBackend::ActiveGraphics();
        if (!data || !graphics) return;

        auto& state = GdiPlusBackend::GetState();
        const auto previousTransform = state.currentTransform;
        state.currentTransform = GdiPlusBackend::ComposeTransform(
            state.currentTransform,
            GdiPlusBackend::TransformState{dest.x, dest.y, rotation, 1.0f, 1.0f});

        DrawTextureInternal(
            data,
            source,
            Shape::Rectangle{-origin.x, -origin.y, dest.width, dest.height},
            tint);

        state.currentTransform = previousTransform;
        graphics->ResetTransform();
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
