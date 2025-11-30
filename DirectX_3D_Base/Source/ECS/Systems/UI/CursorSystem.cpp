#include "ECS/Systems/UI/CursorSystem.h" 
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Core/TagComponent.h"
#include "ECS/Coordinator.h"
#include "Main.h" 
#include <Windows.h>

void CursorSystem::Init(ECS::Coordinator* coordinator)
{
    m_coordinator = coordinator;
}

void CursorSystem::Update(float deltaTime)
{
    if (!m_coordinator) return;

    // 1. マウス座標取得 (Windows API)
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(GetActiveWindow(), &p);

    // 2. 座標変換 (ピクセル → NDC -1.0~1.0)
    // 画面サイズ定数 SCREEN_WIDTH / SCREEN_HEIGHT が Main.h にある前提
    float ndcX = (static_cast<float>(p.x) / SCREEN_WIDTH) * 2.0f - 1.0f;
    float ndcY = -((static_cast<float>(p.y) / SCREEN_HEIGHT) * 2.0f - 1.0f);

    // 3. "Cursor" タグを持つエンティティを探して移動させる
    for (auto const& entity : m_entities)
    {
        // TagComponentを持っているか確認
        if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity);

            // "Cursor" タグがついているものだけ動かす
            if (tag.tag == "Cursor") {
                auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
                transform.position.x = ndcX;
                transform.position.y = ndcY;
            }
        }
    }
}