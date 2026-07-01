//
// Created by madoka on 2026/7/1.
//

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/platform/core.hpp"
#include "Madokawaii/platform/log.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "cri_adx2le.h"

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#include <stb_vorbis.c>

namespace Madokawaii::Platform::Audio {

    namespace {
        constexpr CriSint32 RAW_PCM_VOICES = 96;
        constexpr CriSint32 STANDARD_VOICES = 16;
        constexpr CriSint32 MAX_CHANNELS = 2;
        constexpr CriSint32 MAX_SAMPLING_RATE = 192000;
        constexpr CriSint32 MAX_PATH_LENGTH = 2048;
        constexpr int CRI_HEADER_BYTES = 2048;
        constexpr int PLAYER_STOP_WAIT_TICKS = 4096;
        constexpr int DATA_RELEASE_WAIT_TICKS = 4096;

        struct DecodedPcm {
            std::vector<unsigned char> bytes;
            CriSint32 channels = 0;
            CriSint32 sample_rate = 0;
            CriSint64 frames = 0;
        };

        enum class SourceKind {
            RawPcm,
            NativeMemory,
            NativeFile
        };

        struct CriSoundWrapper {
            CriAtomExPlayerHn player = nullptr;
            CriAtomExPlaybackId playback_id = CRIATOMEX_INVALID_PLAYBACK_ID;
            std::vector<unsigned char> data;
            std::string file_path;
            SourceKind source_kind = SourceKind::RawPcm;
            CriAtomExFormat format = CRIATOMEX_FORMAT_RAW_PCM;
            CriSint32 channels = 0;
            CriSint32 sample_rate = 0;
            CriSint64 frames = 0;
            double length = 0.0;
            float volume = 1.0f;
            float pitch = 1.0f;
            bool looping = false;
            bool started = false;
            bool stopped_by_user = false;
            bool has_time_sample = false;
            double last_audio_time = 0.0;
            std::chrono::steady_clock::time_point last_system_time{};
        };

        struct CriSfxWrapper {
            void* opaque = nullptr;
            bool is_playing = false;
        };

        struct PendingAudioMemory {
            std::vector<unsigned char> data;
        };

        bool g_initialized = false;
        CriAtomExVoicePoolHn g_standard_voice_pool = nullptr;
        CriAtomExVoicePoolHn g_raw_pcm_voice_pool = nullptr;
        CriAtomExVoicePoolHn g_hca_mx_voice_pool = nullptr;
        std::vector<PendingAudioMemory> g_pending_audio_memory;

        void* CRIAPI CriMalloc(void*, CriUint32 size) {
            return std::malloc(static_cast<std::size_t>(size));
        }

        void CRIAPI CriFree(void*, void* ptr) {
            std::free(ptr);
        }

        void LogCriError(const char* action) {
            Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: %s failed", action);
        }

