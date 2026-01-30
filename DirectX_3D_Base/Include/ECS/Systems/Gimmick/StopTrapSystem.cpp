#include "ECS/Systems/Gimmick/StopTrapSystem.h"
#include "ECS/Components/Gimmick/StopTrapComponent.h"
#include "ECS/Components/Gimmick/GuardComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Rendering/RenderComponent.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "ECS/Components/Core/SoundComponent.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <cmath>
#include <vector>
#include <set>
#include <algorithm>

using namespace ECS;
using namespace DirectX;

// XMFLOAT3線形補間
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
    if (stateEntity == INVALID_ENTITY_ID) return;
    const GameStateComponent& gameState = m_coordinator->GetComponent<GameStateComponent>(stateEntity);
    bool isScoutingMode = (gameState.currentMode == GameMode::SCOUTING_MODE);

    // 1. 警備員リスト作成
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

    // 2. トラップ処理
    for (const auto& trapEntity : m_entities)
    {
        StopTrapComponent& trapComp = m_coordinator->GetComponent<StopTrapComponent>(trapEntity);
        TransformComponent& trapTrans = m_coordinator->GetComponent<TransformComponent>(trapEntity);
        RenderComponent& trapRender = m_coordinator->GetComponent<RenderComponent>(trapEntity);

        // -------------------------------------------------------
        // ▼▼▼ SE強制停止ロジック (ここが修正のキモです) ▼▼▼
        // -------------------------------------------------------
        if (trapComp.seEntity != INVALID_ENTITY_ID)
        {
            trapComp.seTimer += deltaTime;
            if (trapComp.seTimer >= trapComp.stopDuration)
            {
                // エンティティがまだ存在し、SoundComponentを持っているか確認
                if (m_coordinator->HasComponent<SoundComponent>(trapComp.seEntity))
                {
                    auto& sound = m_coordinator->GetComponent<SoundComponent>(trapComp.seEntity);

                    // まだ停止リクエストを出していない場合
                    if (!sound.stopRequested)
                    {
                        // 1. まず停止リクエストを送る
                        sound.RequestStop();

                        // ★重要: ここではまだDestroyEntityしない！
                        // 次のフレームでSoundSystemが停止処理を行うのを待つ
                    }
                    else
                    {
                        // 2. 既に停止リクエスト済み（=1フレーム経過）なら破棄
                        m_coordinator->DestroyEntity(trapComp.seEntity);
                        trapComp.seEntity = INVALID_ENTITY_ID;
                        trapComp.seTimer = 0.0f;
                    }
                }
                else
                {
                    // コンポーネントが無い（既に消えている）場合はIDリセット
                    trapComp.seEntity = INVALID_ENTITY_ID;
                    trapComp.seTimer = 0.0f;
                }
            }
        }
        // ▲▲▲ SE強制停止ロジック終了 ▲▲▲


        // 見た目の更新
        if (trapComp.isConsumed)
        {
            trapRender.type = MESH_NONE;
        }
        else
        {
            trapRender.type = MESH_NONE; // 常に見えない仕様であればこれでOK
        }

        // 判定処理
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

                DirectX::XMFLOAT3 effectPos = guardTrans.position;
                float effectDuration = std::max(0.1f, trapComp.stopDuration - 0.5f);

                if (distSq <= hitRadius * hitRadius)
                {
                    // 1. ガードをスタン状態にする
                    guardComp.isStunned = true;
                    guardComp.stunTimer = trapComp.stopDuration;

                    // 2. エフェクト再生
                    EntityFactory::CreateOneShotEffect(
                        m_coordinator,
                        "EFK_TASER",
                        effectPos,
                        effectDuration,
                        1.0f
                    );

                    // 3. SE再生 (duration指定なしで生成し、IDを保存)
                    trapComp.seEntity = EntityFactory::CreateOneShotSoundEntity(
                        m_coordinator,
                        "SE_TASER",
                        1.0f
                        // 第4引数(duration)は指定しない、または0.0fにしてLifeTimeComponentを付けない
                    );
                    trapComp.seTimer = 0.0f; // タイマーリセット

                    // 4. 使用済みにする
                    trapComp.isConsumed = true;
                    break;
                }
            }
        }
    }
}