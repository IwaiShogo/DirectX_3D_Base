/*****************************************************************//**
 * @file	TitleScene.cpp
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

// ===== インクルード =====
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



using namespace DirectX;




//仮の入力チェック関数
bool IsInputStart() {
	//ここに実際の入力チェックロジックが入る
	//今回は遷移テストのため、デバッグで一時的にtrueを返すなどしてもいい
	return false;
}

// ===== TitleScene メンバー関数の実装  =====
void TitleScene::Init()
{

	
	m_coordinator = std::make_shared<ECS::Coordinator>();

	ECS::ECSInitializer::InitECS(m_coordinator);


	//ECS::EntityFactory::CreatePlayer(m_coordinator.get(), XMFLOAT3(0.0f, 0.0f, 0.0f));
	
	ECS::EntityID buttonEntity = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"button"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.4f, 0.6f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.2f, 0.1f, 1.0f)
		),
		UIInteractableComponent(0.4f,0.6f),
		UIImageComponent(
			"UI_TEST1"	
			)
		);

	

	

	
	



	// TitleSceneに必要なエンティティの作成 (例：ロゴ、ボタン)
	//ECS::EntityFactory::CreateTitleUiEntities(ECS::ECSInitializer::GetCoordinator()):
	std::cout << "TitleScene::Init() - TitleUiSystem Ready." << std::endl;

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



	//auto interactableEntity = ECS::ECSInitializer::GetSystem<UIInputSystem>();

	//auto comp = m_coordinator.get()->GetComponent<UIInteractableComponent>(interactable);

	auto uiInputSystem = ECS::ECSInitializer::GetSystem<UIInputSystem>();
	if (uiInputSystem)
	{
		uiInputSystem->Update();
	}


	ECS::EntityID interactableEntity = ECS::FindFirstEntityWithComponent<UIInteractableComponent>(m_coordinator.get());


		if (interactableEntity != ECS::INVALID_ENTITY_ID)
		{
			const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(interactableEntity);

			if (comp.isClicked)
			{
				std::cout << "Button Clicked! -> GameScene" << std::endl;
				SceneManager::ChangeScene<GameScene>();
			}
			
		}





	
	
	
	if (IsKeyTrigger('N'))
	{
		SceneManager::ChangeScene<GameScene>();//N:ゲームシーンに切り替え
	}
}

void TitleScene::Draw()
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