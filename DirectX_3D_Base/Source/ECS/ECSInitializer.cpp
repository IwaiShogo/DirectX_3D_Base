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
 * @update	2025/11/08	最終更新日
 * 			作業内容：	- 警備員AIの追加：
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
    // コンポーネントの登録（自動で登録される）
    for (const auto& registerFn : GetComponentRegisterers())
    {
        registerFn(coordinator);
    }

	std::cout << "ECSInitializer: All Components registered." << std::endl;
}

void ECSInitializer::RegisterSystemsAndSetSignatures(Coordinator* coordinator)
{
    Coordinator* coordPtr = coordinator;

    // ============================================================
    // システムの登録とシグネチャの設定（ここから下に追加）
    // ============================================================

    // --- RenderSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  RenderSystem,
        /* Components   */  RenderComponent, TransformComponent
    );

    // --- PhysicsSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PhysicsSystem,
        /* Components   */  RigidBodyComponent, TransformComponent, CollisionComponent
    );

    // --- PlayerControlSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PlayerControlSystem,
        /* Components   */  PlayerControlComponent, TransformComponent, RigidBodyComponent
    );
    
    // --- CollisionSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollisionSystem,
        /* Components   */  CollisionComponent, TransformComponent, RigidBodyComponent
    );

    // --- CameraControlSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CameraControlSystem,
        /* Components   */  CameraComponent
    );

    // --- StateSwitchSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  StateSwitchSystem,
        /* Components   */  GameStateComponent
    );

    // --- CollectionSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollectionSystem,
        /* Components   */  CollectableComponent, TransformComponent
    );

    // --- GameFlowSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GameFlowSystem,
        /* Components   */  GameStateComponent
    );

	// --- GuardAISystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GuardAISystem,
        /* Components   */  GuardComponent, TransformComponent, RigidBodyComponent
	);

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