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
			/* MeshType	*/	MESH_NONE,
			/* Color	*/	XMFLOAT4(0.3f, 0.3f, 1.0f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"M_PLAYER",
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::None
		),
		AnimationComponent(
			{
				"A_PLAYER_IDLE",
				"A_PLAYER_RUN",
				"A_PLAYER_CAUGHT"
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
			/* Offset		*/	XMFLOAT3(0.0f, 0.5f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC
		),
		PlayerControlComponent(
			/* MoveSpeed	*/	9.0f
		),
		PointLightComponent(0.9f, 0.4f, 1.0f, 5.0f, { 0.0f, 0.5f, 0.0f }),
		EffectComponent(
			"EFK_ALERT",
			true,
			false,
			{ 0.0f, 1.3f, 0.0f },
			0.1f
		)
	);

	auto& anim = coordinator->GetComponent<AnimationComponent>(player);
	anim.Play("A_PLAYER_IDLE");

	auto& ctrl = coordinator->GetComponent<PlayerControlComponent>(player);
	ctrl.animState = PlayerAnimState::Idle;


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
 * [EntityID - CreateCollectable]
 * @brief	回収アイテムEntityを生成
 *
 * @param	[in] coordinator
 * @param	[in] position
 * @return	生成されたEntityID
 */
EntityID EntityFactory::CreateCollectable(Coordinator* coordinator, const DirectX::XMFLOAT3& position, int orderIndex, const std::string& itemID)
{
	std::string modelPath = "M_TREASURE1";
	DirectX::XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	if (itemID == "Takara_Daiya") { modelPath = "M_TREASURE1"; }
	else if (itemID == "Takara_Crystal") { modelPath = "M_TREASURE2"; }
	else if (itemID == "Takara_Yubiwa") { modelPath = "M_TREASURE3"; }
	else if (itemID == "Takara_Kaiga1") { modelPath = "M_TREASURE4"; }
	else if (itemID == "Takara_Kaiga2") { modelPath = "M_TREASURE5"; }
	else if (itemID == "Takara_Kaiga3") { modelPath = "M_TREASURE6"; }
	else if (itemID == "Takara_Doki") { modelPath = "M_TREASURE7"; }
	else if (itemID == "Takara_Tubo_Blue") { modelPath = "M_TREASURE8"; }
	else if (itemID == "Takara_Tubo_Gouyoku") { modelPath = "M_TREASURE9"; }
	else if (itemID == "Takara_Dinosaur") { modelPath = "M_TREASURE10"; }
	else if (itemID == "Takara_Ammonite") { modelPath = "M_TREASURE11"; }
	else if (itemID == "Takara_Dinosaur_Foot") { modelPath = "M_TREASURE12"; }

	ECS::EntityID entity = coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"item"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_NONE,
			/* Color	*/	color
		),
		ModelComponent(
			/* Path		*/	modelPath,
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::None
		),
		CollectableComponent(1.0f, orderIndex, itemID),
		//浮遊コンポーネント
		FloatingComponent(
			/* Amplitude */ 0.5f,     // 上下 0.5 の範囲で揺れる
			/* Speed     */ 2.0f,     // 速度
			/* InitialY  */ position.y - 1 // 基準となる高さ（配置位置）
		),
		PointLightComponent(0.0f, 5.0f, 0.0f, 5.0f),
		EffectComponent(
			"EFK_TREASURE_GLOW",
			true,
			true,
			{ 0.0f, 0.0f, 0.0f },
			0.3f
		)
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
		TagComponent("ground"),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	scale
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"M_CORRIDOR",
			/* Scale	*/	0.25f,
			/* Flip		*/	Model::None
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
		AnimationComponent(
			{
				"A_GUARD_RUN",
				"A_GUARD_WALK",
				"A_GUARD_ATTACK"
			}
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
			/* delayBeforeChase		*/	0.1f,
			/* chaseSpeed			*/	25.0f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.5f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC
		),
		PointLightComponent(5.0f, 0.0f, 0.0f, 7.0f, { 0.0f, 0.5f, 0.3f })
	);

	auto& anim = coordinator->GetComponent<AnimationComponent>(guard);
	anim.Play("A_GUARD_RUN");

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
		ModelComponent(
			/* Path		*/	"M_WALL",
			/* Scale	*/	0.25f,
			/* Flip		*/	Model::None
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

EntityID ECS::EntityFactory::CreateTaser(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	ECS::EntityID taser = coordinator->CreateEntity(
		TagComponent(
			/* Tag */    "taser"
		),
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(3.0f, 3.0f, 3.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_NONE,                   // 立方体を指定
			/* Color	*/	XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) // 青色 (R,G,B,A)
		),
		ModelComponent(
			/* Path		*/	"M_TASER",
			/* Scale	*/	0.25f,
			/* Flip		*/	Model::None
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	1.0f,
			/* Friction		*/	0.5f,
			/* Restitution	*/	0.1f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(2.5f, 2.5f, 2.5f), // Scale 1.0の半分
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC
		)
	);
	return taser;
}



