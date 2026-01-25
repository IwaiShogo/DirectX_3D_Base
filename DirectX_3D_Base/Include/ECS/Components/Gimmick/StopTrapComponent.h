#ifndef ___STOP_TRAP_COMPONENT_H___
#define ___STOP_TRAP_COMPONENT_H___

#include "ECS/ECS.h" 

/**
 * @struct StopTrapComponent
 * @brief 侵入者を足止めするトラップのデータ
 */
struct StopTrapComponent
{
    // ■ パラメータ
    float range = 0.5f;          // 当たり判定半径
    float stopDuration = 3.0f;   // 停止時間
    bool isConsumed = false;     // 使用済みフラグ

    // コンストラクタ
    StopTrapComponent(float duration = 3.0f)
        : stopDuration(duration), isConsumed(false) {
    }
};

// 登録マクロ
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(StopTrapComponent)

#endif // !___STOP_TRAP_COMPONENT_H___