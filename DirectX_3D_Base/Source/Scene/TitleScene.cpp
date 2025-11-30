/*****************************************************************//**
 * @file	TitleScene.cpp
 * @brief
 *********************************************************************/

 // ===== インクルード =====
#include "Scene/TitleScene.h"
#include "Scene/StageSelectScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include "ECS/EntityFactory.h"

using namespace DirectX;

//仮の入力チェック関数
static bool IsInputStart() {
	return false;
}

// ===== TitleScene メンバー関数の実装  =====
void TitleScene::Init()
{
	m_coordinator = std::make_shared<ECS::Coordinator>();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// Transformで設定するサイズ
	float scaleX = 0.2f;
	float scaleY = 0.1f;

	// ▼▼▼ 修正: メンバ変数 m_startButton に代入する！ ▼▼▼
	m_startButton = m_coordinator->CreateEntity(
		TagComponent("button"),
		TransformComponent(
			XMFLOAT3(0.4f, 0.6f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(scaleX, scaleY, 1.0f)
		),
		// アニメーション無効 (false)
		UIInteractableComponent(0.4f, 0.6f, false),
		UIImageComponent("UI_TEST1")
	);

	// ▼▼▼ 追加: 元のサイズを正しく設定（巨大化防止） ▼▼▼
	{
		auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_startButton);
		comp.baseScaleX = scaleX;
		comp.baseScaleY = scaleY;
	}

	std::cout << "TitleScene::Init() - TitleUiSystem Ready." << std::endl;
}

void TitleScene::Uninit()
{
	ECS::ECSInitializer::UninitECS();
	m_coordinator.reset();
	std::cout << "TitleScene::Uninit() - Title Systems Destroyed." << std::endl;
}

void TitleScene::Update(float deltaTime)
{
	// 1. システムの一括更新
	// (ここで UIInputSystem も自動的に動くので、手動呼び出しは不要です！)
	m_coordinator->UpdateSystems(deltaTime);


	   // 2. スタートボタンのクリックチェック
	if (m_startButton != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_startButton);

		if (comp.isClicked)
		{
			std::cout << "Button Clicked! -> StageSelectScene" << std::endl;
			SceneManager::ChangeScene<StageSelectScene>();
		}
	}
}
void TitleScene::Draw()
{
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
}