/*****************************************************************//**
 * @file	EntityFactory.cpp
 * @brief	特定のエンティティ（プレイヤー、地面など）の生成ロジックを集約するヘルパークラスの実装。
 * 
 * @details	
 * Componentの具体的な値設定をここに集約し、シーンコードをシンプルにする。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：エンティティ生成の静的実装を作成。地面エンティティの生成ロジックを移動。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/EntityFactory.h"
#include "ECS/ECS.h" // すべてのコンポーネントとCoordinatorにアクセスするため
#include "Main.h" // METERなどの定数にアクセス

using namespace ECS;
using namespace DirectX;

// 静的メンバ変数の定義 (必要に応じて追加)
// const static ECS::EntityID EntityFactory::s_playerID = 2; // エンティティIDはCoordinatorが管理するため不要

/**
 * @brief ゲームワールドの静的な地面エンティティを生成する
 * @param coordinator - エンティティの生成と登録を行うCoordinator
 * @param position - 位置
 * @param scale - スケール
 * @return EntityID - 生成された地面エンティティID
 */
EntityID EntityFactory::CreateGround(Coordinator* coordinator, const XMFLOAT3& position, const XMFLOAT3& scale)
{
	// GameScene::CreateDemoEntities()から地面のロジックを移動
	ECS::EntityID ground = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	scale
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	0.0f, // 静的オブジェクト
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(scale.x / 2.0f, scale.y / 2.0f, scale.z / 2.0f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_STATIC
		)
	);
	return ground;
}

/**
 * @brief プレイヤーエンティティを生成する
 * @param coordinator - エンティティの生成と登録を行うCoordinator
 * @param position - 初期位置
 * @return EntityID - 生成されたプレイヤーエンティティID
 */
EntityID EntityFactory::CreatePlayer(Coordinator * coordinator, const XMFLOAT3 & position)
{
	// 1. プレイヤーエンティティを生成
	EntityID player = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"player"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_MODEL,
			/* Color	*/	XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"Assets/Model/AD/modelkari.fbx",
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::None
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	1.0f,
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC
		),
		PlayerControlComponent(
			/* MoveSpeed	*/	5.0f
		)
	);

	// 2. プレイヤーに追従するカメラエンティティ生成（後続ステップ1-3の準備）
	// CameraComponentは、追従ロジックをCameraControlSystemに伝える情報を保持すると仮定
	EntityID playerCamera = coordinator->CreateEntity(
		CameraComponent(
			/* FocusID		*/	player,
			/* Offset		*/	XMFLOAT3(0.0f, 2.0f, -5.0f),
			/* FollowSpeed	*/	0.1f
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		)
	);

	// 3. PlayerControlComponentにカメラIDをリンク
	coordinator->GetComponent<PlayerControlComponent>(player).attachedCameraID = playerCamera;

	return player;
}

/**
 * [EntityID - CreateGameController]
 * @brief	ゲームの状態（GameMode）を管理するためのEntityを生成する
 * 
 * @param	[in] coordinator 
 * @return	生成されたEntityID
 */
EntityID EntityFactory::CreateGameController(Coordinator* coordinator)
{
	// GameStateComponentのみを持つEntity
	EntityID controller = coordinator->CreateEntity(
		GameStateComponent(GameMode::SCOUTING_MODE),	// 初期モードは偵察モード
		ItemTrackerComponent()
	);

	return controller;
}

/**
 * [EntityID - CreateCollectable]
 * @brief	回収アイテムEntityを生成
 * 
 * @param	[in] coordinator 
 * @param	[in] position 
 * @return	生成されたEntityID
 */
EntityID EntityFactory::CreateCollectable(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	ECS::EntityID entity = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"item"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(45.0f, 45.0f, 45.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
		),
		CollectableComponent(1.0f)
	);

	return entity;
}

/**
 * [EntityID - CreateGuard]
 * @brief	警備員Entityを生成
 *
 * @param	[in] coordinator
 * @param	[in] position
 * @return	生成されたEntityID
 */
EntityID EntityFactory::CreateGuard(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	EntityID guard = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"guard"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.5f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	1.0f,
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		GuardComponent(
			/* predictionDistance	*/	5.0f,
			/* isActive			*/	true,
			/* delayBeforeChase	*/	1.0f,
			/* chaseSpeed			*/	5.0f
		)
	);

	return guard;
}


/**
 * @brief 全てのデモ用エンティティを生成し、ECSに登録する (GameScene::Init()から呼ばれる)
 * @param coordinator - エンティティの生成と登録を行うCoordinator
 */
void EntityFactory::CreateAllDemoEntities(Coordinator* coordinator)
{
	// --- 1. 1つ目の地面（静的オブジェクト） ---
	CreateGround(coordinator,
		XMFLOAT3(0.0f, -0.5f, 0.0f),
		XMFLOAT3(20.0f, 0.2f, 20.0f));

	// --- ゲームコントローラーエンティティ ---
	EntityID gameControllerID = CreateGameController(coordinator);

	// --- 3. プレイヤーとカメラの生成 ---
	CreatePlayer(coordinator, XMFLOAT3(1.0f, 1.5f, 0.0f));
	
	// --- 警備員の生成 ---
	CreateGuard(coordinator, XMFLOAT3(0.0f, 1.5f, 5.0f));
	// --- アイテムの作成 ---
	CreateCollectable(
		/* Coordinator	*/	coordinator,
		/* Position		*/	XMFLOAT3(2.0f, 0.5f, 0.0f)
	);
	CreateCollectable(
		/* Coordinator	*/	coordinator,
		/* Position		*/	XMFLOAT3(-2.0f, 0.5f, 0.0f)
	);

	// アイテムをトラッカーに設定
	coordinator->GetComponent<ItemTrackerComponent>(gameControllerID).totalItems = 2;
}