#ifndef ___PLAYER_CONTROL_COMPONENT_H___
#define ___PLAYER_CONTROL_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include "ECS/ECS.h"        // もしくは "ECS/Types.h" など、プロジェクトに合わせて

// プレイヤー用アニメーション状態
enum class PlayerAnimState
{
    Idle,   // その場で待機
    Run,    // 走り
};

// プレイヤー操作コンポーネント
struct PlayerControlComponent
{
    float moveSpeed = 5.0f;             ///< プレイヤーの移動速度

    // カメラ関連
    ECS::EntityID attachedCameraID = 0; ///< プレイヤーに追従するカメラEntityID

    // アクション関連（Bボタン）
    bool isItemStealTriggered = false;  ///< アイテム窃盗アクションがトリガーされたか

    // 現在のアニメーション状態
    PlayerAnimState animState = PlayerAnimState::Idle;

    PlayerControlComponent(float speed = 5.0f)
        : moveSpeed(speed)
    {
    }
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(PlayerControlComponent)

#endif // !___PLAYER_CONTROL_COMPONENT_H___
