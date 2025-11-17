#pragma once
#include <string>
#include <vector>
#include <xaudio2.h>

enum class SoundType
{
    BGM,
    SE
};

struct SoundComponent
{
    std::string filePath;                // ファイルパス
    SoundType type;                      // BGM or SE
    bool loop = false;                   // ループ再生
    float volume = 1.0f;                 // 初期音量（0.0?1.0）
    IXAudio2SourceVoice* voice = nullptr;
    std::vector<IXAudio2SourceVoice*> seVoices;

    bool playRequested = false;
    std::vector<BYTE> rawData;
    WAVEFORMATEX wfx = {};

    // 変更：音量を指定できるコンストラクタ
    SoundComponent(const std::string& path, SoundType t, bool loopBGM = false, float vol = 1.0f)
        : filePath(path), type(t), loop(loopBGM), volume(vol), voice(nullptr), playRequested(false)
    {
    }
};