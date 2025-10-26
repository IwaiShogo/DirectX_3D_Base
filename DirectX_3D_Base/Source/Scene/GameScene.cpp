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

// Component
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/RigidBodyComponent.h"

// System
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/PhysicsSystem.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用

// ===== 静的メンバー変数の定義 =====
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

/**
 * @brief デモ用のEntity（地面と回転する箱）を作成し、ECSに登録する
 * @param coordinator - Coordinatorインスタンスへのポインタ
 */
static void CreateDemoEntities(ECS::Coordinator* coordinator)
{
	// --- 1. 1つ目の地面 (Transform + Render) ---
	ECS::EntityID ground1 = coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(0.0f, -0.1f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(10.0f, 0.2f, 10.0f)
		),
		RenderComponent(
			MESH_BOX,
			XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
		)
	);

	// --- 2. 2つ目の地面（柱） (Transform + Render) ---
	ECS::EntityID ground2 = coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(0.0f, 1.0f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.2f, 3.0f, 0.2f)
		),
		RenderComponent(
			MESH_BOX,
			XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f)
		),
		RigidBodyComponent(

		)
	);

	// --- 3. 回転する箱 (Transform + Render) ---
	ECS::EntityID rotatingBox = coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(1.0f, 1.5f, 0.0f),
			XMFLOAT3(0.0f, SceneDemo::RotationRad, 0.0f),
			XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			MESH_BOX,
			XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
		)
	);
}


// ===== GameScene メンバー関数の実装 =====

GameScene::GameScene()
{
	// コンストラクタで特に処理は行わず、Init()でECSを初期化する
}

GameScene::~GameScene()
{
	// デストラクタでUninit()が呼ばれ、m_coordinatorが解放される
}

void GameScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_unique<ECS::Coordinator>();
	m_coordinator->Init();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	// --- 2. Componentの登録 ---
	m_coordinator->RegisterComponentType<TransformComponent>();
	m_coordinator->RegisterComponentType<RenderComponent>();
	m_coordinator->RegisterComponentType<RigidBodyComponent>();

	// --- 3. Systemの登録とSignatureの設定 ---
	// RenderSystemの登録
	m_renderSystem = m_coordinator->RegisterSystem<RenderSystem>();

	ECS::Signature renderSignature;
	renderSignature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
	renderSignature.set(m_coordinator->GetComponentTypeID<RenderComponent>());
	m_coordinator->SetSystemSignature<RenderSystem>(renderSignature);
	m_renderSystem->Init();

	// PhysicsSystemの登録
	m_physicsSystem = m_coordinator->RegisterSystem<PhysicsSystem>();

	ECS::Signature physicsSignature;
	physicsSignature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
	physicsSignature.set(m_coordinator->GetComponentTypeID<RigidBodyComponent>());
	m_coordinator->SetSystemSignature<PhysicsSystem>(physicsSignature);
	m_physicsSystem->Init();

	// --- 4. デモ用Entityの作成 ---
	CreateDemoEntities(m_coordinator.get());

	std::cout << "GameScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;
}

void GameScene::Uninit()
{
	// Coordinatorの破棄（unique_ptrが自動的にdeleteを実行）
	m_coordinator.reset();
	m_renderSystem.reset();

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

	// --- 2. ECS EntityのComponent更新 ---
	// 3つ目のEntity（ID: 2）は回転する箱と仮定
	const ECS::EntityID rotatingBoxID = 2;

	if (m_coordinator && m_coordinator->m_entityManager->GetSignature(rotatingBoxID).test(m_coordinator->m_componentManager->GetComponentTypeID<TransformComponent>()))
	{
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(rotatingBoxID);
		transform.Rotation.y = SceneDemo::RotationRad; // Y軸回転を更新
		transform.Position.y = newY; // Y軸位置を更新
	}

	// TODO: PhysicsSystemなどが実装されたら、ここでUpdateSystemを実行する
	if (m_physicsSystem)
	{
		m_physicsSystem->Update();
	}
}

void GameScene::Draw()
{
	// RenderSystemは常に存在すると仮定
	if (m_renderSystem)
	{
		// 1. カメラ設定やデバッググリッド描画
		m_renderSystem->DrawSetup();

		// 2. ECS Entityの描画
		m_renderSystem->DrawEntities();
	}
}