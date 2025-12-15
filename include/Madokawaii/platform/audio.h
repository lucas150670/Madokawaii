//
// Created by madoka on 25-9-11.
//

#ifndef MADOKAWAII_AUDIO_H
#define MADOKAWAII_AUDIO_H

namespace Madokawaii::Platform::Audio {
    void InitAudioDevice();
    void CloseAudioDevice();

    struct Music {
        bool looping;
        void *implementationDefined;
    };
    Music LoadMusicStream(const char*);
    Music LoadMusicStreamFromMemory(const char*, const unsigned char*, int);
    void UnloadMusicStream(Music);
    void PlayMusicStream(Music);
    void UpdateMusicStream(Music);
    void StopMusicStream(Music);
    void PauseMusicStream(Music);
    void ResumeMusicStream(Music);
    bool IsMusicStreamPlaying(Music);
    float GetMusicTimeLength(Music);
    float GetMusicTimePlayed(Music);
}

#endif //MADOKAWAII_AUDIO_H