        std::string ToLower(std::string value) {
            std::ranges::transform(value, value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        std::string NormalizeExtension(std::string_view value) {
            std::string ext(value);
            ext = ToLower(ext);
            if (!ext.empty() && ext.front() != '.') {
                ext.insert(ext.begin(), '.');
            }
            return ext;
        }

        std::string ExtensionFromPath(const char* path) {
            if (!path) {
                return {};
            }
            const std::string_view value(path);
            const auto dot = value.find_last_of('.');
            if (dot == std::string_view::npos) {
                return {};
            }
            const auto separator = value.find_last_of("/\\");
            if (separator != std::string_view::npos && dot < separator) {
                return {};
            }
            return NormalizeExtension(value.substr(dot));
        }

        bool IsDecodedByFrontend(std::string_view ext) {
            return ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg";
        }

        bool IsCriNativeExtension(std::string_view ext) {
            return ext == ".adx" || ext == ".hca" || ext == ".hcamx" || ext == ".hca_mx";
        }

        bool IsHcaMxExtension(std::string_view ext) {
            return ext == ".hcamx" || ext == ".hca_mx";
        }

        std::vector<unsigned char> ReadFileBytes(const char* path, std::size_t max_bytes = 0) {
            std::vector<unsigned char> result;
            if (!path) {
                return result;
            }

            int file_size = 0;
            unsigned char* file_data = Core::LoadFileData(path, &file_size);
            if (!file_data || file_size <= 0) {
                if (file_data) {
                    Core::UnloadFileData(file_data);
                }
                Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: Failed to open audio file: %s", path);
                return result;
            }

            auto bytes_to_read = static_cast<std::size_t>(file_size);
            if (max_bytes != 0) {
                bytes_to_read = std::min(bytes_to_read, max_bytes);
            }

            result.assign(file_data, file_data + bytes_to_read);
            Core::UnloadFileData(file_data);
            return result;
        }

        bool CopyPcmToBytes(const int16_t* pcm, std::size_t sample_count, DecodedPcm* out) {
            if (!pcm || !out) {
                return false;
            }
            if (sample_count > std::numeric_limits<std::size_t>::max() / sizeof(int16_t)) {
                return false;
            }

            const auto bytes = sample_count * sizeof(int16_t);
            out->bytes.resize(bytes);
            std::memcpy(out->bytes.data(), pcm, bytes);
            return true;
        }

        std::optional<DecodedPcm> DecodeWav(const unsigned char* data, int data_size) {
            unsigned channels = 0;
            unsigned sample_rate = 0;
            drwav_uint64 frames = 0;
            drwav_int16* pcm = drwav_open_memory_and_read_pcm_frames_s16(
                data, static_cast<std::size_t>(data_size), &channels, &sample_rate, &frames, nullptr);
            if (!pcm) {
                return std::nullopt;
            }

            DecodedPcm decoded{};
            decoded.channels = static_cast<CriSint32>(channels);
            decoded.sample_rate = static_cast<CriSint32>(sample_rate);
            decoded.frames = static_cast<CriSint64>(frames);

            const auto sample_count = static_cast<std::size_t>(frames) * channels;
            const bool copied = CopyPcmToBytes(reinterpret_cast<const int16_t*>(pcm), sample_count, &decoded);
            drwav_free(pcm, nullptr);
            if (!copied) {
                return std::nullopt;
            }
            return decoded;
        }

        std::optional<DecodedPcm> DecodeFlac(const unsigned char* data, int data_size) {
            unsigned channels = 0;
            unsigned sample_rate = 0;
            drflac_uint64 frames = 0;
            drflac_int16* pcm = drflac_open_memory_and_read_pcm_frames_s16(
                data, static_cast<std::size_t>(data_size), &channels, &sample_rate, &frames, nullptr);
            if (!pcm) {
                return std::nullopt;
            }

            DecodedPcm decoded{};
            decoded.channels = static_cast<CriSint32>(channels);
            decoded.sample_rate = static_cast<CriSint32>(sample_rate);
            decoded.frames = static_cast<CriSint64>(frames);

            const auto sample_count = static_cast<std::size_t>(frames) * channels;
            const bool copied = CopyPcmToBytes(reinterpret_cast<const int16_t*>(pcm), sample_count, &decoded);
            drflac_free(pcm, nullptr);
            if (!copied) {
                return std::nullopt;
            }
            return decoded;
        }

        std::optional<DecodedPcm> DecodeMp3(const unsigned char* data, int data_size) {
            drmp3_config config{};
            drmp3_uint64 frames = 0;
            drmp3_int16* pcm = drmp3_open_memory_and_read_pcm_frames_s16(
                data, static_cast<std::size_t>(data_size), &config, &frames, nullptr);
            if (!pcm) {
                return std::nullopt;
            }

            DecodedPcm decoded{};
            decoded.channels = static_cast<CriSint32>(config.channels);
            decoded.sample_rate = static_cast<CriSint32>(config.sampleRate);
            decoded.frames = static_cast<CriSint64>(frames);

            const auto sample_count = static_cast<std::size_t>(frames) * config.channels;
            const bool copied = CopyPcmToBytes(reinterpret_cast<const int16_t*>(pcm), sample_count, &decoded);
            drmp3_free(pcm, nullptr);
            if (!copied) {
                return std::nullopt;
            }
            return decoded;
        }

        std::optional<DecodedPcm> DecodeOgg(const unsigned char* data, int data_size) {
            int channels = 0;
            int sample_rate = 0;
            short* pcm = nullptr;
            const int frames = stb_vorbis_decode_memory(data, data_size, &channels, &sample_rate, &pcm);
            if (frames < 0 || !pcm) {
                return std::nullopt;
            }

            DecodedPcm decoded{};
            decoded.channels = static_cast<CriSint32>(channels);
            decoded.sample_rate = static_cast<CriSint32>(sample_rate);
            decoded.frames = static_cast<CriSint64>(frames);

            const auto sample_count = static_cast<std::size_t>(frames) * static_cast<std::size_t>(channels);
            const bool copied = CopyPcmToBytes(reinterpret_cast<const int16_t*>(pcm), sample_count, &decoded);
            std::free(pcm);
            if (!copied) {
                return std::nullopt;
            }
            return decoded;
        }

        std::optional<DecodedPcm> DecodeFrontend(std::string_view ext, const unsigned char* data, int data_size) {
            if (!data || data_size <= 0) {
                return std::nullopt;
            }
            if (ext == ".wav") {
                return DecodeWav(data, data_size);
            }
            if (ext == ".flac") {
                return DecodeFlac(data, data_size);
            }
            if (ext == ".mp3") {
                return DecodeMp3(data, data_size);
            }
            if (ext == ".ogg") {
                return DecodeOgg(data, data_size);
            }
            return std::nullopt;
        }

        bool AnalyzeCriHeader(const unsigned char* data, int data_size, CriAtomExFormatInfo* info) {
            if (!data || data_size <= 0 || !info) {
                return false;
            }
            std::memset(info, 0, sizeof(*info));
            return criAtomEx_AnalyzeAudioHeader(data, data_size, info) == CRI_TRUE;
        }

        bool FillNativeInfoFromMemory(
            std::string_view ext,
            const unsigned char* data,
            int data_size,
            CriAtomExFormatInfo* info) {
            if (!AnalyzeCriHeader(data, data_size, info)) {
                return false;
            }
            if (IsHcaMxExtension(ext)) {
                info->format = CRIATOMEX_FORMAT_HCA_MX;
            }
            return info->format == CRIATOMEX_FORMAT_ADX ||
                   info->format == CRIATOMEX_FORMAT_HCA ||
                   info->format == CRIATOMEX_FORMAT_HCA_MX;
        }

        bool FillNativeInfoFromFile(const char* path, std::string_view ext, CriAtomExFormatInfo* info) {
            const auto header = ReadFileBytes(path, CRI_HEADER_BYTES);
            if (header.empty()) {
                return false;
            }
            return FillNativeInfoFromMemory(ext, header.data(), static_cast<int>(header.size()), info);
        }

        double LengthFromInfo(const CriAtomExFormatInfo& info) {
            if (info.sampling_rate <= 0 || info.num_samples <= 0) {
                return 0.0;
            }
            return static_cast<double>(info.num_samples) / static_cast<double>(info.sampling_rate);
        }

        float PitchRatioToCents(float pitch) {
            if (pitch <= 0.0f) {
                return 0.0f;
            }
            return 1200.0f * std::log2(pitch);
        }

        void ConfigurePlayer(CriSoundWrapper* wrapper) {
            if (!wrapper || !wrapper->player) {
                return;
            }

            if (wrapper->source_kind == SourceKind::NativeFile) {
                criAtomExPlayer_SetFile(wrapper->player, nullptr, wrapper->file_path.c_str());
            } else {
                criAtomExPlayer_SetData(
                    wrapper->player,
                    wrapper->data.data(),
                    static_cast<CriSint32>(wrapper->data.size()));
            }

            criAtomExPlayer_SetFormat(wrapper->player, wrapper->format);
            criAtomExPlayer_SetNumChannels(wrapper->player, wrapper->channels);
            criAtomExPlayer_SetSamplingRate(wrapper->player, wrapper->sample_rate);
            criAtomExPlayer_SetVolume(wrapper->player, wrapper->volume);
            criAtomExPlayer_SetPitch(wrapper->player, PitchRatioToCents(wrapper->pitch));
        }

        CriAtomExPlayerHn CreatePlayer() {
            CriAtomExPlayerConfig config{};
            criAtomExPlayer_SetDefaultConfig(&config);
            config.max_path = MAX_PATH_LENGTH;
            config.max_path_strings = 1;
            config.updates_time = CRI_TRUE;
            return criAtomExPlayer_Create(&config, nullptr, 0);
        }

        bool CreateVoicePools() {
            CriAtomExStandardVoicePoolConfig standard_config{};
            criAtomExVoicePool_SetDefaultConfigForStandardVoicePool(&standard_config);
            standard_config.num_voices = STANDARD_VOICES;
            standard_config.player_config.max_channels = MAX_CHANNELS;
            standard_config.player_config.max_sampling_rate = MAX_SAMPLING_RATE;
            standard_config.player_config.streaming_flag = CRI_TRUE;
            standard_config.is_streaming_only = CRI_FALSE;
            g_standard_voice_pool = criAtomExVoicePool_AllocateStandardVoicePool(&standard_config, nullptr, 0);
            if (!g_standard_voice_pool) {
                LogCriError("AllocateStandardVoicePool");
                return false;
            }

            CriAtomExRawPcmVoicePoolConfig raw_config{};
            criAtomExVoicePool_SetDefaultConfigForRawPcmVoicePool(&raw_config);
            raw_config.num_voices = RAW_PCM_VOICES;
            raw_config.player_config.pcm_format = CRIATOM_PCM_FORMAT_SINT16;
            raw_config.player_config.max_channels = MAX_CHANNELS;
            raw_config.player_config.max_sampling_rate = MAX_SAMPLING_RATE;
            g_raw_pcm_voice_pool = criAtomExVoicePool_AllocateRawPcmVoicePool(&raw_config, nullptr, 0);
            if (!g_raw_pcm_voice_pool) {
                LogCriError("AllocateRawPcmVoicePool");
                return false;
            }

            CriAtomExHcaMxVoicePoolConfig hca_mx_config{};
            criAtomExVoicePool_SetDefaultConfigForHcaMxVoicePool(&hca_mx_config);
            hca_mx_config.num_voices = STANDARD_VOICES;
            hca_mx_config.player_config.max_channels = MAX_CHANNELS;
            hca_mx_config.player_config.max_sampling_rate = MAX_SAMPLING_RATE;
            hca_mx_config.player_config.streaming_flag = CRI_TRUE;
            hca_mx_config.is_streaming_only = CRI_FALSE;
            g_hca_mx_voice_pool = criAtomExVoicePool_AllocateHcaMxVoicePool(&hca_mx_config, nullptr, 0);
            if (!g_hca_mx_voice_pool) {
                Log::TraceLog(Log::TraceLogLevel::LOG_WARNING, "CRI: Failed to allocate HCA-MX voice pool");
            }

            return true;
        }

        void ApplyLoopMode(CriSoundWrapper* wrapper) {
            if (!wrapper || !wrapper->player) {
                return;
            }
            criAtomExPlayer_LimitLoopCount(
                wrapper->player,
                wrapper->looping ? CRIATOMEXPLAYER_NO_LOOP_LIMITATION : CRIATOMEXPLAYER_IGNORE_LOOP);
        }

        void RestartPlayback(CriSoundWrapper* wrapper) {
            if (!wrapper || !wrapper->player) {
                return;
            }
            wrapper->stopped_by_user = false;
            wrapper->started = true;
            wrapper->has_time_sample = false;
            ApplyLoopMode(wrapper);
            wrapper->playback_id = criAtomExPlayer_Start(wrapper->player);
        }

        CriAtomExPlaybackStatus PlaybackStatus(CriSoundWrapper* wrapper) {
            if (!wrapper || wrapper->playback_id == CRIATOMEX_INVALID_PLAYBACK_ID) {
                return CRIATOMEXPLAYBACK_STATUS_REMOVED;
            }
            return criAtomExPlayback_GetStatus(wrapper->playback_id);
        }

        bool IsPlaybackActive(CriSoundWrapper* wrapper) {
            if (!wrapper || !wrapper->player) {
                return false;
            }

            if (wrapper->stopped_by_user) {
                return false;
            }

            const auto status = PlaybackStatus(wrapper);
            if (status == CRIATOMEXPLAYBACK_STATUS_PREP ||
                status == CRIATOMEXPLAYBACK_STATUS_PLAYING) {
                return true;
            }

            if (wrapper->started && wrapper->looping && !wrapper->stopped_by_user) {
                RestartPlayback(wrapper);
                return wrapper->playback_id != CRIATOMEX_INVALID_PLAYBACK_ID;
            }

            wrapper->playback_id = CRIATOMEX_INVALID_PLAYBACK_ID;
            wrapper->started = false;
            return false;
        }

        bool IsAudioMemoryReferenced(const std::vector<unsigned char>& data) {
            if (data.empty()) {
                return false;
            }
            return criAtomEx_IsDataPlaying(
                const_cast<unsigned char*>(data.data()),
                static_cast<CriSint32>(data.size())) == CRI_TRUE;
        }

        void PollPendingAudioMemory() {
            for (auto it = g_pending_audio_memory.begin(); it != g_pending_audio_memory.end();) {
                if (!IsAudioMemoryReferenced(it->data)) {
                    it = g_pending_audio_memory.erase(it);
                } else {
                    ++it;
                }
            }
        }

        bool IsPlayerStopped(CriSoundWrapper* wrapper) {
            if (!wrapper || !wrapper->player) {
                return true;
            }

            const auto status = criAtomExPlayer_GetStatus(wrapper->player);
            return status == CRIATOMEXPLAYER_STATUS_STOP ||
                   status == CRIATOMEXPLAYER_STATUS_PLAYEND ||
                   status == CRIATOMEXPLAYER_STATUS_ERROR;
        }

        void RequestStopPlayback(CriSoundWrapper* wrapper) {
            if (!wrapper || !wrapper->player) {
                return;
            }

            wrapper->stopped_by_user = true;
            wrapper->started = false;
            wrapper->playback_id = CRIATOMEX_INVALID_PLAYBACK_ID;
            wrapper->has_time_sample = false;
            criAtomExPlayer_StopWithoutReleaseTime(wrapper->player);
        }

        bool WaitUntilPlayerStopped(CriSoundWrapper* wrapper, int max_ticks = PLAYER_STOP_WAIT_TICKS) {
            if (!wrapper || !wrapper->player) {
                return true;
            }

            for (int i = 0; i < max_ticks; ++i) {
                if (IsPlayerStopped(wrapper)) {
                    return true;
                }
                criAtomEx_ExecuteMain();
            }
            return IsPlayerStopped(wrapper);
        }

        bool WaitUntilDataReleased(CriSoundWrapper* wrapper, int max_ticks = DATA_RELEASE_WAIT_TICKS) {
            if (!wrapper || wrapper->data.empty()) {
                return true;
            }

            for (int i = 0; i < max_ticks; ++i) {
                PollPendingAudioMemory();
                if (!IsAudioMemoryReferenced(wrapper->data)) {
                    return true;
                }
                criAtomEx_ExecuteMain();
            }
            return !IsAudioMemoryReferenced(wrapper->data);
        }

        void DeferAudioMemoryRelease(CriSoundWrapper* wrapper) {
            if (!wrapper || wrapper->data.empty()) {
                return;
            }
            g_pending_audio_memory.push_back(PendingAudioMemory{std::move(wrapper->data)});
            Log::TraceLog(
                Log::TraceLogLevel::LOG_WARNING,
                "CRI: Audio memory is still referenced after stop; deferring release");
        }

        Music CreateMusicFromDecodedPcm(DecodedPcm&& decoded) {
            Music m{};
            m.looping = false;
            m.pitch = 1.0f;

            if (decoded.bytes.empty() || decoded.channels <= 0 || decoded.sample_rate <= 0 || decoded.frames <= 0) {
                Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: Invalid decoded PCM data");
                return m;
            }

            if (decoded.channels > MAX_CHANNELS || decoded.sample_rate > MAX_SAMPLING_RATE) {
                Log::TraceLog(
                    Log::TraceLogLevel::LOG_WARNING,
                    "CRI: PCM format exceeds configured voice pool (%d ch, %d Hz)",
                    decoded.channels,
                    decoded.sample_rate);
            }

            auto* wrapper = new CriSoundWrapper{};
            wrapper->data = std::move(decoded.bytes);
            wrapper->source_kind = SourceKind::RawPcm;
            wrapper->format = CRIATOMEX_FORMAT_RAW_PCM;
            wrapper->channels = decoded.channels;
            wrapper->sample_rate = decoded.sample_rate;
            wrapper->frames = decoded.frames;
            wrapper->length = static_cast<double>(decoded.frames) / static_cast<double>(decoded.sample_rate);
            wrapper->player = CreatePlayer();
            if (!wrapper->player) {
                delete wrapper;
                LogCriError("CreatePlayer");
                return m;
            }
            ConfigurePlayer(wrapper);

            m.implementationDefined = wrapper;
            return m;
        }

        Music CreateMusicFromNativeMemory(
            std::string_view ext,
            const unsigned char* data,
            int data_size) {
            Music m{};
            m.looping = false;
            m.pitch = 1.0f;

            CriAtomExFormatInfo info{};
            if (!FillNativeInfoFromMemory(ext, data, std::min(data_size, CRI_HEADER_BYTES), &info)) {
                Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: Unsupported native memory audio");
                return m;
            }

            auto* wrapper = new CriSoundWrapper{};
            wrapper->data.assign(data, data + data_size);
            wrapper->source_kind = SourceKind::NativeMemory;
            wrapper->format = info.format;
            wrapper->channels = info.num_channels;
            wrapper->sample_rate = info.sampling_rate;
            wrapper->frames = info.num_samples;
            wrapper->length = LengthFromInfo(info);
            wrapper->player = CreatePlayer();
            if (!wrapper->player) {
                delete wrapper;
                LogCriError("CreatePlayer");
                return m;
            }
            ConfigurePlayer(wrapper);

            m.implementationDefined = wrapper;
            return m;
        }

        Music CreateMusicFromNativeFile(const char* path, std::string_view ext) {
            Music m{};
            m.looping = false;
            m.pitch = 1.0f;

            CriAtomExFormatInfo info{};
            if (!FillNativeInfoFromFile(path, ext, &info)) {
                Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: Unsupported native audio file: %s", path);
                return m;
            }

            auto* wrapper = new CriSoundWrapper{};
            wrapper->file_path = path;
            wrapper->source_kind = SourceKind::NativeFile;
            wrapper->format = info.format;
            wrapper->channels = info.num_channels;
            wrapper->sample_rate = info.sampling_rate;
            wrapper->frames = info.num_samples;
            wrapper->length = LengthFromInfo(info);
            wrapper->player = CreatePlayer();
            if (!wrapper->player) {
                delete wrapper;
                LogCriError("CreatePlayer");
                return m;
            }
            ConfigurePlayer(wrapper);

            m.implementationDefined = wrapper;
            return m;
        }

        Music LoadMusicStreamCriImpl(
            std::string_view ext,
            const unsigned char* data,
            int data_size) {
            if (IsDecodedByFrontend(ext)) {
                auto decoded = DecodeFrontend(ext, data, data_size);
                if (!decoded) {
                    Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: Failed to decode %.*s audio", static_cast<int>(ext.size()), ext.data());
                    return {};
                }
                return CreateMusicFromDecodedPcm(std::move(*decoded));
            }

            return CreateMusicFromNativeMemory(ext, data, data_size);
        }

        CriSoundWrapper* GetCriWrapper(Music m) {
            return static_cast<CriSoundWrapper*>(m.implementationDefined);
        }

        CriSoundWrapper* GetCriWrapperFromSound(Sound s) {
            if (!s.implementationDefined) {
                return nullptr;
            }
            auto* sfx = static_cast<CriSfxWrapper*>(s.implementationDefined);
            return static_cast<CriSoundWrapper*>(sfx->opaque);
        }

        double GetMusicTimePlayedCriImpl(CriSoundWrapper* wrapper) {
            if (!wrapper) {
                return -1.0;
            }
            if (wrapper->playback_id == CRIATOMEX_INVALID_PLAYBACK_ID) {
                return 0.0;
            }

            CriSint64 samples = 0;
            CriSint32 sample_rate = 0;
            if (criAtomExPlayback_GetNumPlayedSamples(wrapper->playback_id, &samples, &sample_rate) == CRI_TRUE &&
                sample_rate > 0) {
                return static_cast<double>(samples) / static_cast<double>(sample_rate);
            }

            const auto time_ms = criAtomExPlayback_GetTime(wrapper->playback_id);
            if (time_ms >= 0) {
                return static_cast<double>(time_ms) / 1000.0;
            }

            const auto status = PlaybackStatus(wrapper);
            if (status == CRIATOMEXPLAYBACK_STATUS_REMOVED && !wrapper->looping && !wrapper->stopped_by_user) {
                return wrapper->length;
            }
            return 0.0;
        }
    }

    bool AudioEngineNeedAttribution() {
        return true;
    }

    std::string GetAudioEngineAttributionInfo() {
        return "Powered by \"CRIWARE\". CRIWARE is a trademark of CRI Middleware Co., Ltd.";
    }

    std::string GetAudioEngineLogoPath() {
        return "assets/criware_logo01.png";
    }

    std::string GetAudioEngineImplementer() {
        return "CRI ADX LE";
    }

    void InitAudioDevice() {
        if (g_initialized) {
            return;
        }

        criAtomEx_SetUserAllocator(CriMalloc, CriFree, nullptr);

        CriAtomExConfig_PC config{};
        criAtomEx_SetDefaultConfig_PC(&config);
        config.atom_ex.max_virtual_voices = RAW_PCM_VOICES + STANDARD_VOICES;
        config.atom_ex.max_sequences = config.atom_ex.max_virtual_voices;
        config.atom_ex.max_tracks = config.atom_ex.max_virtual_voices * 2;
        config.atom_ex.max_track_items = config.atom_ex.max_virtual_voices * 2;
        config.atom_ex.max_pitch = 2400.0f;
        config.hca_mx.max_voices = STANDARD_VOICES;
        config.hca_mx.max_input_channels = MAX_CHANNELS;
        config.hca_mx.max_sampling_rate = MAX_SAMPLING_RATE;

        criAtomEx_Initialize_PC(&config, nullptr, 0);
        g_initialized = true;

        if (!CreateVoicePools()) {
            CloseAudioDevice();
            abort();
        }
    }

    void CloseAudioDevice() {
        if (!g_initialized) {
            return;
        }
        criAtomExPlayer_StopAllPlayersWithoutReleaseTime();
        for (int i = 0; i < DATA_RELEASE_WAIT_TICKS && !g_pending_audio_memory.empty(); ++i) {
            PollPendingAudioMemory();
            criAtomEx_ExecuteMain();
        }
        criAtomExVoicePool_FreeAll();
        g_standard_voice_pool = nullptr;
        g_raw_pcm_voice_pool = nullptr;
        g_hca_mx_voice_pool = nullptr;
        criAtomEx_Finalize_PC();
        g_pending_audio_memory.clear();
        criAtomEx_SetUserAllocator(nullptr, nullptr, nullptr);
        g_initialized = false;
    }

    Music LoadMusicStream(const char* path) {
        if (!g_initialized) {
            InitAudioDevice();
        }
        PollPendingAudioMemory();

        const auto ext = ExtensionFromPath(path);
        if (IsDecodedByFrontend(ext)) {
            auto bytes = ReadFileBytes(path);
            if (bytes.empty()) {
                return {};
            }
            return LoadMusicStreamCriImpl(ext, bytes.data(), static_cast<int>(bytes.size()));
        }

        if (IsCriNativeExtension(ext)) {
            return CreateMusicFromNativeFile(path, ext);
        }

        const auto header = ReadFileBytes(path, CRI_HEADER_BYTES);
        CriAtomExFormatInfo info{};
        if (!header.empty() && FillNativeInfoFromMemory(ext, header.data(), static_cast<int>(header.size()), &info)) {
            return CreateMusicFromNativeFile(path, ext);
        }

        Log::TraceLog(Log::TraceLogLevel::LOG_ERROR, "CRI: Unsupported audio file type: %s", path ? path : "(null)");
        return {};
    }

    Music LoadMusicStreamFromMemory(const char *fileType, const unsigned char *data, int dataSize)
    {
        if (!g_initialized) {
            InitAudioDevice();
        }
        PollPendingAudioMemory();

        const auto ext = NormalizeExtension(fileType ? std::string_view(fileType) : std::string_view{});
        return LoadMusicStreamCriImpl(ext, data, dataSize);
    }

    void UnloadMusicStream(Music m) {
        if (m.implementationDefined) {
            auto* wrapper = GetCriWrapper(m);
            if (wrapper->player) {
                RequestStopPlayback(wrapper);
                if (!WaitUntilPlayerStopped(wrapper)) {
                    Log::TraceLog(Log::TraceLogLevel::LOG_WARNING, "CRI: Player did not stop before unload");
                }
                if (!WaitUntilDataReleased(wrapper)) {
                    DeferAudioMemoryRelease(wrapper);
                }
                criAtomExPlayer_Destroy(wrapper->player);
                wrapper->player = nullptr;
            }
            delete wrapper;
            PollPendingAudioMemory();
        }
    }

    void PlayMusicStream(Music m) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper || !wrapper->player) {
            return;
        }

