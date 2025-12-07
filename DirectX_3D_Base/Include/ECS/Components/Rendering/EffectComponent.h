/*****************************************************************//**
 * @file	EffectComponent.h
 * @brief	エフェクト
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/07	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___EFFECT_COMPONENT_H___
#define ___EFFECT_COMPONENT_H___

#include <string>
#include <DirectXMath.h>
#include <Effekseer/Effekseer.h> // Handle型のために必要

struct EffectComponent
{
    std::string assetID;            // 再生するエフェクトのアセットID
    Effekseer::Handle handle = 0;   // 再生中のハンドル (0なら未再生/終了)

    bool playOnAwake;               // 生成時に再生するか
    bool isLooping;                 // ループ再生するか（Effekseerデータ側で設定されている場合はそれに従うが、再トリガー用）
    DirectX::XMFLOAT3 offset;       // エンティティからの位置オフセット
    float scale;                    // スケール倍率

    // 制御フラグ
    bool requestPlay = false;
    bool requestStop = false;

    EffectComponent(std::string id = "", bool loop = false, bool autoPlay = true, DirectX::XMFLOAT3 off = { 0,0,0 }, float scl = 1.0f)
        : assetID(id)
        , handle(0)
        , playOnAwake(autoPlay)
        , isLooping(loop)
        , offset(off)
        , scale(scl)
    {
        if (autoPlay) requestPlay = true;
    }

    void Play() { requestPlay = true; }
    void Stop() { requestStop = true; }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(EffectComponent)

#endif // !___EFFECT_COMPONENT_H___