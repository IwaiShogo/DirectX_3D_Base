/*****************************************************************//**
 * @file	EntityFactory.cpp
 * @brief	特定のエンティティ（プレイヤー、地面など）の生成ロジックを集約するヘルパークラスの実装
 *
 * @details
 * Componentの具体的な値設定をここに集約し、シーンコードをシンプルにする。
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/09	初回作成日
 * 			作業内容：	- 追加：エンティティ生成の静的実装を作成。
 *
 * @update	2025/11/08	最終更新日
 * 			作業内容：	- 追加：警備員AIの追加
 *
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/EntityFactory.h"
#include "ECS/ECSInitializer.h"
#include "ECS/ECS.h" // すべてのコンポーネントとCoordinatorにアクセスするため
#include "Main.h" // METERなどの定数にアクセス


using namespace ECS;
using namespace DirectX;

/**
 * @brief 全てのデモ用エンティティを生成し、ECSに登録する (GameScene::Init()から呼ばれる)
 * @param coordinator - エンティティの生成と登録を行うCoordinator
 */
void EntityFactory::CreateAllDemoEntities(Coordinator* coordinator)
{
	// 1. Map Entity (Game Controller) の作成と初期化
	//ECS::EntityID mapEntityID = coordinator->CreateEntity(
	//	TagComponent("game_controller"),
	//	MapComponent(), // 50x50のエリアでBSP/MSTを生成
	//	GameStateComponent(GameMode::SCOUTING_MODE),
	//	ItemTrackerComponent(),
	//	DebugComponent() // F1キーによるデバッグ機能のトグル用
	//);

	//// 2. MapGenerationSystemを呼び出し、BSP/MSTを生成
	//auto mapGenSystem = ECSInitializer::GetSystem<MapGenerationSystem>();
	//if (mapGenSystem)
	//{
	//	// MapGenerationSystem::GenerateMapがBSP/MSTを実行し、MapComponent.layoutを更新する
	//	mapGenSystem->CreateMap("ST_001");
	//}
	//else
	//{
	//	// マップ生成システムが見つからない場合は処理を中断
	//	// throw std::runtime_error("MapGenerationSystem is not registered!");
	//	return;
	//}

	//CreateUITestEntity(coordinator, { 0.9f, 0.0f }, { 0.5f, 1.0f }, "UI_TEST");
}

/**
 * @brief プレイヤーエンティティを生成する
 * @param coordinator - エンティティの生成と登録を行うCoordinator
 * @param position - 初期位置
 * @return EntityID - 生成されたプレイヤーエンティティID
 */
EntityID EntityFactory::CreatePlayer(Coordinator* coordinator, const XMFLOAT3& position)
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
			/* Color	*/	XMFLOAT4(0.3f, 0.3f, 1.0f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"M_PLAYER",
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::None
		),
		AnimationComponent(
			{
				"A_PLAYER_IDLE"
			}
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
			/* MoveSpeed	*/	9.0f
		)
	);

	auto& anim = coordinator->GetComponent<AnimationComponent>(player);
	anim.Play("A_PLAYER_IDLE");

	// 2. プレイヤーに追従するカメラエンティティ生成（後続ステップ1-3の準備）
	// CameraComponentは、追従ロジックをCameraControlSystemに伝える情報を保持すると仮定
	EntityID playerCamera = coordinator->CreateEntity(
		CameraComponent(
			/* FocusID		*/	player,
			/* Offset		*/	XMFLOAT3(0.0f, 3.0f, -5.0f),
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
			/* Color	*/	XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)
		),
		CollectableComponent(1.0f)
	);

	return entity;
}

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
			/* Color	*/	XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f)
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

/*
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
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_MODEL,
			/* Color	*/	XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"M_GUARD",
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
		GuardComponent(
			/* predictionDistance	*/	1.0f,
			/* isActive				*/	false,
			/* delayBeforeChase		*/	3.0f,
			/* chaseSpeed			*/	9.0f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC
		)
	);

	return guard;
}

/**
 * @brief ゲームワールドの静的な壁エンティティを生成する
 *
 * @param [in] coordinator - エンティティの生成と登録を行うCoordinator
 * @param [in] position - 位置
 * @param [in] scale - スケール
 * @param [in] color - ボックスの色（デフォルトでBackroomsの黄色）
 * @return EntityID - 生成された壁エンティティID
 */
