#include "ECS/Systems/Core/ResultControlSystem.h"
#include "ECS/Systems/Core/GameControlSystem.h"
#include "ECS//Systems/Core/ResultControlSystem.h"
#include "Scene/SceneManager.h"
#include "ECS/ECSInitializer.h"
#include "ECS/Systems/UI/UIInputSystem.h"
#include "ECS//EntityFactory.h"

using namespace std;


void ResultControlSystem::Update(float deltaTime)
{
	m_timer += deltaTime;

	// --- アニメーション制御 ---

	// 1. 星のアニメーション (0.5秒間隔で出現)
	// Tag: "AnimStar" を持つエンティティを順次表示
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

				// バウンス演出 (出現から0.5秒間で 0 -> 1.2 -> 1.0)
				float t = (m_timer - appearTime) * 3.0f; // 速度調整
				float scale = 0.0f;

				if (t < 1.0f) scale = t; // 線形拡大
				else if (t < 1.5f) scale = 1.0f + (1.5f - t) * 0.4f; // ぽよん (1.2)
				else scale = 1.0f;

				trans.scale = { 100.0f * scale, 100.0f * scale, 1.0f };
			}
			starIndex++;
		}
	}

	// 2. スタンプのアニメーション (全星出現後の 2.0s 頃)
	for (auto const& entity : m_coordinator->GetActiveEntities())
	{
		if (m_coordinator->HasComponent<TagComponent>(entity) &&
			m_coordinator->GetComponent<TagComponent>(entity).tag == "AnimStamp")
		{
			float appearTime = 2.0f;
			if (m_timer >= appearTime)
			{
				auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
				auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);

				float t = (m_timer - appearTime) * 5.0f; // 高速

				// 巨大な状態から縮小してドン！
				float scale = 1.0f;
				float alpha = 1.0f;

				if (t < 1.0f) {
					scale = 5.0f - 4.0f * t; // 5 -> 1
					alpha = t;
				}
				else {
					scale = 1.0f;
					alpha = 1.0f;
				}

				trans.scale = { 500.0f * scale, 500.0f * scale, 1.0f }; // 元サイズ基準
				ui.color.w = alpha;
			}
		}
	}
}
