/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/GameScene.h"
#include "ECS/Coordinator.h"
// Component
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"
#include "ECS/Component/Input.h"
#include "ECS/Component/Collider.h"
#include "ECS/Component/Camera.h"
// System
#include "ECS/System/MovementSystem.h"
#include "ECS/System/RenderSystem.h"
#include "ECS/System/InputSystem.h"
#include "ECS/System/CollisionSystem.h"
#include "ECS/System/CameraSystem.h"

/* リソースマネージャー */
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"

// --------------------------------------------------
// グローバルなCoordinatorの定義 (RenderSystem.hでextern宣言したもの)
// --------------------------------------------------
// ※ 実際のプロジェクトでは、より安全なDI (依存性注入)やシングルトンパターンを推奨しますが、
//    ここではECSのコア構造をシンプルに動作させるため、GameSceneが保持するインスタンスへの
//    グローバルアクセスを許可する形で定義します。
Coordinator* g_pCoordinator = nullptr;

// --------------------------------------------------
// 内部関数: ECSの初期設定
// --------------------------------------------------

/**
 * @brief ECSコア、Component、Systemの初期化と登録を行う
 */
void GameScene::SetupECS()
{
    // 1. Coordinatorの初期化 (内部の各種Managerを初期化)
    coordinator_.Init();

    // 2. Componentの登録
    // 使用する全てのComponent型を登録し、ComponentType IDを割り当てる
    coordinator_.RegisterComponent<Transform>();
    coordinator_.RegisterComponent<MeshRenderer>();
    coordinator_.RegisterComponent<Input>();
    coordinator_.RegisterComponent<Collider>();
    coordinator_.RegisterComponent<CameraSetting>();
    // ... 今後追加する Component (例: Input, Health, Collider) もここに追加

    // 3. Systemの登録とSignatureの設定

    // MovementSystemの登録とシグネチャ設定
    movementSystem_ = coordinator_.RegisterSystem<MovementSystem>();
    Signature movementSignature;
    movementSignature.set(coordinator_.GetComponentType<Transform>(), true);
    // movementSignature.set(coordinator_.GetComponentType<Velocity>(), true); // Velocity Componentがあれば
    coordinator_.SetSystemSignature<MovementSystem>(movementSignature);
    movementSystem_->Initialize();

    // RenderSystemの登録とシグネチャ設定
    renderSystem_ = coordinator_.RegisterSystem<RenderSystem>();
    Signature renderSignature;
    renderSignature.set(coordinator_.GetComponentType<Transform>(), true);
    renderSignature.set(coordinator_.GetComponentType<MeshRenderer>(), true);
    coordinator_.SetSystemSignature<RenderSystem>(renderSignature);
    renderSystem_->Initialize();

    // InputSystemの登録とシグネチャ設定
    inputSystem_ = coordinator_.RegisterSystem<InputSystem>();
    Signature inputSignature;
    inputSignature.set(coordinator_.GetComponentType<Input>(), true); // Input Componentのみ
    coordinator_.SetSystemSignature<InputSystem>(inputSignature);
    inputSystem_->Initialize();

    // CollisionSystemの登録とシグネチャ設定
    collisionSystem_ = coordinator_.RegisterSystem<CollisionSystem>();
    Signature collisionSignature;
    collisionSignature.set(coordinator_.GetComponentType<Transform>(), true);
    collisionSignature.set(coordinator_.GetComponentType<Collider>(), true); // ★Collider Componentを要求
    coordinator_.SetSystemSignature<CollisionSystem>(collisionSignature);
    //collisionSystem_->Initialize();

    // CameraSystemの登録とシグネチャ設定
    cameraSystem_ = coordinator_.RegisterSystem<CameraSystem>(); // ★追加
    Signature cameraSignature;
    cameraSignature.set(coordinator_.GetComponentType<Transform>(), true);
    cameraSignature.set(coordinator_.GetComponentType<CameraSetting>(), true);
    coordinator_.SetSystemSignature<CameraSystem>(cameraSignature);
    cameraSystem_->Initialize();

    // 4. グローバルなCoordinatorへの参照を設定
    // GameSceneが持つインスタンスをグローバル参照に設定
    // これはRenderSystem::Draw()内のg_Coordinatorアクセスを有効にするために必須
    g_pCoordinator = &coordinator_;
}

