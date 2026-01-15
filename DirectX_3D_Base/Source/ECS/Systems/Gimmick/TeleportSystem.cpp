#include "TeleportSystem.h"
#include "ECS/EntityFactory.h"
#include "ECS/Components/Gimmick/TeleportComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Gameplay/PlayerControlComponent.h"

using namespace DirectX;
using namespace ECS;

void TeleportSystem::Init(Coordinator* coordinator) {
    m_coordinator = coordinator;
}

void TeleportSystem::Update(float deltaTime) {
    if (!m_coordinator) return;

    // ここで型が完全に定義されているため、FindFirst... が正常に動作します
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;

    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    for (auto const& entity : m_entities) {
        auto& tp = m_coordinator->GetComponent<TeleportComponent>(entity);
        auto& tTrans = m_coordinator->GetComponent<TransformComponent>(entity);

        if (tp.coolDownTimer > 0.0f) {
            tp.coolDownTimer -= deltaTime;
            continue;
        }

        XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
        XMVECTOR tPos = XMLoadFloat3(&tTrans.position);
        float distSq = XMVectorGetX(XMVector3LengthSq(pPos - tPos));

        // 距離判定を少し広げる（2.0m程度）
        if (distSq < 2.0f * 2.0f) {
            if (tp.targetEntity != INVALID_ENTITY_ID) {
                auto& destTrans = m_coordinator->GetComponent<TransformComponent>(tp.targetEntity);
                auto& destTp = m_coordinator->GetComponent<TeleportComponent>(tp.targetEntity);

                // 座標移動
                pTrans.position = destTrans.position;
                pTrans.position.y += 0.8f; // ★少し高めに浮かせて埋まりを確実に防ぐ

                // クールダウン（TeleportComponent.h で 1.5f に設定されています）
                tp.coolDownTimer = TeleportComponent::COOLDOWN_MAX;
                destTp.coolDownTimer = TeleportComponent::COOLDOWN_MAX;

                // デバッグログを出して動作を確認
                printf("[System] Teleported to Entity: %d\n", tp.targetEntity);
            }
        }
    }
}