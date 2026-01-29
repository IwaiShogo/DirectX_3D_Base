/*****************************************************************//**
 * @file	EnemySpawnSystem.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/12/11	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/Gameplay/EnemySpawnSystem.h"
#include "ECS/EntityFactory.h"
#include <vector>

using namespace ECS;
using namespace DirectX;

void EnemySpawnSystem::Update(float deltaTime)
{
    if (m_coordinator)
    {
        ECS::EntityID stateID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
        if (stateID != ECS::INVALID_ENTITY_ID)
        {
            if (m_coordinator->GetComponent<GameStateComponent>(stateID).isPaused) return;
        }
    }

    // TPSモード＆プレイ中チェック (これが重要！)
    ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID != ECS::INVALID_ENTITY_ID)
    {
        auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
        bool isActionMode = (state.currentMode == GameMode::ACTION_MODE);
        bool isPlaying = (state.sequenceState == GameSequenceState::Playing);

        // まだカウントダウン開始条件を満たしていないなら何もしない
        if (!isActionMode || !isPlaying) return;
    }

    std::vector<EntityID> spawnersToDestroy;

    for (auto const& entity : m_entities)
    {
        auto& spawn = m_coordinator->GetComponent<EnemySpawnComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

        spawn.timer -= deltaTime;

        // 1. 予兆エフェクト (例: 地面に赤いリング)
        if (!spawn.effectPlayed && spawn.timer <= spawn.effectTiming)
        {
            spawn.effectPlayed = true;
            // 以前と同じ予兆エフェクトと音
            EntityFactory::CreateOneShotEffect(m_coordinator, "UI_SONAR", trans.position, spawn.timer);
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_CHARGE");
        }

        // 2. スポーン実行 (タイマー0)
        if (spawn.timer <= 0.0f)
        {
            // ★ここで本物の警備員エンティティを生成！
            if (spawn.type == EnemyType::Guard)
            {
                EntityFactory::CreateGuard(m_coordinator, trans.position);
            }

            // 出現エフェクト
            EntityFactory::CreateOneShotEffect(m_coordinator, "EFK_SPAWN", trans.position, 2.0f);
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_SPAWN");

            // 役目を終えたスポーナーは削除リストへ
            spawnersToDestroy.push_back(entity);
        }
    }

    // 使い終わったスポーナーを消去
    for (auto entity : spawnersToDestroy)
    {
        m_coordinator->DestroyEntity(entity);
    }
}