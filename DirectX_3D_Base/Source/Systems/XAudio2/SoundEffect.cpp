/*****************************************************************//**
 * @file	SoundEffect.cpp
 * @brief	SoundEffectクラスの実装
 * 
 * @details	
 * WAVのロード、ソースボイスの作成、再生・停止処理。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：SoundEffectクラスの実装
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Systems/XAudio2/SoundEffect.h"

namespace Audio
{
	/**
	 * @brief デストラクタ。Releaseを呼び出す。
	 */
	SoundEffect::~SoundEffect()
	{
		Release();
	}

	/**
	 * [bool - Load]
	 * @brief	WAVファイルをロードし、ソースボイスを作成する。
	 * @param	[in] filePath WAVファイルのパス
	 * @return	true: 成功, false: 失敗
	 */
	bool SoundEffect::Load(const std::string& filePath)
	{
		// 1. WAVファイルをロード
		LoadWavData wavData;
		if (!SoundEngine::GetInstance().LoadWavFile(filePath, wavData))
		{
			std::cerr << "Error: Failed to load WAV file data: " << filePath << std::endl;
			return false;
		}

		// SoundEngine::LoadWavFileで確保したデータをメンバー変数に移動
		m_format = wavData.format;
		m_audioData = wavData.audioData;
		m_audioBytes = wavData.audioBytes;

		// 2. XAudio2ソースボイスの作成
		HRESULT hr = SoundEngine::GetInstance().GetXAudio2Engine()->CreateSourceVoice(
			&m_pSourceVoice,
			m_format,
			0,							// フラグ
			XAUDIO2_DEFAULT_FREQ_RATIO,	// 最大ピッチ変化率
			&m_voiceCallback,			// コールバック
			nullptr,					// サブミキシングボイス
			nullptr);					// エフェクトチェーン

		if (FAILED(hr))
		{
			std::cerr << "Error: CreateSourceVoice failed. HRESULT = " << std::hex << hr << std::endl;
			Release();
			return false;
		}

		// 3. XAUDIO2_BUFFERの設定
		m_buffer.AudioBytes = m_audioBytes;
		m_buffer.pAudioData = m_audioData;
		m_buffer.Flags = XAUDIO2_END_OF_STREAM; // 必須フラグ

		std::cout << "SoundEffect loaded and SourceVoice created: " << filePath << std::endl;
		return true;
	}

	/**
	 * [void - Play]
	 * @brief	サウンドを再生する。
	 * @param	[in] loopCount ループ回数 (0: ループなし, XAUDIO2_LOOP_INFINITE: 無限ループ)
	 * @param	[in] volume ボリューム (0.0f - 1.0f)
	 */
	void SoundEffect::Play(UINT32 loopCount, float volume)
	{
		if (!m_pSourceVoice) return;

		// 既に再生中の場合は一度停止する (SE用途の場合)
		m_pSourceVoice->Stop(0);
		m_pSourceVoice->FlushSourceBuffers();

		// ループ設定
		m_buffer.LoopCount = loopCount;

		// ボリューム設定
		SetVolume(volume);

		// バッファをキューに投入
		HRESULT hr = m_pSourceVoice->SubmitSourceBuffer(&m_buffer);
		if (FAILED(hr))
		{
			std::cerr << "Error: SubmitSourceBuffer failed. HRESULT = " << std::hex << hr << std::endl;
			return;
		}

		// 再生開始
		hr = m_pSourceVoice->Start(0);
		if (FAILED(hr))
		{
			std::cerr << "Error: Start voice failed. HRESULT = " << std::hex << hr << std::endl;
		}
	}

	/**
	 * [void - Stop]
	 * @brief	サウンドの再生を停止する。
	 */
	void SoundEffect::Stop()
	{
		if (m_pSourceVoice)
		{
			m_pSourceVoice->Stop(0);
			m_pSourceVoice->FlushSourceBuffers();
		}
	}

	/**
	 * [bool - IsPlaying]
	 * @brief	現在サウンドが再生中であるかをチェックする。
	 * @return	true: 再生中, false: 停止中
	 */
	bool SoundEffect::IsPlaying() const
	{
		if (!m_pSourceVoice) return false;

		XAUDIO2_VOICE_STATE state;
		m_pSourceVoice->GetState(&state);

		return state.BuffersQueued > 0;
	}

	/**
	 * [void - Release]
	 * @brief	リソースを解放する。SoundEngine::LoadWavFileで確保されたメモリも解放する。
	 */
	void SoundEffect::Release()
	{
		if (m_pSourceVoice)
		{
			// 再生停止
			Stop();
			// ボイス解放
			m_pSourceVoice->DestroyVoice();
			m_pSourceVoice = nullptr;
		}

		// SoundEngine::LoadWavFileでヒープに確保されたWAVデータを解放
		if (m_format)
		{
			delete[] reinterpret_cast<BYTE*>(m_format);
			m_format = nullptr;
		}
		if (m_audioData)
		{
			delete[] m_audioData;
			m_audioData = nullptr;
		}
		m_audioBytes = 0;
	}

	/**
	 * [void - SetVolume]
	 * @brief	ボリュームを設定する。
	 */
	void SoundEffect::SetVolume(float volume)
	{
		if (m_pSourceVoice)
		{
			// XAudio2は0.0fからXAUDIO2_MAX_VOLUME_LEVEL (通常4.0f) までの範囲
			// 1.0fが等倍
			m_pSourceVoice->SetVolume(volume);
		}
	}
}