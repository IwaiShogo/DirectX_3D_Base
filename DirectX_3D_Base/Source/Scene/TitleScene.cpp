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


#include "Scene/TitleScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include "ECS/EntityFactory.h"
#include <ECS/Systems/Core/TitleSceneSystem.h>

using namespace DirectX;

bool IsInputStart() {

	return false;
}

void TitleScene::Init()
{
	m_coordinator = std::make_shared<ECS::Coordinator>();

	ECS::ECSInitializer::InitECS(m_coordinator);

	{
		auto system = m_coordinator->RegisterSystem<TitleSceneSystem>();
		ECS::Signature signature;
		signature.set(m_coordinator->GetComponentTypeID<TitleSceneComponent>());
		m_coordinator->SetSystemSignature<TitleSceneSystem>(signature);
		system->Init(m_coordinator.get());
	}

	// --- 4. デモ用Entityの作成 ---	
	ECS::EntityFactory::CreateTitleSceneEntity(m_coordinator.get());
}

void TitleScene::Uninit()
{
	ECS::ECSInitializer::UninitECS();

	m_coordinator.reset();


	//このシーンで作成したエンティティを破棄
	//ECS::ECSInitializer::GetCoordinator()->DestoryEntities(m_sceneEntities);
	std::cout << "TitleScene::Uninit() - Title  Systems Destroyed." << std::endl;
}

void TitleScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);
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