/**
 * @brief ECS Entityの初期配置（テスト用Entityの生成など）を行う
 */
void GameScene::SetupEntities()
{
    // --------------------------------------------------
    // メインカメラ Entityの生成
    // --------------------------------------------------
    mainCameraEntity_ = coordinator_.CreateEntity();

    // Transform
    Transform camT;
    camT.position = { 0.0f, 10.0f, -20.0f };
    camT.rotation = { 20.0f, 0.0f, 0.0f };
    coordinator_.AddComponent(mainCameraEntity_, camT);

    // CameraSetting
    CameraSetting camS;
    camS.aspectRatio = 1280.0f / 720.0f;
    coordinator_.AddComponent(mainCameraEntity_, camS);





    // テスト用のEntityを生成
    Entity cube = coordinator_.CreateEntity();

    // Transform Componentを追加
    Transform t;
    t.position = { 10.0f, 5.0f, 0.0f };
    t.rotation = { 0.0f, 45.0f, 0.0f };
    coordinator_.AddComponent(cube, t);

    // MeshRenderer Componentを追加
    MeshRenderer mr;
    mr.meshId = 1;      // 実際のメッシュID
    mr.textureId = 1;   // 実際のテクスチャID
    mr.isLoaded = true;
    mr.color = { 0.8f, 0.2f, 0.2f, 1.0f }; // 赤いキューブ
    coordinator_.AddComponent(cube, mr);

    // RenderSystemとMovementSystemの両方のSignatureを満たすため、両方のSystemに処理対象として追加される。
}

// --------------------------------------------------
// Sceneインターフェースの実装
// --------------------------------------------------

void GameScene::Initialize()
{
    // ECSのセットアップ
    SetupECS();

    // Entityの初期配置
    SetupEntities();

    // ... カメラやDirectXリソースの初期化など
}

void GameScene::Update(float deltaTime)
{
    // --------------------------------------------------
    // ECSの更新処理 (データ駆動型のロジック実行)
    // --------------------------------------------------

    // 1. InputSystemの更新（外部の状態をECSデータに書き込む）
    inputSystem_->Update(deltaTime);

    // 2. カメラSystemの更新
    cameraSystem_->Update(deltaTime);

    // 2. ロジックSystemの更新
    // MovementSystemはTransform Componentを更新し、位置を変化させる
    movementSystem_->Update(deltaTime);

    // 3. 衝突Systemの更新 (衝突フラグの更新)
    // 移動後のTransformデータを使用して衝突判定を実行
    collisionSystem_->Update(deltaTime);

    // ... 他のSystem (例: InputSystem, PhysicsSystem, AnimationSystem) もここで実行

    // --------------------------------------------------
    // 注意: RenderSystemはUpdateでは描画せず、Drawで実行することが一般的です。
    // --------------------------------------------------
}

void GameScene::Draw()
{
    // DirectXの描画開始処理（例: ClearRenderTargetView, BeginSceneなど）

    // 2. 描画Systemの実行
    // RenderSystemはTransformとMeshRendererを読み取り、DirectXのDrawCallを発行する
    // ※ CameraとMeshManagerは外部システムとしてDrawメソッドに渡す必要があります。
    //   ここではCameraとMeshManagerが外部で定義されていることを前提に擬似的に呼び出します。
    //   (例: Camera* g_Camera; MeshManager* g_MeshManager; )

    // Camera& cam = GetCurrentCamera(); 
    // MeshManager& meshMgr = GetMeshManager();

    // renderSystem_->Draw(cam, meshMgr); 

    // DirectXの描画終了処理（例: Presentなど）

    // 1. カメラSystemから最新のViewProjection行列を取得
    // CameraSystemは既にUpdateで計算済み
    DirectX::XMMATRIX viewProjMatrix = cameraSystem_->GetViewProjectionMatrix(); // ★修正

    // 2. RenderSystemの実行
    // RenderSystem.hのDrawメソッドのシグネチャに合わせて、ダミーのMeshManagerを使用
    class DummyMeshManager {};
    DummyMeshManager meshMgr;

    renderSystem_->Draw(viewProjMatrix, meshMgr); // ★修正: ViewProj行列を渡す

}

void GameScene::Finalize()
{
    // ... シーン固有のリソース解放など
}