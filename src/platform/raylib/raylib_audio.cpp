//
// Created by madoka on 2025/9/19.
//
#include <raylib.h>
#include <filesystem>
#include <fstream>
#include <random>

#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"

namespace Madokawaii::Platform::Audio {

    static double lastAudioTime = 0.0;
    static std::chrono::steady_clock::time_point lastSystemTime;

    static ::Music& AsRL(Music& m) { return *static_cast<::Music*>(m.implementationDefined); }
    static const ::Music& AsRL(const Music& m) { return *static_cast<const ::Music*>(m.implementationDefined); }
    static ::Sound& AsRL(Sound& s) { return *static_cast<::Sound*>(s.implementationDefined); }
    static const ::Sound& AsRL(const Sound& s) { return *static_cast<const ::Sound*>(s.implementationDefined); }

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

    double GetMusicTimeLength(Music m) { return ::GetMusicTimeLength(AsRL(m)); }

    double GetMusicTimePlayed(Music m) {
        double audioTime = ::GetMusicTimePlayed(AsRL(m));
        auto now = std::chrono::steady_clock::now();
        if (lastAudioTime == 0.0) { // 第一次调用，初始化
            lastAudioTime = audioTime;
            lastSystemTime = now;
            return audioTime;
        }
        if (fabs(audioTime - lastAudioTime) > 1e-3f) {
            lastAudioTime = audioTime;
            lastSystemTime = now;
            return static_cast<float>(audioTime);
        }
        double delta = std::chrono::duration<double>(now - lastSystemTime).count();
        return lastAudioTime + delta;
    }


    void SetMusicPitch(Music m, float pitch) { ::SetMusicPitch(AsRL(m), pitch); }

    void SetMusicVolume(Music m, float volume) { ::SetMusicVolume(AsRL(m), volume); }

    Sound LoadSound(const char* fileName)
    {
        Sound s{};
        auto rl = ::LoadSound(fileName);
        s.implementationDefined = new ::Sound(rl);
        return s;
    }

    Sound LoadSoundFromMemory(const char *fileType, const unsigned char *data, int dataSize)
    {
        static std::random_device rd;
        static std::default_random_engine e(rd());
        static std::uniform_int_distribution<int> dist(0, 1000000);
        std::filesystem::path path = std::filesystem::temp_directory_path() / std::format("temp_sound_{}{}", dist(e), fileType);
        {
            std::ofstream ofs(path, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(data), dataSize);
        }
        ::Sound snd = ::LoadSound(path.string().c_str());
        Sound s{};
        s.implementationDefined = new ::Sound(snd);
        return s;
    }

    void UnloadSound(Sound s)
    {
        if (s.implementationDefined)
        {
            auto rl = AsRL(s);
            ::UnloadSound(rl);
        }
    }

    void PlaySound(Sound sound) { ::PlaySound(AsRL(sound)); }
    void StopSound(Sound sound) { ::StopSound(AsRL(sound)); }
    void PauseSound(Sound sound) { ::PauseSound(AsRL(sound)); }
    void ResumeSound(Sound sound) { ::ResumeSound(AsRL(sound)); }
    bool IsSoundPlaying(Sound sound) { return ::IsSoundPlaying(AsRL(sound)); }
    bool IsSoundValid(Sound sound) { return sound.implementationDefined != nullptr && ::IsSoundValid(AsRL(sound)); }
    void SetSoundVolume(Sound sound, float volume) { ::SetSoundVolume(AsRL(sound), volume); }
}