        RequestStopPlayback(wrapper);
        WaitUntilPlayerStopped(wrapper);

        wrapper->looping = m.looping;
        wrapper->stopped_by_user = false;
        wrapper->started = true;
        wrapper->has_time_sample = false;
        ApplyLoopMode(wrapper);
        criAtomExPlayer_SetVolume(wrapper->player, wrapper->volume);
        criAtomExPlayer_SetPitch(wrapper->player, PitchRatioToCents(wrapper->pitch));
        wrapper->playback_id = criAtomExPlayer_Start(wrapper->player);
    }

    void UpdateMusicStream(Music m) {
        criAtomEx_ExecuteMain();
        PollPendingAudioMemory();
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper) {
            return;
        }
        wrapper->looping = m.looping;
        IsPlaybackActive(wrapper);
    }

    void StopMusicStream(Music m) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper || !wrapper->player) {
            return;
        }
        RequestStopPlayback(wrapper);
        criAtomEx_ExecuteMain();
        PollPendingAudioMemory();
    }

    void PauseMusicStream(Music m) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper || !wrapper->player) {
            return;
        }
        criAtomExPlayer_Pause(wrapper->player, CRI_TRUE);
    }

    void ResumeMusicStream(Music m) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper || !wrapper->player) {
            return;
        }
        criAtomExPlayer_Pause(wrapper->player, CRI_FALSE);
    }

    bool IsMusicStreamPlaying(Music m) {
        return IsPlaybackActive(GetCriWrapper(m));
    }

    double GetMusicTimeLength(Music m) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper) {
            return -1.0;
        }
        return wrapper->length;
    }

    double GetMusicTimePlayed(Music m) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper) {
            return -1.0;
        }

        if (std::fabs(wrapper->pitch - 1.0f) > 1e-6f) {
            return GetMusicTimePlayedCriImpl(wrapper);
        }

        const auto audio_time = GetMusicTimePlayedCriImpl(wrapper);
        if (audio_time < 0.0) {
            return audio_time;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!wrapper->has_time_sample) {
            wrapper->has_time_sample = true;
            wrapper->last_audio_time = audio_time;
            wrapper->last_system_time = now;
            return audio_time;
        }

        if (std::fabs(audio_time - wrapper->last_audio_time) >
#if !defined(PLATFORM_ANDROID)
            1e-2)
