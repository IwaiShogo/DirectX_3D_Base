/*****************************************************************//**
 * @file	LifeTimeSystem.cpp
 * @brief	生存時間
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/08	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/Core/LifeTimeSystem.h"
#include "ECS/Components/Rendering/EffectComponent.h"
#include "ECS/Systems/Rendering/EffectSystem.h"
#include "ECS/ECSInitializer.h"


void LifeTimeSystem::Update(float deltaTime)
{
    // 削除対象を一時保存するリスト
    std::vector<ECS::EntityID> entitiesToDestroy;

    for (auto const& entity : m_entities)
    {
        auto& timer = m_coordinator->GetComponent<LifeTimeComponent>(entity);

        timer.lifeTime -= deltaTime;

        if (timer.lifeTime <= 0.0f)
        {
            entitiesToDestroy.push_back(entity);
        }
    }

    auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>();

    // 寿命が尽きたエンティティを一括削除
    for (auto const& entity : entitiesToDestroy)
    {
        if (m_coordinator->HasComponent<EffectComponent>(entity))
        {
            if (effectSystem)
            {
                effectSystem->StopEffectImmediate(entity);
            }
        }
        m_coordinator->DestroyEntity(entity);
    }
}