EntityID ECS::EntityFactory::CreateWall(Coordinator* coordinator, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale, const float rotationY)
{
	ECS::EntityID entity = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"wall"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, rotationY, 0.0f),
			/* Scale	*/	scale
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX, // MESH_BOXで仮描画
			/* Color	*/	XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Accel	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass		*/	0.0f, // 【重要】質量0.0fで静的オブジェクト（動かない壁）として定義
			/* Friction	*/	0.5f,
			/* Restit.	*/	0.0f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(scale.x / 2.0f, scale.y / 2.0f, scale.z / 2.0f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_STATIC
		)
	);

	return entity;
}

/**
 * [EntityID - CreateGoal]
 * @brief	ゴールエンティティの生成
 *
 * @param	[in] coordinator
 * @param	[in] position
 * @param	[in] scale
 * @return	生成されたエンティティ
 */
EntityID ECS::EntityFactory::CreateGoal(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	EntityID goal = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"goal"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX, // MESH_BOXで仮描画
			/* Color	*/	XMFLOAT4(0.3f, 1.0f, 0.3f, 1.0f)
		),
		//ModelComponent(
		//	/* Path		*/	"Assets/Model/Item/yubiwakana.fbx",
		//	/* Scale	*/	0.1f,
		//	/* Flip		*/	Model::None
		//),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	0.0f, // 静的オブジェクト
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_TRIGGER
		)
	);

	return goal;
}

EntityID ECS::EntityFactory::CreateTaser(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	ECS::EntityID taser = coordinator->CreateEntity(
		TagComponent(
			/* Tag */    "taser"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,                   // 立方体を指定
			/* Color	*/	XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) // 青色 (R,G,B,A)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	1.0f,
			/* Friction		*/	0.5f,
			/* Restitution	*/	0.1f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f), // Scale 1.0の半分
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC
		)
	);
	return taser;
}

EntityID ECS::EntityFactory::CreateOneShotSoundEntity(Coordinator* coordinator, const std::string& assetID, float volume)
{
	EntityID entity = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"SE"
		),
		SoundComponent(),
		OneShotSoundComponent(
			/* AssetID	*/	assetID,
			/* Volume	*/	volume
		)
	);

	return entity;
}

EntityID ECS::EntityFactory::CreateLoopSoundEntity(Coordinator* coordinator, const std::string& assetID, float volume)
{
	EntityID entity = coordinator->CreateEntity(
		TagComponent("BGM"),
		SoundComponent()//サウンドコンポーネント
	);

	//コンポーネント値設定
	auto& sound = coordinator->GetComponent<SoundComponent>(entity);
	sound.assetID = assetID;
	sound.type = SoundType::BGM;
	sound.RequestPlay(volume, XAUDIO2_LOOP_INFINITE);
	return entity;
}

EntityID ECS::EntityFactory::CreateUITestEntity(Coordinator* coordinator, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size, const std::string& assetID)
{
	EntityID entity = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ui"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(position.x, position.y, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(size.x, size.y, 1.0f)
		),
		UIImageComponent(
			/* AssetID	*/	assetID
		)
	);

	return entity;
}

EntityID ECS::EntityFactory::CreateTitleSceneEntity(Coordinator* coordinator)
{
	EntityID entity = coordinator->CreateEntity(
		TitleSceneComponent{}
	);
	return entity;
}

EntityID ECS::EntityFactory::CreateGameSceneEntity(Coordinator* coordinator)
{
	EntityID entity = coordinator->CreateEntity(
		GameSceneComponent{}
	);
	return entity;
}

void ECS::EntityFactory::GenerateStageFromConfig(ECS::Coordinator* coordinator, const std::string& stageId)
{
	// 1. ゲームコントローラー（MapComponent持ち）を作る
	ECS::EntityID mapEntityID = coordinator->CreateEntity(
		TagComponent("game_controller"),
		MapComponent(),
		GameStateComponent(GameMode::SCOUTING_MODE),
		ItemTrackerComponent(),
		DebugComponent()
	);

	// 2. MapGenerationSystem を呼び出して、指定されたID (ST_006など) でマップを作る
	auto mapGenSystem = ECSInitializer::GetSystem<MapGenerationSystem>();
	if (mapGenSystem)
	{
		// ★ここで引数の stageId を使うことで、正しく難易度が変わります
		mapGenSystem->CreateMap(stageId);
	}

	// 3. プレイヤー作成 (マップ生成側でやっていない場合)
//	CreatePlayer(coordinator, XMFLOAT3(2.5f, 0.0f, 2.5f));
}
