//作成:2026.1.04
//福留広明

#ifndef ___TELEPORT_COMPONENT_H___
#define ___TELEPORT_COMPONENT_H___

#include "ECS/ECS.h"

/**
 * @struct TeleportComponent
 * @brief テレポートの送り先情報を保持する
 */
struct TeleportComponent {
    ECS::EntityID targetEntity = ECS::INVALID_ENTITY_ID; // 対になるテレポート先
    float coolDownTimer = 0.0f;                         // 連続移動防止用
    static constexpr float COOLDOWN_MAX = 3.0f;         // 秒
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TeleportComponent)

#endif // !___TELEPORT_COMPONENT_H___