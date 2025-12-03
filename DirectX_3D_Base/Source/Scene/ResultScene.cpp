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
#include "ECS/EntityFactory.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIButtonComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include <ECS/Systems/Core/ResultSceneSystem.h>

using namespace DirectX;
using namespace std;

//仮の入力チェック関数
bool IsInputTitle() { return false; }
bool ResultScene::isClear = false;
int ResultScene::finalItenCount = 0;


//===== ResultScene メンバー関数の実装 =====

void ResultScene::Init()
{

	m_coordinator = std::make_shared<ECS::Coordinator>();

	ECS::ECSInitializer::InitECS(m_coordinator);
	
	ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), {0.0f, 0.0f, 0.0f});

	std::cout << "ResultScene::Init() - ResultUISystem Ready." << std::endl;
}

void ResultScene::Uninit()
{
	//このシーンで作成したエンティティを破棄
	//ECS::ECSInitializer::GetCoordinator()->DestoryEntities(m_sceneEntities);
	//m_buttons.clear();
	std::cout << "ResultScene::Uninit() - Result Scene Systems Destroyed." << std::endl;
}

void ResultScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);

	/*ECS::EntityID interactableEntity = ECS::FindFirstEntityWithComponent<UIButtonComponent>(m_coordinator.get());*/


	/*auto uiInput = ECS::ECSInitializer::GetSystem<UIInputSystem>();
	if (uiInput)
	{
		uiInput->Update(deltaTime);
	}*/

	// ボタンのクリック判定
	



	
	
}

void ResultScene::Draw()
{
	//RenderSystemは常に存在すると仮定し、Draw処理は共有する
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();	//UIエンティティの描画
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
	
}