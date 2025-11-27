/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	ゲームのメインロジックを含むシーンクラスの実装。
 * 
 * @details	
 * ECSの初期化と実行、デモEntityの作成ロジックを内包する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：ECSのライフサイクルとデモロジックを管理する `GameScene` クラスの実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- 警備員AIの追加：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/GameScene.h"

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Gameplay/CollectionSystem.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "Systems/Input.h"
#include "ECS/Systems/Core/GameSceneSystem.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
 
// ===== 静的メンバー変数の定義 =====u
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* GameScene::s_coordinator = nullptr;

using namespace DirectX;

// ===== GameScene メンバー関数の実装 =====

void GameScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_shared<ECS::Coordinator>();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

    {
        auto system = m_coordinator->RegisterSystem<GameSceneSystem>();
        ECS::Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<GameSceneComponent>());
        m_coordinator->SetSystemSignature<GameSceneSystem>(signature);
        system->Init(m_coordinator.get());
    }

	// --- 4. デモ用Entityの作成 ---
	ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());

	ECS::EntityFactory::CreateGameSceneEntity(m_coordinator.get());
}

void GameScene::Uninit()
{
	// 1. ECS Systemの静的リソースを解放
	ECS::ECSInitializer::UninitECS();

	// Coordinatorの破棄（unique_ptrが自動的にdeleteを実行）
	m_coordinator.reset();

	// 静的ポインタをクリア
	s_coordinator = nullptr;

	std::cout << "GameScene::Uninit() - ECS Destroyed." << std::endl;
}

void GameScene::Update(float deltaTime)
{
	if (IsKeyTrigger('Q') || IsButtonTriggered(BUTTON_A))
	{
		SceneManager::ChangeScene<GameScene>();
	}

	// ECSの更新
	m_coordinator->UpdateSystems(deltaTime);

	if (IsKeyTrigger(VK_SPACE))
	{
		ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST");
	}
}

void GameScene::Draw()
{
	
	// エンティティの描画
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	// UIの描画
	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
}