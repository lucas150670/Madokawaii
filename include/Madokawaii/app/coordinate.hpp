//
// Created by madoka on 2026/6/27.
//

#ifndef MADOKAWAII_COORDINATE_H
#define MADOKAWAII_COORDINATE_H

#include <algorithm>
#include <cmath>

#include "Madokawaii/platform/graphics.hpp"

namespace Madokawaii::App::Coordinate {

constexpr double DEFAULT_VISIBLE_PADDING_PIXELS = 200.0;
constexpr double PI = 3.14159265358979323846;

struct NormalizedPoint {
    double x{};
    double y{};
};

using NormalizedVector = NormalizedPoint;

struct ScreenViewport {
    double width{1.0};
    double height{1.0};

    [[nodiscard]] double aspectRatio() const {
        return width / height;
    }
};

inline ScreenViewport MakeScreenViewport(double width, double height) {
    return {
        std::max(width, 1.0),
        std::max(height, 1.0)
    };
}

inline Madokawaii::Platform::Graphics::Vector2 ToScreenPoint(NormalizedPoint point, ScreenViewport viewport) {
    return {
        static_cast<float>(point.x * viewport.width),
        static_cast<float>((1.0 - point.y) * viewport.height)
    };
}

// Rotate in screen-space proportions, then return the result in normalized coordinates.
inline NormalizedVector RotateNormalizedVector(NormalizedVector vector, double angleDegrees, ScreenViewport viewport) {
    const double angle = angleDegrees * PI / 180.0;
    const double sinAngle = std::sin(angle);
    const double cosAngle = std::cos(angle);
    const double aspectRatio = viewport.aspectRatio();

    return {
        cosAngle * vector.x - sinAngle * vector.y / aspectRatio,
        cosAngle * vector.y + sinAngle * vector.x * aspectRatio
    };
}

inline bool IsPointVisible(NormalizedPoint point, ScreenViewport viewport,
                           double paddingPixels = DEFAULT_VISIBLE_PADDING_PIXELS) {
    const double paddingX = paddingPixels / viewport.width;
    const double paddingY = paddingPixels / viewport.height;

    return !(point.x < -paddingX || point.x >= 1.0 + paddingX ||
             point.y < -paddingY || point.y >= 1.0 + paddingY);
}

}

#endif //MADOKAWAII_COORDINATE_H
