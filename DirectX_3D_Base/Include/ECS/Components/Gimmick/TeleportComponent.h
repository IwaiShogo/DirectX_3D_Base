//作成:2026.1.04
//福留広明

#ifndef ___TELEPORT_COMPONENT_H___
#define ___TELEPORT_COMPONENT_H___

#include "ECS/ECS.h"


/**
 * @enum TeleportState
 * @brief テレポートの進行状態
 */
enum class TeleportState {
    Idle,       // 待機中
    FadingOut,  // 暗転中（テレポート前）
    FadingIn    // 明転中（テレポート後）
};

/**
 * @struct TeleportComponent
 * @brief テレポートの送り先情報を保持する
 */
struct TeleportComponent {
    ECS::EntityID targetEntity = ECS::INVALID_ENTITY_ID; // 対になるテレポート先
    float coolDownTimer = 0.0f;                         // 連続移動防止用
    static constexpr float COOLDOWN_MAX = 3000.0f;         // 秒

    TeleportState state = TeleportState::Idle;
    float currentAlpha = 0.0f;              // 現在の不透明度 (0.0～1.0)
    static constexpr float FADE_SPEED = 4.0f; // 0.5秒で暗転 (1.0 / 0.5)

};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TeleportComponent)

#endif // !___TELEPORT_COMPONENT_H___