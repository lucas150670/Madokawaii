//
// Created by madoka on 2025/9/19.
//
#include <raylib.h>
#include "Madokawaii/platform/audio.h"

namespace Madokawaii::Platform::Audio {

    static ::Music& AsRL(Music& m) { return *static_cast<::Music*>(m.implementationDefined); }
    static const ::Music& AsRL(const Music& m) { return *static_cast<const ::Music*>(m.implementationDefined); }

    void InitAudioDevice() { ::InitAudioDevice(); }

    void CloseAudioDevice() { ::CloseAudioDevice(); }

    Music LoadMusicStream(const char* path) {
        Music m{};
        auto rl = ::LoadMusicStream(path);
        m.looping = true;
        m.implementationDefined = new ::Music(rl);
        return m;
    }

    Music LoadMusicStreamFromMemory(const char *fileType, const unsigned char *data, int dataSize)
    {
        Music m{};
        auto rl = ::LoadMusicStreamFromMemory(fileType, data, dataSize);
        m.looping = true;
        m.implementationDefined = new ::Music(rl);
        return m;
    }

    void UnloadMusicStream(Music m) {
        if (m.implementationDefined) {
            auto rl = AsRL(m);
            ::UnloadMusicStream(rl);
            // clear up did by raylib. no need delete
        }
    }

    void PlayMusicStream(Music m) { AsRL(m).looping = m.looping; ::PlayMusicStream(AsRL(m)); }

    void UpdateMusicStream(Music m) { ::UpdateMusicStream(AsRL(m)); }

    void StopMusicStream(Music m) { ::StopMusicStream(AsRL(m)); }

    void PauseMusicStream(Music m) { ::PauseMusicStream(AsRL(m)); }

    void ResumeMusicStream(Music m) { ::ResumeMusicStream(AsRL(m)); }

    bool IsMusicStreamPlaying(Music m) { return ::IsMusicStreamPlaying(AsRL(m)); }

    float GetMusicTimeLength(Music m) { return ::GetMusicTimeLength(AsRL(m)); }

    float GetMusicTimePlayed(Music m) { return ::GetMusicTimePlayed(AsRL(m)); }

    void SetMusicPitch(Music m, float pitch) { ::SetMusicPitch(AsRL(m), pitch); }
}