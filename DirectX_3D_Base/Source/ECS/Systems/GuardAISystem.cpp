// GuardAISystem.cpp
#include "ECS/Systems/GuardAISystem.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

using namespace DirectX;

void GuardAISystem::Update()
{
    if (!m_coordinator) return;

    const float deltaTime = 1.0f / 60.0f; // 固定フレーム想定

    for (auto const& entity : m_entities)
    {
        auto& guardComp = m_coordinator->GetComponent<GuardComponent>(entity);
        auto& transformComp = m_coordinator->GetComponent<TransformComponent>(entity);

        if (!guardComp.isActive) continue;

        // 遅延処理
        guardComp.elapsedTime += deltaTime;
        if (guardComp.elapsedTime < guardComp.delayBeforeChase)
            continue;

        // プレイヤー取得
        ECS::EntityID playerID = ECS::FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        if (playerID == ECS::INVALID_ENTITY_ID) continue;

        auto& playerTransform = m_coordinator->GetComponent<TransformComponent>(playerID);

        XMVECTOR playerPosV = XMLoadFloat3(&playerTransform.position);
        XMVECTOR guardPosV = XMLoadFloat3(&transformComp.position);

        // 方向と距離
        XMVECTOR directionV = XMVectorSubtract(playerPosV, guardPosV);
        float distance = XMVectorGetX(XMVector3Length(directionV));
        if (distance < 0.001f) continue;

        directionV = XMVector3Normalize(directionV);

        // 速度ベースで移動
        XMVECTOR moveV = XMVectorScale(directionV, guardComp.speed * deltaTime);
        XMStoreFloat3(&transformComp.position, XMVectorAdd(guardPosV, moveV));

        // Y軸回転でプレイヤー方向を向く
        XMFLOAT3 guardPos, playerPos;
        XMStoreFloat3(&guardPos, guardPosV);
        XMStoreFloat3(&playerPos, playerPosV);

        float dx = playerPos.x - guardPos.x;
        float dz = playerPos.z - guardPos.z;
        float angleY = std::atan2(dx, dz);
        transformComp.rotation = { 0.0f, angleY, 0.0f };
    }
}

