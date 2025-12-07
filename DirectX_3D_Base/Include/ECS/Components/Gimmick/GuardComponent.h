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

    // 変更: 単一のターゲットではなく、経路リストを持つ
    std::vector<DirectX::XMFLOAT3> path; // スムージング済みのワールド座標経路
    int currentPathIndex = 0;            // 現在目指している経路のインデックス

    // 経路再計算用タイマー（毎フレームA*は重いため）
    float pathRecalcTimer = 0.0f;
    static constexpr float PATH_RECALC_INTERVAL = 0.5f; // 0.5秒ごとに経路更新

    // コンストラクタで初期値を設定可能
    GuardComponent(
        float predDist = 5.0f,
        bool active = true,
        float delay = 0.0f,
        float spd = 4.0f,
        float vRange = 5.0f,
        float vAngle = 60.0f
    )
        : predictionDistance(predDist)
        , isActive(active)
        , delayBeforeChase(delay)
        , elapsedTime(0.0f)
        , speed(spd)
        , viewRange(vRange)
        , viewAngle(vAngle)
        , currentPathIndex(0)
        , pathRecalcTimer(0.0f)
    {
    }
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(GuardComponent)

#endif // !___GUARD_COMPONENT_H___