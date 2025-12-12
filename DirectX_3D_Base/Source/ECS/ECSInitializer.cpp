/*****************************************************************//**
 * @file	ECSInitializer.cpp
 * @brief	ECSシステム全体の初期化を集約し、シーンのInit()から責務を分離するためのヘルパークラスの実装
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
 * 			作業内容：	- 追加：警備員AIの追加
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/ECSInitializer.h"
#include "ECS/AllComponents.h"
#include "ECS/AllSystems.h"

#include "ECS/Systems/UI/UIInputSystem.h"
#include "ECS/Components/UI/UIButtonComponent.h"
#include "ECS/Systems/Core/ResultControlSystem.h"
#include "ECS/Components/Core/ItemProximityEffectSystem.h"
#include "ECS/Systems/Core/ItemProximityEffectSystem.h"


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
    // ============================================================
    // システムの登録とシグネチャの設定（ここから下に追加）
    // ※登録順にシステムが実行される。
    // ============================================================

    // ------------------------------------------------------------
    // 1. Update（更新処理）
    // ------------------------------------------------------------

    // @system  PlayerControlSystem
    // @brief   キー入力、コントローラー入力
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PlayerControlSystem,
        /* Components   */  PlayerControlComponent, TransformComponent, RigidBodyComponent,AnimationComponent
    );

    // @system  PhysicsSystem
    // @brief   物理計算（位置の更新）
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PhysicsSystem,
        /* Components   */  RigidBodyComponent, TransformComponent, CollisionComponent
    );

    // @system  CollectionSystem
    // @brief   アイテム回収ロジック
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollectionSystem,
        /* Components   */  CollectableComponent, TransformComponent
    );

    // @system  CollisionSystem
    // @brief   衝突検出と応答（位置の修正）
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollisionSystem,
        /* Components   */  CollisionComponent, TransformComponent, RigidBodyComponent
    );

    // @system  GameControlSystem
    // @brief   ゲームステート
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GameControlSystem,
        /* Components   */  GameStateComponent
    );

    // @system  CameraControlSystem
    // @brief   カメラ制御（ビュー・プロジェクション行列の更新）
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CameraControlSystem,
        /* Components   */  CameraComponent
    );

    // @system  BasicCameraSystem
    // @brief   固定カメラ
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  BasicCameraSystem,
        /* Components   */  BasicCameraComponent, TransformComponent
    );

#ifdef _DEBUG
    // @system  DebugDrawSystem
    // @brief   デバッグ描画システム
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  DebugDrawSystem,
        /* Components   */  DebugComponent
    );
#endif

    // @system  GuardAISystem
    // @brief   警備員AI
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GuardAISystem,
        /* Components   */  GuardComponent, TransformComponent, RigidBodyComponent
	);

    // @system UIInoutSystem
    // @brief  マウスカーソルの判定
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        UIInputSystem,
        UIButtonComponent, TransformComponent
    );
    
    // @system  CursorSystem
    // @brief   カーソルUI
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        CursorSystem,
        UICursorComponent, TransformComponent
    );

    // @system  AudioSystem
    // @brief   音声再生
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  AudioSystem,
        /* Components   */  SoundComponent
    );

    // @system  AnimationSystem
    // @brief   アニメーション更新
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  AnimationSystem,
        /* Components   */  ModelComponent, AnimationComponent
    );

    // @system  LifeTimeSystem
    // @brief   生存時間
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  LifeTimeSystem,
        /* Components   */  LifeTimeComponent
    );

    // @system  TitleControlSystem
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  TitleControlSystem,
        /* Components   */  TitleControllerComponent
    );

    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        ResultControlSystem,
        TagComponent,UIButtonComponent

    );

    // 2. システム登録とシグネチャ設定
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        FloatingSystem,
        TransformComponent, FloatingComponent
    );

    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        RotatorSystem,
        TransformComponent,
        RotatorComponent
    );
    // ------------------------------------------------------------
    // 2. Draw（描画処理）
    // ------------------------------------------------------------


    // @system  RenderSystem
    // @brief   カメラ設定や、デバッググリッド描画＆Entitiesの描画
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  RenderSystem,
        /* Components   */  RenderComponent, TransformComponent 
    );

    // @system  ItemProximityEffectSystem
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        ItemProximityEffectSystem,
        CollectableComponent, TransformComponent, ProximitySparkleEffectComponent
    );

    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  EffectSystem,
        /* Components   */  EffectComponent, TransformComponent
    );

    // @system  UIRenderSystem
    // @brief   UIの描画
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  UIRenderSystem,
        /* Components   */  UIImageComponent
    );

    // ------------------------------------------------------------
    // 3. その他Updateが行われないシステム
    // ------------------------------------------------------------

    // @system  MapGenerationSystem
    // @brief   ランダムマップを生成
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  MapGenerationSystem,
        /* Components   */  MapComponent
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