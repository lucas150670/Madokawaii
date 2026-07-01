//
// Audio implementation based on FMOD Core SDK.
//

#include <filesystem>
#include <random>
#include <string>
#include "fmod.hpp"
#include <unordered_map>

#include <fast_io.h>
#include <fast_io_device.h>

#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/platform/log.hpp"
#include "Madokawaii/platform/core.hpp"

namespace Madokawaii::Platform::Audio {

    static double lastAudioTime = 0.0;
    static std::chrono::steady_clock::time_point lastSystemTime;

    FMOD_RESULT F_CALL FModDebugCallback(
        FMOD_DEBUG_FLAGS flags,
        const char *file,
        int line,
        const char *func,
        const char *message) {
        Log::TraceLog(Log::TraceLogLevel::LOG_INFO, "FMOD: %s", message);
        return FMOD_OK;
    }

    struct FModSoundWrapper {
        FMOD::Sound* sound = nullptr;
        FMOD::Channel* channel = nullptr;
        bool is_memory_file = false;
        int sample_rate = 0;
        double length = 0.0;
        float volume = 1.0f;
    };

    struct FModSoundRefCountWrapper {
        FMOD::Sound* sound = nullptr;
        int ref_count = 0;
    };

    struct FModSfxWrapper {
        // borrowed from FModSoundWrapper; should not operate it directly.
        void* opaque = nullptr;
        // A Channel is considered playing after System::playSound or System::playDSP, even if it is paused.
        // create a discrete flag to control its playback.
        bool is_playing = false;
    };

    FMOD_RESULT F_CALL FModSfxPlaybackEndCallback(
        FMOD_CHANNELCONTROL *channelcontrol,
        FMOD_CHANNELCONTROL_TYPE controltype,
        FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
        void *, void *) {
        if (callbacktype == FMOD_CHANNELCONTROL_CALLBACK_END && controltype == FMOD_CHANNELCONTROL_CHANNEL) {
            auto *channel = reinterpret_cast<FMOD::Channel*>(channelcontrol);
            void *userData;
            channel->getUserData(&userData);

            auto sfxWrapper = static_cast<FModSfxWrapper*>(userData);
            sfxWrapper->is_playing = false;
        }
        return FMOD_OK;
    }

    FMOD::System* _system = nullptr;

    std::unordered_map<std::string, FModSoundRefCountWrapper> named_loaded_sounds;

    // see https://www.fmod.com/attribution for detailed info.
    bool AudioEngineNeedAttribution() {
        return true;
    }

    std::string GetAudioEngineAttributionInfo() {
        return "Made using FMOD Studio by Firelight Technologies Pty Ltd.";
    }

    std::string GetAudioEngineLogoPath() {
        return "assets/fmod.png";
    }

    std::string GetAudioEngineImplementer() {
        return "FMOD Core";
    }

    void InitAudioDevice()
    {
        FMOD::Debug_Initialize(FMOD_DEBUG_LEVEL_LOG,
                               FMOD_DEBUG_MODE_CALLBACK,
                               FModDebugCallback,
                               nullptr);

        FMOD_RESULT result = FMOD::System_Create(&_system);      // Create the main system object.
        if (result != FMOD_OK)
        {
            Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "FMOD error! (%s)\n", result);
            abort();
        }
        _system->setDSPBufferSize(
            128, // dspBufferLength
            4);  // dspBufferSize

