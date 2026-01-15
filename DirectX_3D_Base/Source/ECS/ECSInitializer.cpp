/*****************************************************************//**
 * @file	ECSInitializer.cpp
 * @brief	ECSシステムの初期化情報をまとめ、Init()の責務を分担するためのヘルパークラスの実装
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/10/31	作成
 * 作業内容	- 新規追加
 *
 * @update	2025/11/08	最終更新
 * 作業内容	- 警備AIの追加
 *
 * @note
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/ECSInitializer.h"
#include "ECS/AllComponents.h"
#include "ECS/AllSystems.h"

// Screen transition (card-tilt fade)
#include "ECS/Components/Core/ScreenTransitionComponent.h"
#include "ECS/Systems/Core/ScreenTransitionSystem.h"

#include "ECS/Systems/UI/UIInputSystem.h"
#include "ECS/Components/UI/UIButtonComponent.h"
#include "ECS/Systems/Core/ResultControlSystem.h"


#include <iostream>

using namespace ECS;

// 静的メンバ変数 s_systems の実体定義
std::unordered_map<std::type_index, std::shared_ptr<ECS::System>> ECS::ECSInitializer::s_systems;

/**
 * [void - RegisterComponents]
 * @brief	全コンポーネントをCoordinatorに登録。
 *
 * @param	[in] coordinator
 */
void ECSInitializer::RegisterComponents(Coordinator* coordinator)
{
    // コンポーネントの登録（一括登録）
    for (const auto& registerFn : GetComponentRegisterers())
    {
        registerFn(coordinator);
    }

    std::cout << "ECSInitializer: All Components registered." << std::endl;
}

void ECSInitializer::RegisterSystemsAndSetSignatures(Coordinator* coordinator)
{
    // ============================================================
    // システムの登録とシグネチャの設定（ここに追加）
    // 登録順にシステムが実行されます。
    // ============================================================

    // ------------------------------------------------------------
    // 1. Update（更新系）
    // ------------------------------------------------------------

    // @system  PlayerControlSystem
    // @brief   プレイヤーのキー入力、コントローラー制御
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PlayerControlSystem,
        /* Components   */  PlayerControlComponent, TransformComponent, RigidBodyComponent, AnimationComponent
    );

    // @system  PhysicsSystem
    // @brief   物理的な位置の更新
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PhysicsSystem,
        /* Components   */  RigidBodyComponent, TransformComponent, CollisionComponent
    );

    // @system  CollectionSystem
    // @brief   アイテム回収チェック
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollectionSystem,
        /* Components   */  CollectableComponent, TransformComponent
    );

    // @system  CollisionSystem
    // @brief   衝突判定と位置補正
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollisionSystem,
        /* Components   */  CollisionComponent, TransformComponent, RigidBodyComponent
    );

    // @system  GameControlSystem
    // @brief   ゲームステート管理
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GameControlSystem,
        /* Components   */  GameStateComponent
    );

    // @system  CameraControlSystem
    // @brief   カメラとオブジェクトの追従更新
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CameraControlSystem,
        /* Components   */  CameraComponent
    );

    // @system  BasicCameraSystem
    // @brief   固定カメラ制御
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
    // @brief   警備AI制御
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GuardAISystem,
        /* Components   */  GuardComponent, TransformComponent, RigidBodyComponent
    );

    // @system  TeleportSystem
    // @brief   テレポート判定と移動の実行
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  TeleportSystem,
        /* Components   */  TeleportComponent, TransformComponent
    );

    // @system UIInputSystem
    // @brief   マウスカーソルの入力判定
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        UIInputSystem,
        UIButtonComponent, TransformComponent
    );

    // @system  CursorSystem
    // @brief   UI用カーソル表示
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        CursorSystem,
        UICursorComponent, TransformComponent
    );

    // @system  AudioSystem
    // @brief   音声再生管理
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
    // @brief   生存時間管理
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
        TagComponent, UIButtonComponent
    );
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        OpeningControlSystem,
        TagComponent, UIButtonComponent

    );

    // 浮遊システムの設定
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        FloatingSystem,
        TransformComponent, FloatingComponent
    );

    // @system  EnemySpawnSystem
    // @brief   敵のスポーン管理
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  EnemySpawnSystem,
        /* Components   */  EnemySpawnComponent
    );

    // @system  EffectSystem
    // @brief   エフェクト（VFX）管理
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  EffectSystem,
        /* Components   */  EffectComponent, TransformComponent
    );

    // @system  ScreenTransitionSystem
    // @brief   画面遷移（カード斜めフェード）: Transform + UIImage + ScreenTransitionComponent を更新
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  ScreenTransitionSystem,
        /* Components   */  TransformComponent, UIImageComponent, ScreenTransitionComponent
    );


    // ------------------------------------------------------------
    // 2. Draw（描画系）
    // ------------------------------------------------------------

    // @system  FlickerSystem
    // @brief   点滅エフェクト
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  FlickerSystem,
        /* Components   */  FlickerComponent
    );

    // @system  RenderSystem
    // @brief   カメラ設定、デバッグ描画を含むエンティティの描画
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  RenderSystem,
        /* Components   */  RenderComponent, TransformComponent
    );

    // @system  UIRenderSystem
    // @brief   UI要素の描画
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  UIRenderSystem,
        /* Components   */  UIImageComponent
    );

    // ------------------------------------------------------------
    // 3. その他（毎フレーム更新しないシステムなど）
    // ------------------------------------------------------------

    // @system  MapGenerationSystem
    // @brief   マップデータの生成
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  MapGenerationSystem,
        /* Components   */  MapComponent
    );

    std::cout << "ECSInitializer: All Systems registered and initialized." << std::endl;
}

/**
 * [void - InitECS]
 * @brief	CoordinatorとSystemの初期化エントリーポイント。
 *
 * @param	[in] coordinator
 */
void ECSInitializer::InitECS(std::shared_ptr<Coordinator>& coordinator)
{
    // Coordinatorのポインタ取得
    Coordinator* rawCoordinator = coordinator.get();

    // 1. Coordinator自体の初期化 (ECSデータバッファの初期化)
    rawCoordinator->Init();

    // 2. コンポーネントの登録
    RegisterComponents(rawCoordinator);

    // 3. システムの登録とシグネチャの設定 (静的マップに格納)
    RegisterSystemsAndSetSignatures(rawCoordinator);
}

/**
 * @brief ECSに関連するすべての静的リソースのクリーンアップ。
 */
void ECSInitializer::UninitECS()
{
    s_systems.clear(); // 全システムSharedPtrの解放
}