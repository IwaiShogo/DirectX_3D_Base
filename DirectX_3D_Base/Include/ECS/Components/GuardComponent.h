/*****************************************************************//**
 * @file	GuardComponent.h
 * @brief	プレイヤーの位置を追跡する警備員AIのコンポーネント定義
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Fukudome Hiroaki
 * ------------------------------------------------------------
 * 
 * @date	2025/11/09	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GUARD_COMPONENT_H___
#define ___GUARD_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>

struct GuardComponent
{
    float predictionDistance;   // プレイヤーを予測して追跡する距離
    bool isActive;              // AIが有効かどうか
    float delayBeforeChase;     // 追跡開始までの遅延（秒）
    float elapsedTime;          // 経過時間（内部用）
    float speed;                // ガードの移動速度 ← 追加！

    // コンストラクタで初期値を設定可能
    GuardComponent(
        float predDist = 5.0f,
        bool active = true,
        float delay = 0.0f,
        float spd = 5.0f // ← デフォルトスピード
    )
        : predictionDistance(predDist)
        , isActive(active)
        , delayBeforeChase(delay)
        , elapsedTime(0.0f)
        , speed(spd) // ← 初期化
    {
    }
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(GuardComponent)

#endif // !___GUARD_COMPONENT_H___