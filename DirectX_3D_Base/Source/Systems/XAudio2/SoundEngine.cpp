/*****************************************************************//**
 * @file	SoundEngine.cpp
 * @brief	XAudio2のマスターコントローラとなるSoundEngineクラスの実装
 * 
 * @details	
 * XAudio2の初期化、マスターボイスの管理、およびWAVファイル
 * のデータロード処理を実装する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：Initialize/Terminate/LoadWavFileの実装
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Systems/XAudio2/SoundEngine.h"
#include <fstream>
#include <algorithm>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

// シングルトンインスタンスの初期化
Audio::SoundEngine* Audio::SoundEngine::s_instance = nullptr;

namespace Audio
{
	/**
	 * [HRESULT - FindChunk]
	 * @brief	RIFFファイル内で指定されたチャンクを探すヘルパー関数
	 */
	HRESULT FindChunk(HANDLE hFile, FOURCC fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
	{
		HRESULT hr = S_OK;
		DWORD dwChunkType;
		DWORD dwRead;

		// ファイルポインタをRIFFヘッダーの直後（'WAVE'のFOURCC後）まで移動させることを想定
		// FindChunkは、通常、外部で RIFFチャンクと WAVEチャンクのチェックを終えた後に呼ばれます。
		// ここでは、現在のファイルポインタ位置から順次チャンクを検索します。

		while (true)
		{
			// 1. チャンクID (4バイト) を読み込む
			if (FALSE == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
				return HRESULT_FROM_WIN32(GetLastError());

			// 2. チャンクサイズ (4バイト) を読み込む
			if (FALSE == ReadFile(hFile, &dwChunkSize, sizeof(DWORD), &dwRead, NULL))
				return HRESULT_FROM_WIN32(GetLastError());

			// 3. チャンクのデータ開始位置を記録
			dwChunkDataPosition = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

			// 4. 目的のチャンクが見つかったかチェック
			if (dwChunkType == fourcc)
			{
				return S_OK;
			}

			// 5. 見つからなかった場合、次のチャンクへスキップ
			// ファイルポインタをチャンクデータサイズ分だけ進める
			LONG lOffset = dwChunkSize;
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, lOffset, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		// ループを抜けるのはファイル終端に達した時のみ (ReadFileがFALSEを返す)
		return E_FAIL;
	}

	/**
	 * [HRESULT - ReadChunkData]
	 * @brief	ファイルから指定されたバイト数のデータを読み込むヘルパー関数。
	 */
	HRESULT ReadChunkData(HANDLE hFile, void* pBuffer, DWORD dwBuffersize, DWORD dwOffset)
	{
		HRESULT hr = S_OK;
		if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN))
			return HRESULT_FROM_WIN32(GetLastError());
		if (FALSE == ReadFile(hFile, pBuffer, dwBuffersize, &dwBuffersize, NULL))
			return HRESULT_FROM_WIN32(GetLastError());
		return S_OK;
	}

	/**
	 * [bool - Initialize]
	 * @brief	XAudio2エンジンとマスターボイスを初期化する。
	 *
	 * @return	true: 成功, false: 失敗
	 */
	bool SoundEngine::Initialize()
	{
		// COMの初期化（WAVファイル読み込みで必要となる可能性があるため）
		// XAudio2自体はCOM初期化なしで動作することが多いが、念のため
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(hr))
		{
			std::cerr << "Error: CoInitializeEx failed. HRESULT = " << std::hex << hr << std::endl;
			// 致命的ではないとして続行する選択肢もあるが、ここでは失敗とする
			// ただし、既に初期化済み (RPC_E_TOO_LATE) の場合は成功とする
			if (hr != RPC_E_TOO_LATE)
			{
				return false;
			}
		}

		// 1. XAudio2エンジンの作成
		// XAUDIO2_DEBUG_ENGINEをデバッグビルド時のみ使用すると良い
		hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
		if (FAILED(hr))
		{
			std::cerr << "Error: XAudio2Create failed. HRESULT = " << std::hex << hr << std::endl;
			return false;
		}

#ifdef _DEBUG
		// デバッグビルド時のみ、XAudio2のデバッグ出力を有効化する
		XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
		debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
		m_pXAudio2->SetDebugConfiguration(&debugConfig, 0);
#endif

		// 2. マスターボイスの作成 (オーディオ出力先)
		hr = m_pXAudio2->CreateMasteringVoice(&m_pMasteringVoice);
		if (FAILED(hr))
		{
			std::cerr << "Error: CreateMasteringVoice failed. HRESULT = " << std::hex << hr << std::endl;
			m_pXAudio2->Release();
			m_pXAudio2 = nullptr;
			return false;
		}

		std::cout << "SoundEngine initialized successfully." << std::endl;
		return true;
	}

	/**
	 * [void - Terminate]
	 * @brief	XAudio2エンジンを終了し、リソースを解放する。
	 */
	void SoundEngine::Terminate()
	{
		if (m_pMasteringVoice)
		{
			// マスターボイスは、XAudio2オブジェクトが解放されると自動的に解放されるため、明示的なReleaseは通常不要
			// ただし、ここでは念のためNULL化
			m_pMasteringVoice = nullptr;
		}

		if (m_pXAudio2)
		{
			// IXAudio2のReleaseを呼ぶと、それに紐づく全ボイスも破棄される
			m_pXAudio2->Release();
			m_pXAudio2 = nullptr;
			std::cout << "SoundEngine terminated." << std::endl;
		}

		// COMの終了処理
		CoUninitialize();
	}

	/**
	 * [bool - LoadWavFile]
	 * @brief	WAVファイルを読み込み、LoadWavData構造体に格納する。
	 *
	 * @param	[in] filePath 読み込むWAVファイルのパス
	 * @param	[out] wavData 読み込んだデータを受け取る構造体
	 * @return	true: 成功, false: 失敗
	 */
	bool SoundEngine::LoadWavFile(const std::string& filePath, LoadWavData& wavData)
	{
		HANDLE hFile = CreateFileA(
			filePath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Error: Failed to open WAV file: " << filePath << std::endl;
			return false;
		}

		// RIFFチャンクとWAVEフォーマットの確認
		DWORD dwChunkSize;
		DWORD dwChunkPosition;
		// RIFFヘッダー
		if (FAILED(FindChunk(hFile, 'FFIR', dwChunkSize, dwChunkPosition)))
		{
			CloseHandle(hFile);
			std::cerr << "Error: RIFF chunk not found in " << filePath << std::endl;
			return false;
		}
		DWORD dwWaveType;
		if (FAILED(ReadChunkData(hFile, &dwWaveType, sizeof(DWORD), dwChunkPosition)))
		{
			CloseHandle(hFile);
			std::cerr << "Error: Could not read WAVE type from " << filePath << std::endl;
			return false;
		}
		if (dwWaveType != 'EVAW') // 'WAVE' の FOURCC
		{
			CloseHandle(hFile);
			std::cerr << "Error: File is not a valid WAVE file: " << filePath << std::endl;
			return false;
		}

		// fmtチャンクの読み込み (WAVEFORMATEXを取得)
		if (FAILED(FindChunk(hFile, ' tmf', dwChunkSize, dwChunkPosition)))
		{
			CloseHandle(hFile);
			std::cerr << "Error: 'fmt ' chunk not found in " << filePath << std::endl;
			return false;
		}
		// WAVEFORMATEXのメモリ確保と読み込み
		wavData.format = reinterpret_cast<WAVEFORMATEX*>(new BYTE[dwChunkSize]);
		if (FAILED(ReadChunkData(hFile, wavData.format, dwChunkSize, dwChunkPosition)))
		{
			wavData.Release(); // 失敗したら確保したメモリを解放
			CloseHandle(hFile);
			std::cerr << "Error: Could not read 'fmt ' chunk data from " << filePath << std::endl;
			return false;
		}

		// dataチャンクの読み込み (オーディオデータを取得)
		if (FAILED(FindChunk(hFile, 'atad', dwChunkSize, dwChunkPosition)))
		{
			wavData.Release();
			CloseHandle(hFile);
			std::cerr << "Error: 'data' chunk not found in " << filePath << std::endl;
			return false;
		}
		// オーディオデータ用のメモリ確保と読み込み
		wavData.audioBytes = dwChunkSize;
		wavData.audioData = new BYTE[dwChunkSize];
		if (FAILED(ReadChunkData(hFile, wavData.audioData, dwChunkSize, dwChunkPosition)))
		{
			wavData.Release();
			CloseHandle(hFile);
			std::cerr << "Error: Could not read 'data' chunk from " << filePath << std::endl;
			return false;
		}

		CloseHandle(hFile);
		std::cout << "WAV file loaded successfully: " << filePath << std::endl;
		return true;
	}
}