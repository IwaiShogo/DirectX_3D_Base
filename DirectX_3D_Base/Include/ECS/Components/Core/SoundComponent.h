/*****************************************************************//**
 * @file	SoundComponent.h
 * @brief	サウンド再生に必要な情報を保持するコンポーネント
 * 
 * @details	
 * 再生すべきサウンドのID、状態、制御パラメータを定義。
 * AudioSystemによって処理されます。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：SoundComponent構造体の定義
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SOUND_COMPONENT_H___
#define ___SOUND_COMPONENT_H___

// ===== インクルード =====
#include <string>
#include <xaudio2.h>

/**
 * @enum    SoundType
 * @brief   サウンドの再生カテゴリを定義
 */
enum class SoundType
{
    SE,     // 効果音（通常はループなし）
    BGM,    // 背景音楽（通常は無限ループ）
    Ambient // 環境音
};

/**
 * @struct  SoundComponent
 * @brief   エンティティにサウンド再生能力を与えるコンポーネント。
 */
struct SoundComponent
{
    // ----------------------------------------
    // 再生指示データ
    // ----------------------------------------
    std::string assetID = "";       // AssetManagerに登録されたサウンドID
    SoundType type = SoundType::SE; // サウンドの種類（SE/BGM/Ambient）
    float volume = 1.0f;            // 再生ボリューム（0.0f - 1.0f)
    UINT32 loopCount = 0;           // ループ回数（0: 一回再生, XAUDIO2_LOOP_INFINITE: 無限ループ）
    
    // ----------------------------------------
    // 実行時データ/状態
    // ----------------------------------------
    bool isPlaying = false;     // 現在再生中か
    bool playRequested = false; // 再生が要求されたか
    bool stopRequested = false; // 停止が要求されたか

    // ----------------------------------------
    // コンストラクタ
    // ----------------------------------------
    SoundComponent() = default;

    /**
     * [void - SoundComponent]
     * @brief	初期再生設定を行うコンストラクタ
     * 
     * @param	[in] id アセットID
     * @param	[in] t サウンドタイプ
     * @param	[in] vol ボリューム
     * @param	[in] loop ループ回数
     * @note	（省略可）
     */
    SoundComponent(const std::string& id, SoundType t = SoundType::SE, float vol = 1.0f, UINT32 loop = 0)
        : assetID(id)
        , type(t)
        , volume(vol)
        , loopCount(loop)
    {}

    // ----------------------------------------
    // ヘルパーメソッド
    // ----------------------------------------
    void RequestPlay(float vol = -1.0f, UINT32 loop = 0)
    {
        // ボリュームが指定されていなければ既存のボリュームを維持
        if (vol >= 0.0f)
        {
            volume = vol;
        }
        loopCount = loop;
        playRequested = true;
        stopRequested = false;
    }

    void RequestStop()
    {
        stopRequested = true;
        playRequested = false;
    }

    /**
     * [void - RequestBGM]
     * @brief	BGMとして再生を要求する
     */
    void RequestBGM(float vol = 1.0f)
    {
        type = SoundType::BGM;
        RequestPlay(vol, XAUDIO2_LOOP_INFINITE);
    }
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(SoundComponent)

#endif // !___SOUND_COMPONENT_H___