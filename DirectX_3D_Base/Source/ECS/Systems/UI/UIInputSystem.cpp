/*****************************************************************//**
 * @file	UIInputSystem.cpp
 * @brief	UIINputSystemの実装
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author Oda Kaito
 * ------------------------------------------------------------
 *
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 *
 *
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/UI/UIInputSystem.h"
#include "Systems/Input.h"
#include "Main.h" // SCREEN_WIDTH, SCREEN_HEIGHTが必要なため

using namespace ECS;

void UIInputSystem::Update(float deltaTime)
{
	if (!m_coordinator) return;

	// 1. カーソル Entity を取得
	EntityID cursorEntity = FindFirstEntityWithComponent<UICursorComponent>(m_coordinator);
	if (cursorEntity == INVALID_ENTITY_ID) return;

	auto& cursorComp = m_coordinator->GetComponent<UICursorComponent>(cursorEntity);
	auto& cursorTrans = m_coordinator->GetComponent<TransformComponent>(cursorEntity);

	// m_entities を直接ループせず、一時的なベクターにコピーする
	std::vector<EntityID> entitiesToCheck(m_entities.begin(), m_entities.end());

	for (const auto& entity : entitiesToCheck)
	{
		// 念のため、処理する瞬間にそのエンティティがまだ生存しているか確認
		// (DestroyEntityされると Signature がリセットされるため、Component取得前にチェック)
		if (m_coordinator->GetActiveEntities().find(entity) == m_coordinator->GetActiveEntities().end()) continue;
		if (!m_coordinator->HasComponent<UIButtonComponent>(entity)) continue;

		auto& button = m_coordinator->GetComponent<UIButtonComponent>(entity);
		const auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

		if (!button.isVisible) continue;

		// 当たり判定
		if (IsOverlapping(cursorTrans.position, cursorTrans.scale, trans.position, trans.scale))
		{
			// 重なっている
			if (cursorComp.isTriggered)
			{
				button.state = ButtonState::Pressed;
				// コールバック実行 (ここで DestroyEntity されても entitiesToCheck は壊れない)
				if (button.onClick) button.onClick();
			}
			else
			{
				button.state = ButtonState::Hover;
			}
		}
		else
		{
			button.state = ButtonState::Normal;
		}
	}
}

bool UIInputSystem::IsOverlapping(const DirectX::XMFLOAT3& posA, const DirectX::XMFLOAT3& scaleA, const DirectX::XMFLOAT3& posB, const DirectX::XMFLOAT3& scaleB)
{
	// 簡易的なAABB判定 (原点が左上か中心かによりますが、ここでは中心基準または左上基準で相対的に判定)
	// UIのTransformが「左上座標」かつ「Scaleがサイズ(幅・高さ)」である場合を想定した実装例:

	/*
	// 左上基準の場合:
	float leftA = posA.x;
	float rightA = posA.x + scaleA.x;
	float topA = posA.y;
	float bottomA = posA.y + scaleA.y;

	float leftB = posB.x;
	float rightB = posB.x + scaleB.x;
	float topB = posB.y;
	float bottomB = posB.y + scaleB.y;
	*/

	// 中心基準の場合（スプライト描画の実装に合わせる）:
	// ここではSpriteクラスが中心基準か左上基準かによりますが、一般的な「中心座標±半径」で判定します
	float halfWA = scaleA.x * 0.5f;
	float halfHA = scaleA.y * 0.5f;
	float halfWB = scaleB.x * 0.5f;
	float halfHB = scaleB.y * 0.5f;

	// X軸の重なり
	bool xOverlap = std::abs(posA.x - posB.x) < (halfWA + halfWB);
	// Y軸の重なり
	bool yOverlap = std::abs(posA.y - posB.y) < (halfHA + halfHB);

	return xOverlap && yOverlap;
}
