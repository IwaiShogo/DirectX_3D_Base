#include "ECS/Systems/Core/ResultControlSystem.h"
#include "ECS/ECS.h"

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/Core/TagComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>

#include <cmath>
#include <cstdlib>
#include <string>

using namespace ECS;

static bool StartsWith(const std::string& s, const char* prefix)
{
    return s.rfind(prefix, 0) == 0;
}

void ResultControlSystem::Update(float deltaTime)
{
    if (!m_coordinator) return;

    m_timer += deltaTime;

    // ---------------------------------------------------------
    // 1. 星のアニメーション (0.5秒間隔で 1つずつポップ)
    //    Tag: "AnimStar0/1/2"
    // ---------------------------------------------------------
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

        const auto& tagComp = m_coordinator->GetComponent<TagComponent>(entity);
        const std::string& tag = tagComp.tag;

        if (!StartsWith(tag, "AnimStar")) continue;

        // "AnimStar0" の末尾数字を行番号として使う（無ければ0扱い）
        int row = 0;
        if (tag.size() > 7)
        {
            row = std::atoi(tag.c_str() + 7); // 7 = strlen("AnimStar")
            if (row < 0) row = 0;
            if (row > 2) row = 2;
        }

        float appearTime = 0.5f + row * 0.5f;
        if (m_timer < appearTime) continue;

        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

        // 初回だけ「最終サイズ」を保存し、0開始に落とす
        auto it = m_starTargetScale.find(entity);
        if (it == m_starTargetScale.end())
        {
            m_starTargetScale[entity] = trans.scale; // ResultSceneで入れた最終サイズ
            it = m_starTargetScale.find(entity);

            trans.scale = { 0.0f, 0.0f, it->second.z };
        }

        // 出現からの経過時間でスケールを 0 -> 1.2 -> 1.0 に変化
        float t = (m_timer - appearTime) * 3.0f;
        float pop = 0.0f;

        if (t < 1.0f)            pop = t;
        else if (t < 1.5f)       pop = 1.0f + (1.5f - t) * 0.4f;
        else                     pop = 1.0f;

        const auto& target = it->second;
        trans.scale = { target.x * pop, target.y * pop, target.z };
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

            float t = (m_timer - appearTime) * 2.0f;
            float scale = 0.0f;

            if (t < 1.0f)            scale = t;
            else if (t < 1.5f)       scale = 1.0f + (1.5f - t) * 0.3f;
            else                     scale = 1.0f;

            float alpha = (t < 1.0f) ? t : 1.0f;

            trans.scale = { 140.0f * scale, 140.0f * scale, 1.0f };
            ui.color.w = alpha;
        }
    }

    // ---------------------------------------------------------
    // 3. ★を取った行の STAR_TEXT を波打たせる
    //    Tag: "AnimStarText"
    // ---------------------------------------------------------
    {
        const float waveStartTime = 0.8f;

        if (m_timer >= waveStartTime)
        {
            float t = m_timer - waveStartTime;
            float waveScale = 1.0f + 0.05f * std::sin(t * 6.0f);

            for (auto const& entity : m_coordinator->GetActiveEntities())
            {
                if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

                const auto& tag = m_coordinator->GetComponent<TagComponent>(entity);
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
