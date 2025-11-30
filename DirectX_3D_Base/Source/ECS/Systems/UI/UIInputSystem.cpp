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

#include "ECS/Components/UI/UIInteractableComponent.h"
#include  "ECS/Components/Core/TransformComponent.h"

#include "ECS/Coordinator.h"

#include "Systems/Input.h"
#include "Main.h" // SCREEN_WIDTH, SCREEN_HEIGHTが必要なため

using namespace DirectX;


void UIInputSystem::Update(float deltaTime)
{

	if (!m_coordinator) return; //m_coordinatorが初期化されていなかったら何もしない 

	// マウス情報の取得
	XMFLOAT2 mousePos = GetMousePosition();

	// クリック判定(押された瞬間)の簡易ロジック
	static bool wasPressed = false;
	bool isPressed = IsMousePress(0); // 左クリック
	bool isTrigger = isPressed && !wasPressed;
	wasPressed = isPressed;

	// マウス座標の正規化(0.0～1.0に変更)
	float ndcMouseX = (mousePos.x / (float)SCREEN_WIDTH) * 2.0f - 1.0f;
	float ndcMouseY = 1.0f - (mousePos.y / (float)SCREEN_HEIGHT) * 2.0f;


	// 全エンティティに対して判定
	for (auto const& entity : m_entities)
	{
		// Coordinator経由でコンポーネントを取得
		auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		auto& interactable = m_coordinator->GetComponent<UIInteractableComponent>(entity);

		// 判定サイズの決定(width/heightがマイナスならScaleをつかう)
		float w = (interactable.width < 0.0f) ? transform.scale.x : interactable.width;
		float h = (interactable.height < 0.0f) ? transform.scale.y : interactable.height;

		// AABB当たり判定(左上基準と仮定)
		// transform.position が 左上座標の場合

		float halfW = w / 2.0f;
		float halfH = h / 2.0f;

		bool hit = (ndcMouseX >= transform.position.x - halfW && ndcMouseX <= transform.position.x + halfW) &&
			(ndcMouseY >= transform.position.y - halfH && ndcMouseY <= transform.position.y + halfH);


		// 状態更新
		interactable.isHovered = hit;
		interactable.isClicked = (hit && isTrigger);
		interactable.isPressed = (hit && isPressed);

	}
}