        result = _system->init(512, FMOD_INIT_NORMAL, 0);    // Initialize FMOD.
        if (result != FMOD_OK)
        {
            Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "FMOD error! (%s)\n", result);
            abort();
        }
    }

    void CloseAudioDevice()
    {
        _system->release();
    }

    Music LoadMusicStreamFModImpl(bool is_mem_stream, const char* path_or_data, int mem_stream_file_size) {
        Music m{};
        m.looping = false;
        m.pitch = 1.0f;

        FMOD_RESULT result = FMOD_OK;
        FMOD::Sound* sound;
        bool need_create_entry = false;

        if (is_mem_stream) {
            const char* data = path_or_data;
            int dataSize = mem_stream_file_size;

            FMOD_CREATESOUNDEXINFO exInfo{
                .cbsize = sizeof(FMOD_CREATESOUNDEXINFO),
                .length = static_cast<unsigned>(dataSize)
            };
            result = _system->createSound(data,
                                          FMOD_LOOP_NORMAL | FMOD_OPENMEMORY | FMOD_ACCURATETIME,
                                          &exInfo,
                                          &sound);
        }
        else {
            const char* path = path_or_data;
            if (named_loaded_sounds.contains(path))
            {
                auto& load_count_ref = named_loaded_sounds[path];
                sound = load_count_ref.sound;
                load_count_ref.ref_count++;
            }
            else
            {
                result = _system->createSound(path,
                    FMOD_LOOP_NORMAL | FMOD_ACCURATETIME,
                    nullptr,
                    &sound);
                need_create_entry = true;
            }
        }


        if (result != FMOD_OK)
        {
            Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "FMOD error! (%s)\n", result);
            abort();
        }
        if (need_create_entry) {
            named_loaded_sounds.emplace(std::string(path_or_data), FModSoundRefCountWrapper{.sound = sound, .ref_count = 1});
        }

        unsigned length = 0;
        sound->getLength(&length, FMOD_TIMEUNIT_PCM);

        float frequency;
        int priority;

        sound->getDefaults(&frequency, &priority);
        int sample_rate = std::floor(frequency);

        // ReSharper disable once CppDFAMemoryLeak
        auto loaded_channel = new FModSoundWrapper{
            .sound = sound,
            .channel = nullptr,
            .is_memory_file = false,
            .sample_rate = sample_rate,
            .length = length * 1.0 / sample_rate
        };
        m.implementationDefined = loaded_channel;
        return m;
    }

    Music LoadMusicStream(const char* path)
    {
        return LoadMusicStreamFModImpl(false, path, 0);
    }

    Music LoadMusicStreamFromMemory(const char *fileType, const unsigned char *data, int dataSize)
    {
        return LoadMusicStreamFModImpl(true, reinterpret_cast<const char*>(data), dataSize);
    }

    void UnloadMusicStream(Music m) {
        if (m.implementationDefined) {
            auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
            bool channelIsPlaying = false;
            wrapper->channel->isPlaying(&channelIsPlaying);
            if (channelIsPlaying) wrapper->channel->stop();
            wrapper->channel = nullptr;

            if (wrapper->is_memory_file) {
                wrapper->sound->release();
            }
            else {
                if (auto iter= std::ranges::find_if(named_loaded_sounds,
                    [&wrapper](const auto& sounds_pair) {
                        return sounds_pair.second.sound == wrapper->sound;
                    }); iter != named_loaded_sounds.end()) {
                    iter->second.ref_count--;
                    if (iter->second.ref_count == 0) {
                        iter->second.sound->release();
                        iter->second.sound = nullptr;
                        named_loaded_sounds.erase(iter);
                    }
                }
            }
            wrapper->sound = nullptr;
            delete wrapper;
            m.implementationDefined = nullptr;
        }
    }

    void PlayMusicStreamFModImpl(Music m, bool is_sfx, void* userdata) {
        FMOD_RESULT result;
        FMOD::Channel* channel;

        if (!m.implementationDefined) return;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        result = _system->playSound(wrapper->sound,
            nullptr,
            true,
            &channel
        );
        if (result != FMOD_OK)
        {
            Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "FMOD error! (%s)\n", result);
            abort();
        }
        channel->setVolume(wrapper->volume);
        unsigned length = 0;
        wrapper->sound->getLength(&length, FMOD_TIMEUNIT_PCM);

        channel->setLoopPoints(
            0, FMOD_TIMEUNIT_PCM,
            length, FMOD_TIMEUNIT_PCM
        );
        channel->setLoopCount(m.looping ? -1 : 0);
        wrapper->channel = channel;
        if (is_sfx) {
            // install callback
            channel->setUserData(userdata);
            channel->setCallback(FModSfxPlaybackEndCallback);
        }
        channel->setPaused(false);

    }

    void PlayMusicStream(Music m) {
        PlayMusicStreamFModImpl(m, false, nullptr);
    }

    void UpdateMusicStream(Music) { _system->update(); }

    void StopMusicStream(Music m) {
        if (!m.implementationDefined) return;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        wrapper->channel->stop();
        wrapper->channel = nullptr;
    }

    void PauseMusicStream(Music m) {
        if (!m.implementationDefined) return;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        if (!wrapper->channel) return;

        wrapper->channel->setPaused(true);
    }

    void ResumeMusicStream(Music m) {
        if (!m.implementationDefined) return;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        if (!wrapper->channel) return;

        wrapper->channel->setPaused(false);
    }

    bool IsMusicStreamPlaying(Music m) {
        if (!m.implementationDefined) return false;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        if (!wrapper->channel) return false;

        bool isPlaying = false;
        wrapper->channel->isPlaying(&isPlaying);
        return isPlaying;
    }

    double GetMusicTimeLength(Music m) {
        if (!m.implementationDefined) return -1;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);

        return wrapper->length;
    }

    double GetMusicTimePlayedFModImpl(Music m) {
        if (!m.implementationDefined) return -1;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        if (!wrapper->channel) return 0;

        unsigned position = 0;
        wrapper->channel->getPosition(&position, FMOD_TIMEUNIT_PCM);
        return position * 1.0 / wrapper->sample_rate;
    }

    double GetMusicTimePlayed(Music m) {
        if (fabs(m.pitch - 1.0f) > 1e-6)
            return GetMusicTimePlayedFModImpl(m);
        auto now = std::chrono::steady_clock::now();
        auto audioTime = GetMusicTimePlayedFModImpl(m);
        if (lastAudioTime == 0.0) { // 第一次调用，初始化
            lastAudioTime = audioTime;
            lastSystemTime = now;
            return lastAudioTime;
        }
        if (fabs(audioTime - lastAudioTime) >
#if !defined(PLATFORM_ANDROID)
            1e-2)
#else
            1)
