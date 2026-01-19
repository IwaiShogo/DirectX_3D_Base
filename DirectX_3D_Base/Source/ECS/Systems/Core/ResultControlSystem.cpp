#include "ECS/Systems/Core/ResultControlSystem.h"
#include "ECS/ECS.h"
#include "ECS/EntityFactory.h"

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
    // 2. �X�^���v�̃A�j���[�V���� (�S���o����� 2.0�b������Ńh���I)
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


            trans.scale = { 140.0f * scale, 140.0f * scale, 1.0f };
            trans.rotation.z = -30.0f * TO_RAD;
            ui.color.w = alpha;

            if (t >= 1.0f && !m_playedStampEffect)
            {
                m_playedStampEffect = true;

                std::cout << "Stamp Effect Played!" << std::endl;
               // DirectX::XMFLOAT3 effectPos = { 0.0f,0.0f,0.0f };

                // スタンプと同じ位置にエフェクトを生成
                m_coordinator->CreateEntity(
                    TransformComponent(
                        { 11.0f, 4.8f, -5.0f },   // Position: X=5(右), Y=2(上), Z=0(奥行き)
                        { 0.0f, 0.0f, 0.0f },   // Rotation
                        { 5.0f, 5.0f, 5.0f }    // Scale
                    ),
                    
                    EffectComponent(
                        "EFK_STAMP",
                        false,
                        true,
                        DirectX::XMFLOAT3{ 0,0,0 },
                        1.0f
                    )
                );
            }
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

// ---------------------------------------------------------
// 4. ���ʉ��o�F�p���p������A�j���[�V����
//    Tag: "ResultAnim"
// ---------------------------------------------------------
    {
        const float FRAME_TIME = 0.05f;

        const int COLUMNS = 5; // ��
        const int ROWS = 1; // �c
        const int FRAME_COUNT = COLUMNS * ROWS;

        int frame = static_cast<int>(m_timer / FRAME_TIME) % FRAME_COUNT;

        int col = frame % COLUMNS;
        int row = frame / COLUMNS;

        float uvW = 1.0f / COLUMNS;
        float uvH = 1.0f / ROWS;

        for (auto const& entity : m_coordinator->GetActiveEntities())
        {
            if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
            if (!m_coordinator->HasComponent<UIImageComponent>(entity)) continue;

            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity);
            if (tag.tag != "RESULT_ANIM") continue;

            auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);

            ui.uvScale = { uvW, uvH };
            ui.uvPos = { col * uvW, row * uvH };
        }
    }

// ---------------------------------------------------------
// Result �{�^���� Hover ���o
// ---------------------------------------------------------
    for (auto entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
        if (!m_coordinator->HasComponent<UIButtonComponent>(entity)) continue;
        if (!m_coordinator->HasComponent<TransformComponent>(entity)) continue;

        const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;

        if (tag != "BTN_BACK_STAGE_SELECT" &&
            tag != "BTN_RETRY" &&
            tag != "BTN_BACK_TITLE")
        {
            continue;
        }


        auto& btn = m_coordinator->GetComponent<UIButtonComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);


        // 1. �ڕW�{��
        float targetRatio =
            (btn.state == ButtonState::Hover) ? 1.08f : 1.0f;

        // 2. ���T�C�Y �~ �{��
        float targetX = btn.originalScale.x * targetRatio;
        float targetY = btn.originalScale.y * targetRatio;

        // 3. Lerp
        float speed = 15.0f * deltaTime;
        trans.scale.x += (targetX - trans.scale.x) * speed;
        trans.scale.y += (targetY - trans.scale.y) * speed;

        // ================================
        // �� Hover �ɓ������u�Ԃ� SE �Đ�
        // ================================
        if (btn.prevState != ButtonState::Hover &&
            btn.state == ButtonState::Hover)
        {
                // �{�^���ɃJ�[�\�������Ԃ������Ƃ�SE��炷
                ECS::EntityFactory::CreateOneShotSoundEntity(
                    m_coordinator,
                    "SE_CLEAR",  // SE
                    0.8f         // ����       
            );
        }
        btn.prevState = btn.state;

    }

}
