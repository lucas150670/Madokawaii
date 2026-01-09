//
// Created by madoka on 2025/12/28.
//
#include <complex>
#include <map>

#include "Madokawaii/app/def.h"
#include "Madokawaii/app/note_hit.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"

struct ResPack_Audio_Decompressed
{
    Madokawaii::Platform::Audio::Sound clickSound[20], flickSound[20], dragSound[20];
};
ResPack_Audio_Decompressed audio_sfx_decompressed{};
std::map<int, int> sfx_play_map;

int InitializeNoteHitSfxManager(Madokawaii::App::ResPack::ResPack& respack)
{
    auto loadSoundFromResPackData = [](const Madokawaii::App::ResPack::ResPackData* resData) -> Madokawaii::Platform::Audio::Sound {
        auto s = Madokawaii::Platform::Audio::LoadSoundFromMemory(".ogg", static_cast<const unsigned char*>(resData->data), static_cast<int>(resData->size));
        return s;
    };
    for (int i = 0; i < 20; i++) {
        audio_sfx_decompressed.clickSound[i] = loadSoundFromResPackData(respack.soundClick);
        audio_sfx_decompressed.flickSound[i] = loadSoundFromResPackData(respack.soundFlick);
        audio_sfx_decompressed.dragSound[i] = loadSoundFromResPackData(respack.soundDrag);
    }
    return 0;
}

void CleanupNoteHitSfxManager() {
    sfx_play_map.clear();
}

void UnloadNoteHitSfxManager() {
    auto unloadSound = [](const Madokawaii::Platform::Audio::Sound& sound) { Madokawaii::Platform::Audio::UnloadSound(sound); };
    for (int i = 0; i < 20; i++) {
        unloadSound(audio_sfx_decompressed.clickSound[i]);
        unloadSound(audio_sfx_decompressed.flickSound[i]);
        unloadSound(audio_sfx_decompressed.dragSound[i]);
    }
    memset(&audio_sfx_decompressed, 0, sizeof(ResPack_Audio_Decompressed));
    sfx_play_map.clear();
}

void RegisterNoteHitSfx(int type) {
    if (!sfx_play_map.contains(type)) sfx_play_map[type] = 1;
    else sfx_play_map[type]++;
}

void UpdateNoteHitSfx() {
    int sel_count = 0;
    Madokawaii::Platform::Audio::Sound* sel_sound = nullptr;
    for (auto& [type, count] : sfx_play_map) {
        switch (type) {
            case Madokawaii::App::NoteType::tap:
            case Madokawaii::App::NoteType::hold:
                sel_sound = audio_sfx_decompressed.clickSound;
                break;
            case Madokawaii::App::NoteType::drag:
                sel_sound = audio_sfx_decompressed.dragSound;
                break;
            case Madokawaii::App::NoteType::flick:
                sel_sound = audio_sfx_decompressed.flickSound;
                break;
            default:
                return;
        }
        sel_count = count;
        if (sel_count > 0) {
            int i = 0, played_sfx = 0, wait_times = 0;
            while (played_sfx < sel_count) {
                if (Madokawaii::Platform::Audio::IsSoundPlaying(sel_sound[i])) {
                    i++; wait_times++;
                    if (i == 20) i = 0;
                    if (wait_times == 20) break;
                }
                else {
                    Madokawaii::Platform::Audio::PlaySound(sel_sound[i]);
                    played_sfx++;
                    if (played_sfx == sel_count)
                        break;
                }
            }
        }
    }

}