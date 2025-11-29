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
    float speed;                // ガードの移動速度

    float viewRange = 10.0f;        // 視認距離（半径）
    float viewAngle = 60.0f;        // 視野角（度数法：左右合わせて60度など）
    bool isPlayerDetected = false;  // プレイヤーを発見しているか

    // 経路探索結果の次の目標セル（グリッド座標）
    DirectX::XMFLOAT2 targetGridPos = { -1.0f, -1.0f }; // -1は無効な目標を示す

    // 現在の目標座標に到達したか
    bool isPathCalculated = false;

    // 経路探索結果の次の目標セル（グリッド座標）
    DirectX::XMINT2 nextTargetGridPos = { -1, -1 }; // -1は無効な目標を指す

    // 次のパスが計算が必要がどうかのフラグ
    bool needsPathRecalc = true;

    // コンストラクタで初期値を設定可能
    GuardComponent(
        float predDist = 5.0f,
        bool active = true,
        float delay = 0.0f,
        float spd = 5.0f, // ← デフォルトスピード
        float vRange = 5.0f,
        float vAngle = 60.0f
    )
        : predictionDistance(predDist)
        , isActive(active)
        , delayBeforeChase(delay)
        , elapsedTime(0.0f)
        , speed(spd) // ← 初期化
        , viewRange(vRange)
        , viewAngle(vAngle)
    {
    }
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(GuardComponent)

#endif // !___GUARD_COMPONENT_H___