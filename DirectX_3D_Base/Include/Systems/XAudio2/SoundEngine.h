/*****************************************************************//**
 * @file	SoundEngine.h
 * @brief	XAudio2のマスターコントローラとなるSoundEngineクラス
 * 
 * @details	
 * XAudio2の初期化、マスターボイスの管理、サンプリングレートなどの
 * グローバルな設定をになるシングルトン。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：SoundEngineクラスの定義
 *						- 追加：SoundEffectとの連携のためにWAVファイルを読み込む機能
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SOUND_ENGINE_H___
#define ___SOUND_ENGINE_H___

// ===== インクルード =====
#include <windows.h>
#include <xaudio2.h>
#include <mmreg.h>
#include <stdexcept>
#include <string>
#include <iostream>

#pragma comment(lib, "XAudio2.lib")

namespace Audio
{
	/**
	 * @struct	LoadWavData
	 * @brief	WAVファイルから読み込んだデータを保持する構造体
	 */
	struct LoadWavData
	{
		WAVEFORMATEX* format = nullptr;	// サウンドデータのフォーマット
		BYTE* audioData = nullptr;		// サウンドデータ本体
		DWORD audioBytes = 0;			// サウンドデータのバイトサイズ

		/**
		 * @brief リソースを解放する。
		 */
		void Release()
		{
			if (format)
			{
				delete[] reinterpret_cast<BYTE*>(format);
				format = nullptr;
			}
			if (audioData)
			{
				delete[] audioData;
				audioData = nullptr;
			}
			audioBytes = 0;
		}
	};

	/**
	 * @class	SoundEngine
	 * @brief	XAudio2の初期化とマスターボイスを管理するシングルトンクラス
	 */
	class SoundEngine
	{
	public:
		static SoundEngine* s_instance;

	private:
		// XAudio2 エンジンインターフェース
		IXAudio2* m_pXAudio2 = nullptr;
		// マスターボイス（サウンド出力デバイス）
		IXAudio2MasteringVoice* m_pMasteringVoice = nullptr;
		// コールバック処理のためのクラス（未使用だが定義）
		class VoiceCallback : public IXAudio2VoiceCallback
		{
		public:
			STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 BytesRequired) {}
			STDMETHOD_(void, OnVoiceProcessingPassEnd)(THIS) {}
			STDMETHOD_(void, OnStreamEnd)(THIS) {}
			STDMETHOD_(void, OnBufferStart)(THIS_ void* pBufferContext) {}
			STDMETHOD_(void, OnBufferEnd)(THIS_ void* pBufferContext) {}
			STDMETHOD_(void, OnLoopEnd)(THIS_ void* pBufferContext) {}
			STDMETHOD_(void, OnVoiceError)(THIS_ void* pBufferContext, HRESULT Error)
			{
				std::cerr << "XAudio2 Voice Error: " << std::hex << Error << std::endl;
			}
		} m_voiceCallback;


	private:
		// 外部からのインスタンス化を禁止
		SoundEngine() = default;
		// コピー、ムーブを禁止
		SoundEngine(const SoundEngine&) = delete;
		SoundEngine& operator=(const SoundEngine&) = delete;
		SoundEngine(SoundEngine&&) = delete;
		SoundEngine& operator=(SoundEngine&&) = delete;

	public:
		/**
		 * [SoundEngine & - GetInstance]
		 * @brief	シングルトンインスタンスを取得する。
		 * 
		 * @return	SoundEngineインスタンスへのポインタ
		 */
		static SoundEngine& GetInstance()
		{
			if (s_instance == nullptr)
			{
				s_instance = new SoundEngine();
			}
			return *s_instance;
		}

		/**
		 * [void - ReleaseInstance]
		 * @brief	シングルトンインスタンスを解放する。
		 */
		static void ReleaseInstance()
		{
			if (s_instance != nullptr)
			{
				delete s_instance;
				s_instance = nullptr;
			}
		}

		/**
		 * [bool - Initialize]
		 * @brief	XAudio2エンジンとマスターボイスを初期化する。
		 * 
		 * @return	true: 成功, false: 失敗
		 */
		bool Initialize();

		/**
		 * [void - Terminate]
		 * @brief	XAudio2エンジンを終了し、リソースを解放する。
		 */
		void Terminate();

		/**
		 * [IXAudio2* - GetXAudio2Engine]
		 * @brief	IXAudio2インスタンスを取得する。
		 * 
		 * @return	IXAudio2インスタンスへのポインタ。初期化に失敗している場合はnullptr。
		 */
		IXAudio2* GetXAudio2Engine() const { return m_pXAudio2; }

		/**
		 * [bool - LoadWavFile]
		 * @brief	WAVファイルを読み込み、LoadWavData構造体に格納する。
		 * 
		 * @param	[in] filePath 読み込むWAVファイルのパス
		 * @param	[out] wavData 読み込んだデータを受け取る構造体
		 * @return	true: 成功, false: 失敗
		 */
		bool LoadWavFile(const std::string& filePath, LoadWavData& wavData);

		// TODO: 将来的な機能拡張として、グローバルなボリューム調整メソッドなどを追加可能
	};
}

#endif // !___SOUND_ENGINE_H___