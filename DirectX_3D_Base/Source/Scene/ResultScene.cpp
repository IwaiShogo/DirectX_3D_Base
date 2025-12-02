/*****************************************************************//**
 * @file	ResultScene.cpp
 * @brief
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author
 * ------------------------------------------------------------
 *
 * @date	2025/11/08	初回作成日
 * 			作業内容：	-
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード  =====
#include "Scene/ResultScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include <ECS/Systems/Core/ResultSceneSystem.h>

using namespace DirectX;
using namespace std;
using namespace ECS; // 追加

bool ResultScene::isClear = false;
int ResultScene::finalItenCount = 0;

void ResultScene::Init()
{
	m_coordinator = std::make_shared<ECS::Coordinator>();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// 背景
	m_coordinator->CreateEntity(
		TagComponent("ResultSceneBuckground"),
		TransformComponent(
			XMFLOAT3(0.0f, 0.0f, 2.0f), // 奥
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(2.0f, 2.0f, 1.0f)
		),
		UIImageComponent("UI_RESULTSCENE_BACKGROUND")
	);


	// クリア/失敗ロゴ
	std::string logoID = ResultScene::isClear ? "UI_GAMEOVER" : "UI_GAMEOVER"; // 必要ならID変える
	m_coordinator->CreateEntity(
		TagComponent("ResultLogo"),
		TransformComponent(
			XMFLOAT3(0.0f, 0.6f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.8f, 0.2f, 1.0f)
		),
		UIImageComponent(logoID)
	);

	// ===================================================
	// ボタン配置の調整 (ここがメインの修正)
	// ===================================================
	float buttonY = -0.6f;       // Y座標 (下の方)
	float buttonScaleX = 0.15f;  // 幅 (小さめに)
	float buttonScaleY = 0.15f;  // 高さ
	float buttonSpacing = 0.35f; // 間隔



	// 1. ステージセレクトボタン (左)
	ECS::EntityID ResultSelect = m_coordinator->CreateEntity(
		TagComponent("ResultSelect"),
		TransformComponent(
			XMFLOAT3(-buttonSpacing, buttonY, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(buttonScaleX, buttonScaleY, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent("UI_RESULT_SELECT")
	);
	// ★baseScale更新
	{
		auto& interact = m_coordinator->GetComponent<UIInteractableComponent>(ResultSelect);
		interact.baseScaleX = buttonScaleX;
		interact.baseScaleY = buttonScaleY;
	}

	// 2. リトライボタン (中央)
	ECS::EntityID ResultRetry = m_coordinator->CreateEntity(
		TagComponent("ResultRetry"),
		TransformComponent(
			XMFLOAT3(0.0f, buttonY, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(buttonScaleX, buttonScaleY, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent("UI_RESULT_RETORY")
	);
	// ★baseScale更新
	{
		auto& interact = m_coordinator->GetComponent<UIInteractableComponent>(ResultRetry);
		interact.baseScaleX = buttonScaleX;
		interact.baseScaleY = buttonScaleY;
	}

	// 3. タイトルボタン (右)
	ECS::EntityID ResultTitle = m_coordinator->CreateEntity(
		TagComponent("ResultTitle"),
		TransformComponent(
			XMFLOAT3(buttonSpacing, buttonY, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(buttonScaleX, buttonScaleY, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent("UI_RESULT_TITLE")
	);
	// ★baseScale更新
	{
		auto& interact = m_coordinator->GetComponent<UIInteractableComponent>(ResultTitle);
		interact.baseScaleX = buttonScaleX;
		interact.baseScaleY = buttonScaleY;
	}

	// 4. ボタン背景 (Interactableなし)
	m_coordinator->CreateEntity(
		TagComponent("btnresultnormal"),
		TransformComponent(
			XMFLOAT3(0.0f, buttonY, 0.1f), // ボタンより少し奥
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.8f, 0.25f, 1.0f)    // 全体を覆うサイズ
		),
		UIImageComponent("UI_BTN_RESULT_NORMAL")
	);


	std::cout << "ResultScene::Init() - UI Adjusted." << std::endl;
}

void ResultScene::Uninit()
{
	ECS::ECSInitializer::UninitECS();
	std::cout << "ResultScene::Uninit()" << std::endl;
}

void ResultScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);
}

void ResultScene::Draw()
{
	if (auto renderSystem = ECS::ECSInitializer::GetSystem<RenderSystem>())

	{

		renderSystem->DrawSetup();

		renderSystem->DrawEntities();

	}



	if (auto uiSystem = ECS::ECSInitializer::GetSystem<UIRenderSystem>())

	{

		::SetDepthTest(false);

		::SetBlendMode(BLEND_ALPHA);

		uiSystem->Render();

		::SetDepthTest(true);

	}
}