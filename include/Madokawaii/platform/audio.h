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
    // 调试用：设置音高，可以改变音频播放速率
    void SetMusicPitch(Music, float pitch);
}

#endif //MADOKAWAII_AUDIO_H
