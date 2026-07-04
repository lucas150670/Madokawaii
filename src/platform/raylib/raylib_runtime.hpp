//
// Raylib backend runtime state shared inside the backend.
//

#ifndef MADOKAWAII_RAYLIB_RUNTIME_H
#define MADOKAWAII_RAYLIB_RUNTIME_H

#include <array>
#include <cstdarg>

namespace Madokawaii::Platform::Raylib {
    constexpr int SAMPLE_COUNT = 600;

struct RuntimeInfo {
    std::array<char, 256> vendor{};
    std::array<char, 256> renderer{};
    std::array<char, 256> version{};

    std::array<float, SAMPLE_COUNT> frameTimes = {};
    int frameIndex = 0;
    int frameCount = 0;
};

RuntimeInfo& GetRuntimeInfo();
void LogCallback(int msgType, const char* text, va_list args);

} // namespace Madokawaii::Platform::Raylib

#endif // MADOKAWAII_RAYLIB_RUNTIME_H