#endif
        {
            lastAudioTime = audioTime;
            lastSystemTime = now;
            return static_cast<float>(audioTime);
        }
        double delta = std::chrono::duration<double>(now - lastSystemTime).count();
        return lastAudioTime + delta;
    }

    void SetMusicPitch(Music m, float pitch) {
        m.pitch = pitch;
        if (!m.implementationDefined) return;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        if (!wrapper->channel) return;
        
        wrapper->channel->setPitch(pitch);
    }

    void SetMusicVolume(Music m, float volume) {
        if (!m.implementationDefined) return;
        auto wrapper = static_cast<FModSoundWrapper *>(m.implementationDefined);
        wrapper->volume = volume;
        if (!wrapper->channel) return;

        wrapper->channel->setVolume(volume);
    }

    void* GetFModSfxWrapperOpaque(void* sfx_wrapper) {
        if (!sfx_wrapper) return nullptr;
        auto wrapper = static_cast<FModSfxWrapper *>(sfx_wrapper);
        return wrapper->opaque;
    }

    bool GetFModSfxWrapperIsPlaying(void* sfx_wrapper) {
        if (!sfx_wrapper) return false;
        auto wrapper = static_cast<FModSfxWrapper *>(sfx_wrapper);
        return wrapper->is_playing;
    }

    void WriteFModSfxWrapperPlaying(void* sfx_wrapper, bool playing) {
        if (!sfx_wrapper) return;
        auto wrapper = static_cast<FModSfxWrapper *>(sfx_wrapper);
        wrapper->is_playing = playing;
    }

    Sound LoadSound(const char* fileName)
    {
        Sound s{};
        Music m = LoadMusicStream(fileName);
        s.implementationDefined = new FModSfxWrapper {
            .opaque =  m.implementationDefined,
            .is_playing = false
        };
        // ReSharper disable once CppDFAMemoryLeak
        return s;
    }

    Sound LoadSoundFromMemory(const char *fileType, const unsigned char *data, int dataSize)
    {
        Sound s{};
        Music m = LoadMusicStreamFromMemory(fileType, data, dataSize);
        s.implementationDefined = new FModSfxWrapper {
            .opaque =  m.implementationDefined,
            .is_playing = false
        };
        // ReSharper disable once CppDFAMemoryLeak
        return s;
    }

    void UnloadSound(Sound s)
    {
        Music m{
            .looping = false,
            .pitch = 0.0f,
            .implementationDefined = GetFModSfxWrapperOpaque(s.implementationDefined),
        };
        UnloadMusicStream(m);
        delete static_cast<FModSfxWrapper *>(s.implementationDefined);
    }

    void PlaySound(Sound s) {
        Music m{
            .looping = false,
            .pitch = 0.0f,
            .implementationDefined = GetFModSfxWrapperOpaque(s.implementationDefined),
        };
        PlayMusicStreamFModImpl(m, true, s.implementationDefined);
        WriteFModSfxWrapperPlaying(s.implementationDefined, true);
    }
    void StopSound(Sound s) {
        Music m{
            .looping = false,
            .pitch = 0.0f,
            .implementationDefined = GetFModSfxWrapperOpaque(s.implementationDefined),
        };
        StopMusicStream(m);
        WriteFModSfxWrapperPlaying(s.implementationDefined, false);
    }
    void PauseSound(Sound s) {
        Music m{
            .looping = false,
            .pitch = 0.0f,
            .implementationDefined = GetFModSfxWrapperOpaque(s.implementationDefined),
        };
        PauseMusicStream(m);
        WriteFModSfxWrapperPlaying(s.implementationDefined, false);
    }
    void ResumeSound(Sound s) {
        Music m{
            .looping = false,
            .pitch = 0.0f,
            .implementationDefined = GetFModSfxWrapperOpaque(s.implementationDefined),
        };
        ResumeMusicStream(m);
        WriteFModSfxWrapperPlaying(s.implementationDefined, true);
    }
    bool IsSoundPlaying(Sound s) {
        return GetFModSfxWrapperIsPlaying(s.implementationDefined);
    }
    bool IsSoundValid(Sound sound) { return sound.implementationDefined != nullptr; }
    void SetSoundVolume(Sound s, float volume) {
        Music m{
            .looping = false,
            .pitch = 0.0f,
            .implementationDefined = GetFModSfxWrapperOpaque(s.implementationDefined),
        };
        SetMusicVolume(m, volume);
    }
}
