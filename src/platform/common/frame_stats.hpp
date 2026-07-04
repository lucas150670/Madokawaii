//
// Shared frame-time statistics helpers for render backends.
//

#ifndef MADOKAWAII_PLATFORM_COMMON_FRAME_STATS_H
#define MADOKAWAII_PLATFORM_COMMON_FRAME_STATS_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>

namespace Madokawaii::Platform::Common::FrameStats {

    inline constexpr std::size_t SampleCount = 600;

    template <std::size_t N, typename Index, typename Count>
    void RecordFrameTime(
        std::array<float, N>& frameTimes,
        Index& frameIndex,
        Count& frameCount,
        float frameTime) {
        static_assert(N > 0);
        if (frameTime <= 0.0f) return;

        const auto index = static_cast<std::size_t>(frameIndex) % N;
        frameTimes[index] = frameTime;
        frameIndex = static_cast<Index>((index + 1) % N);
        if (static_cast<std::size_t>(frameCount) < N) {
            frameCount = static_cast<Count>(static_cast<std::size_t>(frameCount) + 1);
        }
    }

    template <std::size_t N, typename Count>
    float GetOnePercentLowFPS(const std::array<float, N>& frameTimes, Count frameCount) {
        static_assert(N > 0);
        const auto count = std::min<std::size_t>(static_cast<std::size_t>(frameCount), N);
        if (count == 0) return 0.0f;

        std::array<float, N> sorted{};
        std::copy_n(frameTimes.begin(), count, sorted.begin());
        std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(count), [](float lhs, float rhs) {
            return lhs > rhs;
        });

        const auto lowFrameCount = std::max<std::size_t>(1, (count + 99) / 100);
        const auto total = std::accumulate(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(lowFrameCount), 0.0f);
        const auto averageFrameTime = total / static_cast<float>(lowFrameCount);
        return averageFrameTime > 0.0f ? 1.0f / averageFrameTime : 0.0f;
    }

} // namespace Madokawaii::Platform::FrameStats

#endif // MADOKAWAII_PLATFORM_COMMON_FRAME_STATS_H
