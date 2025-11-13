#include "ECS/Systems/AudioSystem.h"
#include <fstream>

// ==========================================================
// コンストラクタ / デストラクタ
// ==========================================================
AudioSystem::AudioSystem() {}

AudioSystem::~AudioSystem()
{
    DBG("AudioSystem destructor\n");
    for (auto s : sounds)
    {
        if (s->voice) s->voice->DestroyVoice();
        for (auto v : s->seVoices) v->DestroyVoice();
    }
    if (masterVoice) masterVoice->DestroyVoice();
    if (xAudio2) xAudio2->Release();
}

// ==========================================================
// XAudio2 の初期化
// ==========================================================
bool AudioSystem::Init()
{
    DBG("AudioSystem::Init start\n");
    HRESULT hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        char buf[256]; sprintf_s(buf, "XAudio2Create FAILED hr=0x%08x\n", (unsigned)hr);
        DBG(buf);
        return false;
    }
    DBG("XAudio2Create OK\n");

    hr = xAudio2->CreateMasteringVoice(&masterVoice);
    if (FAILED(hr))
    {
        char buf[256]; sprintf_s(buf, "CreateMasteringVoice FAILED hr=0x%08x\n", (unsigned)hr);
        DBG(buf);
        return false;
    }
    DBG("CreateMasteringVoice OK\n");
    return true;
}

// ==========================================================
// サウンド登録処理
// ==========================================================
void AudioSystem::RegisterSound(SoundComponent* sound)
{
    if (!sound) return;
    DBG("RegisterSound start\n");
    sounds.push_back(sound);

    WAVEFORMATEX wfx = {};
    std::vector<BYTE> buffer;
    if (!LoadWavFile(sound->filePath, wfx, buffer))
    {
        DBG("LoadWavFile FAILED\n");
        return;
    }

    IXAudio2SourceVoice* voice = nullptr;
    HRESULT hr = xAudio2->CreateSourceVoice(&voice, &wfx);
    if (FAILED(hr) || !voice)
    {
        DBG("CreateSourceVoice FAILED\n");
        return;
    }

    voice->SetVolume(sound->volume);

    XAUDIO2_BUFFER xb = {};
    xb.pAudioData = buffer.data();
    xb.AudioBytes = static_cast<UINT32>(buffer.size());
    xb.Flags = XAUDIO2_END_OF_STREAM;
    if (sound->type == SoundType::BGM && sound->loop)
        xb.LoopCount = XAUDIO2_LOOP_INFINITE;

    voice->SubmitSourceBuffer(&xb);

    if (sound->type == SoundType::BGM)
    {
        sound->voice = voice;
    }
    else
    {
        sound->seVoices.push_back(voice);
        sound->wfx = wfx;
        sound->rawData = std::move(buffer);
    }

    DBG("RegisterSound end\n");
}

// ==========================================================
// 再生要求セット
// ==========================================================
void AudioSystem::RequestPlay(SoundComponent* sound)
{
    if (!sound) return;
    sound->playRequested = true;
}

