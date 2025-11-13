#pragma once

#include "ECS/Components/SoundComponent.h"
#include <vector>
#include <xaudio2.h>
#include <string>
#include <memory>
#include <windows.h>

// デバッグ用ヘルパー関数（OutputDebugStringAでログ出力）
inline void DBG(const char* s) { OutputDebugStringA(s); }

class AudioSystem
{
private:
    IXAudio2* xAudio2 = nullptr;
    IXAudio2MasteringVoice* masterVoice = nullptr;
    std::vector<SoundComponent*> sounds;

public:
    AudioSystem();
    ~AudioSystem();

    bool Init();
    void RegisterSound(SoundComponent* sound);
    void RequestPlay(SoundComponent* sound);
    void Update();

    void SetMasterVolume(float volume);
    void SetBGMVolume(SoundComponent* sound, float volume);
    void SetSEVolume(SoundComponent* sound, float volume);

private:
    bool LoadWavFile(const std::string& filename, WAVEFORMATEX& outWfx, std::vector<BYTE>& outBuffer);
};