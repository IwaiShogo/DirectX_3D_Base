#include "ECS/Systems/Core/ResultControlSystem.h"
#include "ECS/ECS.h"

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/Core/TagComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>

using namespace ECS;

void ResultControlSystem::Update(float deltaTime)
{
    if (!m_coordinator) return;

    m_timer += deltaTime;

    // ---------------------------------------------------------
    // 1. 星のアニメーション (0.5秒間隔で 1つずつポップ)
    //    Tag: "AnimStar"
    // ---------------------------------------------------------
    int starIndex = 0;
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (m_coordinator->HasComponent<TagComponent>(entity) &&
            m_coordinator->GetComponent<TagComponent>(entity).tag == "AnimStar")
        {
            // 出現タイミング: 0.5s, 1.0s, 1.5s ...
            float appearTime = 0.5f + starIndex * 0.5f;

            if (m_timer >= appearTime)
            {
                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                // 出現からの経過時間でスケールを 0 -> 1.2 -> 1.0 に変化
                float t = (m_timer - appearTime) * 3.0f;
                float scale = 0.0f;

                if (t < 1.0f)  scale = t;
                else if (t < 1.5f)  scale = 1.0f + (1.5f - t) * 0.4f;
                else                scale = 1.0f;

                trans.scale = { 50.0f * scale, 50.0f * scale, 1.0f };
            }

            ++starIndex;
        }
    }

    // ---------------------------------------------------------
    // 2. スタンプのアニメーション (全星出現後の 2.0秒あたりでドン！)
    //    Tag: "AnimStamp"
    // ---------------------------------------------------------
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (m_coordinator->HasComponent<TagComponent>(entity) &&
            m_coordinator->GetComponent<TagComponent>(entity).tag == "AnimStamp")
        {
            float appearTime = 2.0f;
            if (m_timer < appearTime) continue;

            auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
            auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);

            float t = (m_timer - appearTime) * 5.0f;

            // 変数定義
            float scale = 1.0f;
            float alpha = 1.0f;

            // ★修正: ラジアンへの変換係数 (3.14... / 180)
            const float TO_RAD = 3.141592f / 180.0f;


            if (t < 1.0f)
            {
                scale = 5.0f - 4.0f * t; // 5 -> 1
                alpha = t;               // 0 -> 1
        
            }
            else
            {
                scale = 1.0f;
                alpha = 1.0f;
				
            }


            trans.scale = { 150.0f * scale, 150.0f * scale, 1.0f };
            trans.rotation.z = -30.0f * TO_RAD;
            ui.color.w = alpha;
        }
    }

    // 3) ★を取った行の STAR_TEXT を波打たせる
    {
        const float waveStartTime = 0.8f;

        if (m_timer >= waveStartTime)
        {
            float t = m_timer - waveStartTime;
            float waveScale = 1.0f + 0.05f * std::sin(t * 6.0f);

            for (auto const& entity : m_coordinator->GetActiveEntities())
            {
                if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

                auto& tag = m_coordinator->GetComponent<TagComponent>(entity);
                if (tag.tag != "AnimStarText") continue;

                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                const float baseW = 320.0f;
                const float baseH = 60.0f;

                trans.scale.x = baseW * waveScale;
                trans.scale.y = baseH * waveScale;
            }
        }
    }

}
