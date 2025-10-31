/*****************************************************************//**
 * @file	ECSInitializer.cpp
 * @brief	ECSシステム全体の初期化を集約し、シーンのInit()から責務を分離するためのヘルパークラスの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/ECSInitializer.h"
#include "ECS/AllComponents.h"
#include "ECS/AllSystems.h"
#include <iostream>

using namespace ECS;

// 静的メンバー変数 s_systems の実体を定義し、メモリを確保する
std::unordered_map<std::type_index, std::shared_ptr<ECS::System>> ECS::ECSInitializer::s_systems;

/**
 * [void - RegisterComponents]
 * @brief	全てのコンポーネントをCoordinatorに登録する。
 * 
 * @param	[in] coordinator 
 */
void ECSInitializer::RegisterComponents(Coordinator* coordinator)
{
	// --- Componentの登録（GameScene::Init()から移動） ---
	coordinator->RegisterComponentType<TransformComponent>();
	coordinator->RegisterComponentType<RenderComponent>();
	coordinator->RegisterComponentType<RigidBodyComponent>();
	coordinator->RegisterComponentType<CollisionComponent>();
	coordinator->RegisterComponentType<PlayerControlComponent>();
	coordinator->RegisterComponentType<CameraComponent>();
	coordinator->RegisterComponentType<ModelComponent>();
	// --- 新しいComponentを追加する際はここへ追記 ---

	std::cout << "ECSInitializer: All Components registered." << std::endl;
}

void ECSInitializer::RegisterSystemsAndSetSignatures(Coordinator* coordinator)
{
    Coordinator* coordPtr = coordinator;

    // --- 1. RenderSystem ---
    {
        auto system = coordinator->RegisterSystem<RenderSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(coordinator->GetComponentTypeID<RenderComponent>());
        // Coordinatorが要求する型（テンプレート）でシグネチャを設定
        coordinator->SetSystemSignature<RenderSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(RenderSystem))] = system;
    }

    // --- 2. PhysicsSystem ---
    {
        auto system = coordinator->RegisterSystem<PhysicsSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(coordinator->GetComponentTypeID<RigidBodyComponent>());
        signature.set(coordinator->GetComponentTypeID<CollisionComponent>()); // GameSceneから復元
        // Coordinatorが要求する型（テンプレート）でシグネチャを設定
        coordinator->SetSystemSignature<PhysicsSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(PhysicsSystem))] = system;
    }

    // --- 3. PlayerControlSystem ---
    {
        auto system = coordinator->RegisterSystem<PlayerControlSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<RigidBodyComponent>());
        signature.set(coordinator->GetComponentTypeID<PlayerControlComponent>());
        coordinator->SetSystemSignature<PlayerControlSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(PlayerControlSystem))] = system;
    }

    // --- 4. CollisionSystem ---
    {
        auto system = coordinator->RegisterSystem<CollisionSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(coordinator->GetComponentTypeID<RigidBodyComponent>());
        signature.set(coordinator->GetComponentTypeID<CollisionComponent>());
        coordinator->SetSystemSignature<CollisionSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(CollisionSystem))] = system;
    }

    // --- 5. CameraControlSystem ---
    {
        auto system = coordinator->RegisterSystem<CameraControlSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<CameraComponent>());
        coordinator->SetSystemSignature<CameraControlSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(CameraControlSystem))] = system;
    }

    std::cout << "ECSInitializer: All Systems registered and initialized." << std::endl;
}

/**
 * [void - InitECS]
 * @brief	CoordinatorとSystemを関連付けるエントリポイント。
 * 
 * @param	[in] coordinator 
 */
void ECSInitializer::InitECS(std::shared_ptr<Coordinator>& coordinator)
{
	// Coordinatorの生ポインタを取得
	Coordinator* rawCoordinator = coordinator.get();

	// 1. Coordinator自体の初期化 (ECSコア内部のデータ構造の初期化)
	rawCoordinator->Init();

	// GameScene::s_coordinator への設定は、次のステップでGameScene::Init()に移動します。

	// 2. コンポーネントの登録
	RegisterComponents(rawCoordinator);

	// 3. システムの登録とシグネチャの設定 (静的マップに格納される)
	RegisterSystemsAndSetSignatures(rawCoordinator);
}

/**
 * @brief ECSに関連する全ての静的リソースをクリーンアップする。
 */
void ECSInitializer::UninitECS()
{
	s_systems.clear(); // 全てのシステムSharedPtrを解放
}