/*****************************************************************//**
 * @file	TitleScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/30	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "Scene/TitleScene.h"
#include "Scene/StageSelectScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>

#include "ECS/Components/Rendering/RenderComponent.h"
#include "ECS/Components/Rendering/ModelComponent.h"
#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include "ECS/EntityFactory.h"
#include <ECS/Systems/Core/TitleSceneSystem.h>


using namespace DirectX;
using namespace ECS;

void TitleScene::Init()
{
	// 1. ECS初期化
	m_coordinator = std::make_shared<ECS::Coordinator>();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// 2. TitleSceneSystem の登録
	{
		auto system = m_coordinator->RegisterSystem<TitleSceneSystem>();
		ECS::Signature signature;
		// このシステムは TitleSceneComponent を持つエンティティだけを管理する
		signature.set(m_coordinator->GetComponentTypeID<TitleSceneComponent>());
		m_coordinator->SetSystemSignature<TitleSceneSystem>(signature);
		system->Init(m_coordinator.get());
	}

	// 3. 管理用エンティティの生成
	// これを作ると TitleSceneSystem が動き出し、自動的にボタンなどが生成される
	ECS::EntityFactory::CreateTitleSceneEntity(m_coordinator.get());

	std::cout << "TitleScene::Init() - System Started." << std::endl;
}

void TitleScene::Uninit()
{
	ECS::ECSInitializer::UninitECS();
}

void TitleScene::Update(float deltaTime)
{
	// ロジックは全て System に任せる
	m_coordinator->UpdateSystems(deltaTime);
}

void TitleScene::Draw()
{
	// 描画も System に任せる
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