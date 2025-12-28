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

    struct Sound
    {
        void *implementationDefined;
    };
    Sound LoadSound(const char *fileName);
    Sound LoadSoundFromMemory(const char *fileType, const unsigned char *data, int dataSize);
    void UnloadSound(Sound);

    void PlaySound(Sound sound);                                    //播放音频
    void StopSound(Sound sound);                                    //停止播放音频
    void PauseSound(Sound sound);                                   //暂停音频
    void ResumeSound(Sound sound);                                  //恢复暂停的音频
    bool IsSoundPlaying(Sound sound);                               //检查当前是否正在播放音频
    bool IsSoundValid(Sound sound);
    void SetSoundVolume(Sound sound, float);
}

#endif //MADOKAWAII_AUDIO_H