#else
            1)
#endif
        {
            wrapper->last_audio_time = audio_time;
            wrapper->last_system_time = now;
            return audio_time;
        }

        const double delta = std::chrono::duration<double>(now - wrapper->last_system_time).count();
        return wrapper->last_audio_time + delta;
    }


    void SetMusicPitch(Music m, float pitch) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper || !wrapper->player) {
            return;
        }
        wrapper->pitch = pitch;
        criAtomExPlayer_SetPitch(wrapper->player, PitchRatioToCents(pitch));
        if (wrapper->playback_id != CRIATOMEX_INVALID_PLAYBACK_ID) {
            criAtomExPlayer_UpdateAll(wrapper->player);
        }
    }

    void SetMusicVolume(Music m, float volume) {
        auto* wrapper = GetCriWrapper(m);
        if (!wrapper || !wrapper->player) {
            return;
        }
        wrapper->volume = volume;
        criAtomExPlayer_SetVolume(wrapper->player, volume);
        if (wrapper->playback_id != CRIATOMEX_INVALID_PLAYBACK_ID) {
            criAtomExPlayer_UpdateAll(wrapper->player);
        }
    }

    Sound LoadSound(const char* fileName)
    {
        Sound s{};
        Music m = LoadMusicStream(fileName);
        s.implementationDefined = new CriSfxWrapper{
            .opaque = m.implementationDefined,
            .is_playing = false
        };
        return s;
    }

    Sound LoadSoundFromMemory(const char *fileType, const unsigned char *data, int dataSize)
    {
        Sound s{};
        Music m = LoadMusicStreamFromMemory(fileType, data, dataSize);
        s.implementationDefined = new CriSfxWrapper{
            .opaque = m.implementationDefined,
            .is_playing = false
        };
        return s;
    }

    void UnloadSound(Sound s)
    {
        if (!s.implementationDefined) {
            return;
        }
        Music m{
            .looping = false,
            .pitch = 1.0f,
            .implementationDefined = GetCriWrapperFromSound(s),
        };
        UnloadMusicStream(m);
        delete static_cast<CriSfxWrapper*>(s.implementationDefined);
    }

    void PlaySound(Sound sound) {
        if (!sound.implementationDefined) {
            return;
        }
        auto* sfx = static_cast<CriSfxWrapper*>(sound.implementationDefined);
        Music m{
            .looping = false,
            .pitch = 1.0f,
            .implementationDefined = sfx->opaque,
        };
        PlayMusicStream(m);
        sfx->is_playing = true;
    }

    void StopSound(Sound sound) {
        if (!sound.implementationDefined) {
            return;
        }
        auto* sfx = static_cast<CriSfxWrapper*>(sound.implementationDefined);
        Music m{
            .looping = false,
            .pitch = 1.0f,
            .implementationDefined = sfx->opaque,
        };
        StopMusicStream(m);
        sfx->is_playing = false;
    }

    void PauseSound(Sound sound) {
        if (!sound.implementationDefined) {
            return;
        }
        auto* sfx = static_cast<CriSfxWrapper*>(sound.implementationDefined);
        Music m{
            .looping = false,
            .pitch = 1.0f,
            .implementationDefined = sfx->opaque,
        };
        PauseMusicStream(m);
        sfx->is_playing = false;
    }

    void ResumeSound(Sound sound) {
        if (!sound.implementationDefined) {
            return;
        }
        auto* sfx = static_cast<CriSfxWrapper*>(sound.implementationDefined);
        Music m{
            .looping = false,
            .pitch = 1.0f,
            .implementationDefined = sfx->opaque,
        };
        ResumeMusicStream(m);
        sfx->is_playing = true;
    }

    bool IsSoundPlaying(Sound sound) {
        if (!sound.implementationDefined) {
            return false;
        }
        auto* sfx = static_cast<CriSfxWrapper*>(sound.implementationDefined);
        if (!sfx->is_playing) {
            return false;
        }
        const bool active = IsPlaybackActive(static_cast<CriSoundWrapper*>(sfx->opaque));
        if (!active) {
            sfx->is_playing = false;
        }
        return active;
    }

    bool IsSoundValid(Sound sound) {
        return GetCriWrapperFromSound(sound) != nullptr;
    }

    void SetSoundVolume(Sound sound, float volume) {
        Music m{
            .looping = false,
            .pitch = 1.0f,
            .implementationDefined = GetCriWrapperFromSound(sound),
        };
        SetMusicVolume(m, volume);
    }
}
