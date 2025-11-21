/*****************************************************************//**
 * @file	SoundEffect.h
 * @brief	単一のサウンドリソース（WAV）と再生ロジックを保持するクラス
 * 
 * @details	
 * AssetManagerによって管理される実体であり、
 * XAudio2のソースボイスを用いて再生を行う。SE/BGMの両方に対応。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：SoundEffectクラスの定義
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SOUND_EFFECT_H___
#define ___SOUND_EFFECT_H___

// ===== インクルード =====
#include "Systems/XAudio2/SoundEngine.h"
#include <string>
#include <iostream>
#include <xaudio2.h>

namespace Audio
{
	/**
	 * @class	SoundEffect
	 * @brief	一つのWAVファイルを保持し、XAudio2ソースボイスを介して再生するリソースクラス。
	 * AssetManager::LoadSoundによってロード・管理されます。
	 */
	class SoundEffect
	{
	private:
		// XAudio2 ボイス
		IXAudio2SourceVoice* m_pSourceVoice = nullptr;
		// サウンドデータ格納用バッファ
		XAUDIO2_BUFFER m_buffer = { 0 };
		// WAVファイルのフォーマット情報
		WAVEFORMATEX* m_format = nullptr;
		// WAVファイルのデータ本体（SoundEngine::LoadWavFileでヒープに確保されたもの）
		BYTE* m_audioData = nullptr;
		// データのバイトサイズ
		DWORD m_audioBytes = 0;

		// 再生中のボイスを監視するためのコールバック
		class VoiceCallback : public IXAudio2VoiceCallback
		{
		public:
			STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 BytesRequired) {}
			STDMETHOD_(void, OnVoiceProcessingPassEnd)(THIS) {}
			STDMETHOD_(void, OnStreamEnd)(THIS)
			{
				// 再生完了時に通知を受けるが、ここでは特に何もしない（メモリ解放はAssetManagerの責務）
			}
			STDMETHOD_(void, OnBufferStart)(THIS_ void* pBufferContext) {}
			STDMETHOD_(void, OnBufferEnd)(THIS_ void* pBufferContext) {}
			STDMETHOD_(void, OnLoopEnd)(THIS_ void* pBufferContext) {}
			STDMETHOD_(void, OnVoiceError)(THIS_ void* pBufferContext, HRESULT Error)
			{
				std::cerr << "XAudio2 SoundEffect Voice Error: " << std::hex << Error << std::endl;
			}
		} m_voiceCallback;

	public:
		/**
		 * @brief コンストラクタ。
		 */
		SoundEffect() = default;

		/**
		 * @brief デストラクタ。リソースを解放する。
		 */
		~SoundEffect();

		// コピー、ムーブを禁止
		SoundEffect(const SoundEffect&) = delete;
		SoundEffect& operator=(const SoundEffect&) = delete;
		SoundEffect(SoundEffect&&) = delete;
		SoundEffect& operator=(SoundEffect&&) = delete;

		/**
		 * [bool - Load]
		 * @brief	WAVファイルをロードし、ソースボイスを作成する。
		 * @param	[in] filePath WAVファイルのパス
		 * @return	true: 成功, false: 失敗
		 */
		bool Load(const std::string& filePath);

		/**
		 * [void - Play]
		 * @brief	サウンドを再生する。
		 * @param	[in] loopCount ループ回数 (0: ループなし, XAUDIO2_LOOP_INFINITE: 無限ループ)
		 * @param	[in] volume ボリューム (0.0f - 1.0f)
		 */
		void Play(UINT32 loopCount = 0, float volume = 1.0f);

		/**
		 * [void - Stop]
		 * @brief	サウンドの再生を停止する。
		 */
		void Stop();

		/**
		 * [bool - IsPlaying]
		 * @brief	現在サウンドが再生中であるかをチェックする。
		 * @return	true: 再生中, false: 停止中
		 */
		bool IsPlaying() const;

		/**
		 * [void - Release]
		 * @brief	リソースを解放する。AssetManagerから呼ばれることを想定。
		 */
		void Release();

		/**
		 * [void - SetVolume]
		 * @brief	ボリュームを設定する。
		 */
		void SetVolume(float volume);
	};
}

#endif // !___SOUND_EFFECT_H___