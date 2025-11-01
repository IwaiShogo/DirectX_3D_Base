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
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/GameScene.h"

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
 
// ===== 静的メンバー変数の定義 =====u
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* GameScene::s_coordinator = nullptr;

// ===== デモ用の変数 (Main.cppから移管) =====
namespace SceneDemo
{
	static float RotationRad = 0.0f;
	static float Time = 0.0f;
	const float ROTATION_INCREMENT = 0.01f; // 1フレームあたり0.01ラジアン回転
	const float TIME_INCREMENT = 0.05f; // 毎フレームの時間の進み具合
}

using namespace DirectX;

// ===== GameScene メンバー関数の実装 =====

void GameScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_unique<ECS::Coordinator>();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 4. デモ用Entityの作成 ---
	ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());

	std::cout << "GameScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;
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
	// --- 1. デモ用変数の更新ロジック (Main.cppから移管) ---
	SceneDemo::RotationRad += SceneDemo::ROTATION_INCREMENT;
	if (SceneDemo::RotationRad > XM_2PI)
	{
		SceneDemo::RotationRad -= XM_2PI;
	}

	SceneDemo::Time += SceneDemo::TIME_INCREMENT;
	const float CENTER_Y = 1.5f;
	const float AMPLITUDE = 0.5f;
	const float FREQUENCY = 2.0f;
	float newY = CENTER_Y + AMPLITUDE * sin(SceneDemo::Time * FREQUENCY);

	// --- 2. ECS Systemの更新
	// 1. 入力
	// if (m_playerControlSystem) // 削除
	if (auto system = ECS::ECSInitializer::GetSystem<PlayerControlSystem>())
	{
		system->Update();
	}

	// 2. 物理計算（位置の更新）
	// if (m_physicsSystem) // 削除
	if (auto system = ECS::ECSInitializer::GetSystem<PhysicsSystem>())
	{
		system->Update();
	}

	// 3. 衝突検出と応答（位置の修正）
	// if (m_collisionSystem) // 削除
	if (auto system = ECS::ECSInitializer::GetSystem<CollisionSystem>())
	{
		system->Update();
	}

	// 4. カメラ制御（ビュー・プロジェクション行列の更新）
	// if (m_cameraControlSystem) // 削除
	if (auto system = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
	{
		system->Update();
	}

	// --- 3. ECS EntityのComponent更新 ---
	// 3つ目のEntity（ID: 2）は回転する箱と仮定
	const ECS::EntityID rotatingBoxID = 2;

	if (m_coordinator && m_coordinator->m_entityManager->GetSignature(rotatingBoxID).test(m_coordinator->m_componentManager->GetComponentTypeID<TransformComponent>()))
	{
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(rotatingBoxID);
		//transform.Rotation.y = SceneDemo::RotationRad; // Y軸回転を更新
		//transform.Position.y = newY; // Y軸位置を更新
	}
}

void GameScene::Draw()
{
	// RenderSystemは常に存在すると仮定
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		// 1. カメラ設定やデバッググリッド描画
		system->DrawSetup();

		// 2. ECS Entityの描画
		system->DrawEntities();
	}
}