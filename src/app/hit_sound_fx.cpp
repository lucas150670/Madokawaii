//
// Created by madoka on 2025/12/28.
//

#include "Madokawaii/app/common.hpp"
#include "Madokawaii/app/def.hpp"
#include "Madokawaii/app/note_hit.hpp"
#include "Madokawaii/platform/audio.hpp"
#include "Madokawaii/platform/log.hpp"

namespace Madokawaii::App::NoteHit {

int InitializeSfxManager(AppContext& context, ResPack::ResPack& respack)
{
    auto loadSoundFromResPackData = [](const Madokawaii::App::ResPack::ResPackData* resData) -> Madokawaii::Platform::Audio::Sound {
        auto s = Madokawaii::Platform::Audio::LoadSoundFromMemory(".ogg", static_cast<const unsigned char*>(resData->data), static_cast<int>(resData->size));
        return s;
    };
    auto& sfx = context.noteHitSfx;
    for (int i = 0; i < NoteHitSfxState::SOUND_POOL_SIZE; i++) {
        sfx.clickSounds[i] = loadSoundFromResPackData(respack.soundClick);
        sfx.flickSounds[i] = loadSoundFromResPackData(respack.soundFlick);
        sfx.dragSounds[i] = loadSoundFromResPackData(respack.soundDrag);
    }
    return 0;
}

void CleanupSfxManager(AppContext& context) {
    context.noteHitSfx.playMap.clear();
}

void UnloadSfxManager(AppContext& context) {
    auto unloadSound = [](const Madokawaii::Platform::Audio::Sound& sound) { Madokawaii::Platform::Audio::UnloadSound(sound); };
    auto& sfx = context.noteHitSfx;
    for (int i = 0; i < NoteHitSfxState::SOUND_POOL_SIZE; i++) {
        unloadSound(sfx.clickSounds[i]);
        unloadSound(sfx.flickSounds[i]);
        unloadSound(sfx.dragSounds[i]);
    }
    sfx = {};
}

void RegisterSfx(AppContext& context, int type) {
    auto& playMap = context.noteHitSfx.playMap;
    if (!playMap.contains(type)) playMap[type] = 1;
    else playMap[type]++;
}

void UpdateSfx(AppContext& context) {
    int sel_count = 0;
    Madokawaii::Platform::Audio::Sound* sel_sound = nullptr;
    auto& sfx = context.noteHitSfx;
    for (auto& [type, count] : sfx.playMap) {
        switch (type) {
            case Madokawaii::App::NoteType::tap:
            case Madokawaii::App::NoteType::hold:
                sel_sound = sfx.clickSounds.data();
                break;
            case Madokawaii::App::NoteType::drag:
                sel_sound = sfx.dragSounds.data();
                break;
            case Madokawaii::App::NoteType::flick:
                sel_sound = sfx.flickSounds.data();
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
                    if (i == NoteHitSfxState::SOUND_POOL_SIZE) i = 0;
                    if (wait_times == NoteHitSfxState::SOUND_POOL_SIZE) break;
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

} // namespace Madokawaii::App::NoteHit
