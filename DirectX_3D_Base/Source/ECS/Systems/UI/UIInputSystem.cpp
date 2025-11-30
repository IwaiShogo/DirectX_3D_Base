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
#include "ECS/Components/Core/TransformComponent.h"

#include "ECS/Components/Core/TagComponent.h"

#include "ECS/Coordinator.h"

#include "Systems/Input.h"
#include "Main.h" // SCREEN_WIDTH, SCREEN_HEIGHTが必要なため
#include <algorithm>

using namespace DirectX;

static float Lerp(float start, float end, float t) {
	return start + (end - start) * t;
}

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
        auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
        auto& interactable = m_coordinator->GetComponent<UIInteractableComponent>(entity);

        // A. 遷移演出中(拡大中)の特別処理
        if (interactable.isTransitionExpanding)
        {
            float expandSpeed = 150.0f;
            transform.scale.x += expandSpeed * deltaTime;
            transform.scale.y += expandSpeed * deltaTime;
            transform.position.z = -4.0f; // 最前面へ
            transform.position.x *= 0.9f; // 中心へ
            transform.position.y *= 0.9f;
            continue; // 演出中はクリック判定などをスキップ
        }

        // B. 当たり判定
        float w = (interactable.width < 0.0f) ? transform.scale.x : interactable.width;
        float h = (interactable.height < 0.0f) ? transform.scale.y : interactable.height;
        float halfW = w / 2.0f;
        float halfH = h / 2.0f;

        bool hit = (ndcMouseX >= transform.position.x - halfW && ndcMouseX <= transform.position.x + halfW) &&
            (ndcMouseY >= transform.position.y - halfH && ndcMouseY <= transform.position.y + halfH);

        interactable.isHovered = hit;
        interactable.isClicked = (hit && isTrigger);
        interactable.isPressed = (hit && isPressed);

        // C. コントローラー対応 (Tagによる上書き)
        if (m_coordinator->HasComponent<TagComponent>(entity))
        {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity);
            if (tag.tag == "SelectSceneUIA" && (IsKeyTrigger('A') || IsButtonTriggered(BUTTON_A))) interactable.isClicked = true;
            if (tag.tag == "SelectSceneUIB" && (IsKeyTrigger('B') || IsButtonTriggered(BUTTON_B))) interactable.isClicked = true;
        }

        // D. ふわっと拡大アニメーション (Systemで実行！)
        if (interactable.doHoverAnim)
        {
            // 目標サイズ：ホバー中は1.2倍
            float targetScaleX = interactable.baseScaleX * (interactable.isHovered ? 1.2f : 1.0f);
            float targetScaleY = interactable.baseScaleY * (interactable.isHovered ? 1.2f : 1.0f);

            // 補間処理
            float speed = 10.0f * deltaTime;
            transform.scale.x = Lerp(transform.scale.x, targetScaleX, speed);
            transform.scale.y = Lerp(transform.scale.y, targetScaleY, speed);
        }
    }
}