EntityID ECS::EntityFactory::CreateMapGimmick(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	ECS::EntityID gimmick = coordinator->CreateEntity(
		TagComponent(
			/* Tag */    "map_gimmick"
		),
		TransformComponent(
			/* Position */	position,
			/* Rotation */	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale */	XMFLOAT3(3.0f, 3.0f, 3.0f)
		),
		RenderComponent(
			/* MeshType */	MESH_NONE,
			/* Color */	XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity */		XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration */	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass */			0.0f,
			/* Friction */		0.5f,
			/* Restitution */	0.1f
		),
		CollisionComponent(
			/* Size */			XMFLOAT3(2.5f, 2.5f, 2.5f),
			/* Offset */		XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType */	COLLIDER_STATIC
		)
	);

	return gimmick;
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

EntityID ECS::EntityFactory::CreateTitleSceneEntity(Coordinator* coordinator)
{
	EntityID entity = coordinator->CreateEntity(
		TitleControllerComponent{}
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

EntityID ECS::EntityFactory::CreateBasicCamera(Coordinator* coordinator, const DirectX::XMFLOAT3& position)
{
	EntityID entity = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		BasicCameraComponent(),
		PointLightComponent(2.0f, 1.8f, 1.4f, 30.0f, { 0.0f, 0.0f, 0.0f })
	);

	return entity;
}

EntityID EntityFactory::CreateOneShotEffect(Coordinator* coordinator, const std::string& assetID, const DirectX::XMFLOAT3& position, float duration, float scale)
{
	EntityID entity = coordinator->CreateEntity(
		TagComponent("effect"),
		TransformComponent(
			position,
			{ 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f }
		),
		// エフェクト再生コンポーネント
		EffectComponent(
			assetID,
			false,  // ループなし
			true,   // 生成時に即再生
			{ 0,0,0 },
			scale
		),
		// 寿命コンポーネント (時間が来たらEntityごと消滅)
		LifeTimeComponent(duration)
	);

	return entity;
}

EntityID ECS::EntityFactory::CreateDoor(Coordinator* coordinator, const DirectX::XMFLOAT3& position, float rotationY, bool isEntrance)
{
	EntityID door = coordinator->CreateEntity(
		TagComponent("door"),
		TransformComponent(
			position,
			XMFLOAT3(0.0f, rotationY, 0.0f),
			XMFLOAT3(1.0f, 1.0f, 1.0f) // モデルに合わせてスケール調整
		),
		RenderComponent(MESH_MODEL, XMFLOAT4(1, 1, 1, 1)),
		ModelComponent("M_DOOR", 0.25f, Model::None), // "M_DOOR"はロード済みとする
		AnimationComponent({ "A_DOOR_OPEN", "A_DOOR_CLOSE" }), // アニメーション名
		DoorComponent(isEntrance, isEntrance), // 入口ならロックなし、出口ならロックあり
		CollisionComponent(
			XMFLOAT3(5.0f, 5.0f, 0.5f), // 壁と同じくらいのサイズ
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			COLLIDER_STATIC // 閉まっている時は壁扱い
		),
		RigidBodyComponent(
			XMFLOAT3(0.0f, 0.0f, 0.0f), // 速度
			XMFLOAT3(0.0f, 0.0f, 0.0f), // 加速度
			0.0f,  // 質量0 = 静的（押されても動かない）
			0.5f,  // 摩擦
			0.0f   // 反発
		),
		EffectComponent(
			"EFK_DOOR",
			true,
			true,
			{ 0.0f, 0.0f, 0.0f },
			0.5f
		)
	);

	// 初期アニメーション（閉じた状態）
	auto& anim = coordinator->GetComponent<AnimationComponent>(door);
	anim.Play("A_DOOR_CLOSE", false, 100.0f);

	return door;
}

EntityID ECS::EntityFactory::CreateEnemySpawner(Coordinator* coordinator, const DirectX::XMFLOAT3& position, float delayTime)
{
	EntityID spawner = coordinator->CreateEntity(
		TagComponent("spawner"),
		TransformComponent(position, { 0,0,0 }, { 1,1,1 }),

		// RenderComponentは無し（見えない）
		// RigidBodyも無し（物理干渉しない）

		// スポーン情報 (例: 3秒後にGuardを出す)
		EnemySpawnComponent(EnemyType::Guard, delayTime, 1.5f)
	);
	return spawner;
}


ECS::EntityID EntityFactory::CreateTeleporter(ECS::Coordinator* coordinator, DirectX::XMFLOAT3 position)
{
	return coordinator->CreateEntity(
		TransformComponent(position, { 0,0,0 }, { 2.5f, 0.1f, 2.5f }),
		// 仕様：三人称モード（ACTION_MODE）では見えないため、MESH_NONEを指定
		RenderComponent(MESH_NONE, DirectX::XMFLOAT4(0, 0.8f, 1.0f, 1.0f)),
		// 接触判定：物理衝突はせず、イベントのみ発生させる
		CollisionComponent(DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), { 0,0,0 }, COLLIDER_TRIGGER),
		TagComponent("teleporter"),
		TeleportComponent()
	);
}