// ==========================================================
// 更新処理（再生実行）
// ==========================================================
void AudioSystem::Update()
{
    for (auto sound : sounds)
    {
        if (!sound->playRequested) continue;

        if (sound->type == SoundType::BGM && sound->voice)
        {
            sound->voice->SetVolume(sound->volume);
            DBG("Start BGM\n");
            HRESULT hr = sound->voice->Start(0);
            if (FAILED(hr))
            {
                char buf[128]; sprintf_s(buf, "BGM Start FAILED hr=0x%08x\n", (unsigned)hr);
                DBG(buf);
            }
        }
        else if (sound->type == SoundType::SE)
        {
            DBG("Play SE requested\n");
            bool played = false;
            for (auto v : sound->seVoices)
            {
                XAUDIO2_VOICE_STATE state;
                v->GetState(&state);
                if (state.BuffersQueued == 0)
                {
                    if (!sound->rawData.empty())
                    {
                        XAUDIO2_BUFFER xb = {};
                        xb.pAudioData = sound->rawData.data();
                        xb.AudioBytes = static_cast<UINT32>(sound->rawData.size());
                        xb.Flags = XAUDIO2_END_OF_STREAM;
                        HRESULT hr = v->SubmitSourceBuffer(&xb);
                        if (SUCCEEDED(hr))
                        {
                            v->SetVolume(sound->volume);
                            v->Start(0);
                            played = true;
                            break;
                        }
                    }
                }
            }

            if (!played)
            {
                DBG("No idle SE voice found, creating a new voice\n");
                IXAudio2SourceVoice* newV = nullptr;
                HRESULT hr = xAudio2->CreateSourceVoice(&newV, &sound->wfx);
                if (SUCCEEDED(hr) && newV)
                {
                    XAUDIO2_BUFFER xb = {};
                    xb.pAudioData = sound->rawData.data();
                    xb.AudioBytes = static_cast<UINT32>(sound->rawData.size());
                    xb.Flags = XAUDIO2_END_OF_STREAM;
                    hr = newV->SubmitSourceBuffer(&xb);
                    if (SUCCEEDED(hr))
                    {
                        newV->SetVolume(sound->volume);
                        newV->Start(0);
                        sound->seVoices.push_back(newV);
                        played = true;
                    }
                    else
                    {
                        newV->DestroyVoice();
                    }
                }
            }
        }

        sound->playRequested = false;
    }
}

// ==========================================================
// 音量調整関数
// ==========================================================
void AudioSystem::SetMasterVolume(float volume)
{
    if (masterVoice)
        masterVoice->SetVolume(volume);
}

void AudioSystem::SetBGMVolume(SoundComponent* sound, float volume)
{
    if (sound && sound->voice)
        sound->voice->SetVolume(volume);
}

void AudioSystem::SetSEVolume(SoundComponent* sound, float volume)
{
    if (!sound) return;
    for (auto v : sound->seVoices)
    {
        if (v)
            v->SetVolume(volume);
    }
}

// ==========================================================
// WAVファイル読み込み処理
// ==========================================================
bool AudioSystem::LoadWavFile(const std::string& filename, WAVEFORMATEX& outWfx, std::vector<BYTE>& outBuffer)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        char buf[256]; sprintf_s(buf, "LoadWavFile: cannot open %s\n", filename.c_str());
        DBG(buf);
        return false;
    }

    char id[4];
    file.read(id, 4); // "RIFF"
    if (strncmp(id, "RIFF", 4) != 0) { DBG("Not RIFF\n"); return false; }
    file.seekg(4, std::ios::cur); // Skip chunk size
    file.read(id, 4); // "WAVE"
    if (strncmp(id, "WAVE", 4) != 0) { DBG("Not WAVE\n"); return false; }

    bool foundFmt = false;
    bool foundData = false;
    WAVEFORMATEX wfx = {};
    std::vector<BYTE> data;

    while (!file.eof())
    {
        file.read(id, 4);
        if (file.eof()) break;
        uint32_t chunkSize = 0;
        file.read(reinterpret_cast<char*>(&chunkSize), 4);

        if (strncmp(id, "fmt ", 4) == 0)
        {
            std::vector<BYTE> fmtBuf(chunkSize);
            file.read(reinterpret_cast<char*>(fmtBuf.data()), chunkSize);
            if (chunkSize >= 16)
                memcpy(&wfx, fmtBuf.data(), sizeof(WAVEFORMATEX));
            foundFmt = true;
        }
        else if (strncmp(id, "data", 4) == 0)
        {
            data.resize(chunkSize);
            file.read(reinterpret_cast<char*>(data.data()), chunkSize);
            foundData = true;
        }
        else
        {
            file.seekg(chunkSize, std::ios::cur);
        }

        if (foundFmt && foundData) break;
    }

    if (!foundFmt || !foundData)
    {
        DBG("fmt または data チャンクが見つかりませんでした\n");
        return false;
    }

    outWfx = wfx;
    outBuffer.swap(data);
    return true;
}