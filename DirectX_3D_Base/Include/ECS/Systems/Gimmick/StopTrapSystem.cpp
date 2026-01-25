#include "ECS/Systems/Gimmick/StopTrapSystem.h"
// ===== インクルード =====
#include "ECS/Components/Gimmick/StopTrapComponent.h"
#include "ECS/Components/Gimmick/GuardComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Rendering/RenderComponent.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <cmath>
#include <vector>
#include <set>

using namespace ECS;
using namespace DirectX;

// ヘルパー: 2点間の距離の二乗を計算 (XZ平面のみ)
static float GetDistanceSqXZ(const XMFLOAT3& p1, const XMFLOAT3& p2)
{
    float dx = p1.x - p2.x;
    float dz = p1.z - p2.z;
    return dx * dx + dz * dz;
}

void StopTrapSystem::Init(ECS::Coordinator* coordinator)
{
    m_coordinator = coordinator;
}

void StopTrapSystem::Update(float deltaTime)
{

    EntityID stateEntity = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (stateEntity == INVALID_ENTITY_ID) return; // まだゲーム状態がない場合は何もしない

    const GameStateComponent& gameState = m_coordinator->GetComponent<GameStateComponent>(stateEntity);
    bool isScoutingMode = (gameState.currentMode == GameMode::SCOUTING_MODE);

 
    // 2. 警備員リストの取得
    std::vector<EntityID> guards;
    if (!isScoutingMode && !m_entities.empty())
    {
        const std::set<EntityID>& activeEntities = m_coordinator->GetActiveEntities();

        for (EntityID entity : activeEntities)
        {
            if (m_coordinator->HasComponent<GuardComponent>(entity))
            {
                guards.push_back(entity);
            }
        }
    }

    // 3. 全トラップエンティティを処理
    for (const auto& trapEntity : m_entities)
    {
        StopTrapComponent& trapComp = m_coordinator->GetComponent<StopTrapComponent>(trapEntity);
        TransformComponent& trapTrans = m_coordinator->GetComponent<TransformComponent>(trapEntity);
        RenderComponent& trapRender = m_coordinator->GetComponent<RenderComponent>(trapEntity);

        // ---------------------------------------------------------
        // ■ 1. 表示制御
        // ---------------------------------------------------------
        if (trapComp.isConsumed)
        {
            trapRender.type = MESH_NONE;
        }
        else
        {
            if (isScoutingMode)
            {
                trapRender.type = MESH_NONE;
            }
            else
            {
                trapRender.type = MESH_NONE;
            }
        }

        // ---------------------------------------------------------
        // ■ 2. 当たり判定ロジック (アクションモード かつ 未使用時のみ)
        // ---------------------------------------------------------
        if (!isScoutingMode && !trapComp.isConsumed)
        {
            for (EntityID guardEntity : guards)
            {
                GuardComponent& guardComp = m_coordinator->GetComponent<GuardComponent>(guardEntity);
                TransformComponent& guardTrans = m_coordinator->GetComponent<TransformComponent>(guardEntity);

                // 既にスタン中ならスキップ
                if (guardComp.isStunned) continue;

                // 距離判定
                float hitRadius = trapComp.range + 0.5f;
                float distSq = GetDistanceSqXZ(trapTrans.position, guardTrans.position);

                if (distSq <= hitRadius * hitRadius)
                {
                    // === ヒット時の処理 ===

                    // 1. 警備員をスタン
                    guardComp.isStunned = true;
                    guardComp.stunTimer = trapComp.stopDuration;

                 // 2. エフェクト再生
                    // trapComp.effectID ではなく直接 "EFK_TASER" を指定
                    EntityFactory::CreateOneShotEffect(
                        m_coordinator,
                        "EFK_TASER",
                        guardTrans.position,
                        trapComp.stopDuration,
                        1.0f
                    );

                    // 3. SE再生
                    // trapComp.soundID ではなく直接 "SE_TASER" を指定
                    EntityFactory::CreateOneShotSoundEntity(
                        m_coordinator,
                        "SE_TASER",
                        1.0f
                    );

                   
                    // 4. 使用済みにする
                    trapComp.isConsumed = true;

                    break;
                }
            }
        }
    }
}