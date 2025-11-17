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
#include "ECS/ResourceManager.h"

using namespace ECS;
using namespace DirectX;

// 静的メンバ変数の定義 (必要に応じて追加)
// const static ECS::EntityID EntityFactory::s_playerID = 2; 
// // エンティティIDはCoordinatorが管理するため不要
ECS::EntityID EntityFactory::s_itemGetUI_ID = ECS::INVALID_ENTITY_ID;
ECS::EntityID EntityFactory::s_inventoryItemUI_ID = ECS::INVALID_ENTITY_ID;
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
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.3f, 0.3f, 1.0f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"Assets/Model/Player/hadakakaihi.fbx",
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
			/* MoveSpeed	*/	10.0f
		)
	);

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
		GuardComponent(
			/* predictionDistance	*/	1.0f,
			/* isActive				*/	false,
			/* delayBeforeChase		*/	5.0f,
			/* chaseSpeed			*/	4.0f
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
		ModelComponent(
			/* Path		*/	"Assets/Model/Item/yubiwakana.fbx",
			/* Scale	*/	0.1f,
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
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_TRIGGER
		)
	);

	return goal;
}


/**
 * @brief 全てのデモ用エンティティを生成し、ECSに登録する (GameScene::Init()から呼ばれる)
 * @param coordinator - エンティティの生成と登録を行うCoordinator
 */
void EntityFactory::CreateAllDemoEntities(Coordinator* coordinator)
{
	// 1. Map Entity (Game Controller) の作成と初期化
	// GameStateComponent, ItemTrackerComponent, DebugComponent はこのEntityに付与
	



	ECS::EntityID mapEntityID = coordinator->CreateEntity(
		TagComponent("game_controller"),
		MapComponent(), // 50x50のエリアでBSP/MSTを生成
		GameStateComponent(GameMode::SCOUTING_MODE),
		ItemTrackerComponent(),
		DebugComponent() // F1キーによるデバッグ機能のトグル用
	);

	// 2. MapGenerationSystemを呼び出し、BSP/MSTを生成
	auto mapGenSystem = ECSInitializer::GetSystem<MapGenerationSystem>();
	if (mapGenSystem)
	{
		// MapGenerationSystem::GenerateMapがBSP/MSTを実行し、MapComponent.layoutを更新する
		mapGenSystem->InitMap();
	}
	else
	{
		// マップ生成システムが見つからない場合は処理を中断
		// throw std::runtime_error("MapGenerationSystem is not registered!");
		return;
	}

	CreateLuffyUI(coordinator, XMFLOAT2(0.8f, 0.5f));

	//作成
	bool luffyLoad = ResourceManager::LoadTexture(100, "Assets/Texture/1.png");

	if (!luffyLoad)
	{
		MessageBox(nullptr, "るふぃのUIテクスチャのロードに失敗しました", "エラー", MB_OK);
	}

	    //アイテム取得UI（非表示で生成）
		//CreateItemGetUI(coordinator);
		//アイテム取得UI用のテクスチャをロード（ID: 101 と仮定）
		bool itemGetUILoad = ResourceManager::LoadTexture(101, "Assets/Texture/2.png"); // このパスは仮です
		if (!itemGetUILoad)
		{
			MessageBox(nullptr, "アイテム取得UIのテクスチャ(ID:101)のロードに失敗しました", "エラー", MB_OK);
		}

		// 1. インベントリUIの位置を定義（仮に画面右上に設定）
	// ※ 座標系（ピクセル or 0-1）は既存のUIに合わせてください
		DirectX::XMFLOAT2 inventoryPos = DirectX::XMFLOAT2(0.9f, 0.1f); // 仮の座標 (右上)
		DirectX::XMFLOAT2 inventorySize = DirectX::XMFLOAT2(0.1f, 0.1f); // 仮のサイズ

		// 2. インベントリの「枠」を生成 (常に表示)
		CreateInventoryFrameUI(coordinator, inventoryPos, inventorySize);

		// 3. インベントリの「中身」を生成 (最初は非表示)
		CreateInventoryItemUI(coordinator, inventoryPos, inventorySize);

		// 4. インベントリUI用のテクスチャをロード
		// (ID: 102 を「枠」と仮定)
		bool frameLoad = ResourceManager::LoadTexture(102, "Assets/Texture/3.png"); // 仮パス
		if (!frameLoad)
		{
			MessageBox(nullptr, "インベントリ枠のテクスチャ(ID:102)のロードに失敗", "エラー", MB_OK);
		}

		// (ID: 103 を「中身」と仮定)
		bool itemLoad = ResourceManager::LoadTexture(103, "Assets/Texture/2.png"); // 仮パス
		if (!itemLoad)
		{
			MessageBox(nullptr, "インベントリ中身のテクスチャ(ID:103)のロードに失敗", "エラー", MB_OK);
		}

	std::cout << "GameScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;
	
}

EntityID EntityFactory::CreateDemoUI(Coordinator* coordinator)
{
	uint32_t demoTextureID = 1;

	ECS::EntityID uiElement = coordinator->CreateEntity(
		TagComponent("demo_ui"),//オプション
		UIComponent(
			/*TextureID*/demoTextureID,
			/*Position*/DirectX::XMFLOAT2(150.0f, 100.0f),
			/*Size    */DirectX::XMFLOAT2(200.0f, 100.0f),
			/*Color   */DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
			/*Depth   */0.5f
		)
	);

	return uiElement;
}

EntityID ECS::EntityFactory::CreateLuffyUI(Coordinator* coordinator, const DirectX::XMFLOAT2& position)
{
	uint32_t luffyTextureID = 100;

	DirectX::XMFLOAT2 luffySize = DirectX::XMFLOAT2(0.5f, 0.5f);
	ECS::EntityID luffyElement = coordinator->CreateEntity(
		TagComponent("luffy_ui"),//タグをつける
		UIComponent(
			/*TextureID*/luffyTextureID,
			/*Position*/position,
			/*Size    */luffySize,
			/*Color   */DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),//白
			/*Depth   */0.1f
		)
	);
	return EntityID();
}

/**
 * @brief アイテム取得UIエンティティを生成する (初期状態: 非表示)
 * @param coordinator
 * @return EntityID
 */
EntityID EntityFactory::CreateItemGetUI(Coordinator* coordinator)
{
	uint32_t itemGetTextureID = 101; // アイテム取得UI用のテクスチャID（ResourceManagerでロードする必要あり）

	// 画面中央に配置（仮）
	// ※ Main.hなどに画面サイズの定数があればそれを使用してください
	DirectX::XMFLOAT2 screenCenter = DirectX::XMFLOAT2(0.8f, 0.1f); // 仮に 1280x720 の中央
	DirectX::XMFLOAT2 uiSize = DirectX::XMFLOAT2(0.2f, 0.2f); // 仮のサイズ

	ECS::EntityID uiElement = coordinator->CreateEntity(
		TagComponent("item_get_ui"), // 識別用のタグ
		UIComponent(
			/*TextureID*/ itemGetTextureID,
			/*Position*/  screenCenter,
			/*Size    */  uiSize,
			/*Color   */  DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 白
			/*Depth   */  0.2f, // 描画深度
			/*IsVisible*/ false // 最初は非表示
		)
	);
	//生成したIDを静的メンバーにキャッシュ
		s_itemGetUI_ID = uiElement;
	return uiElement;
}

/**
 * @brief インベントリの「枠」UIを生成する (常に表示)
 * @param coordinator
 * @param position - 画面右上の座標など
 * @param size - UIのサイズ
 * @return EntityID
 */
EntityID EntityFactory::CreateInventoryFrameUI(Coordinator* coordinator, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size)
{
	uint32_t frameTextureID = 102; // 「枠」用のテクスチャID (仮)

	ECS::EntityID uiElement = coordinator->CreateEntity(
		TagComponent("inventory_frame_ui"),
		UIComponent(
			/*TextureID*/ frameTextureID,
			/*Position*/  position,
			/*Size    */  size,
			/*Color   */  DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 白
			/*Depth   */  0.8f, // 描画深度 (奥)
			/*IsVisible*/ true  // ★常に表示
		)
	);
	return uiElement;
}

/**
 * @brief インベントリの「中身」UIを生成する (初期状態: 非表示)
 * @param coordinator
 * @param position - 枠と同じ座標
 * @param size - 枠と同じサイズ
 * @return EntityID
 */
EntityID EntityFactory::CreateInventoryItemUI(Coordinator* coordinator, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size)
{
	uint32_t itemTextureID = 103; // 「中身」用のテクスチャID (仮)

	ECS::EntityID uiElement = coordinator->CreateEntity(
		TagComponent("inventory_item_ui"),
		UIComponent(
			/*TextureID*/ itemTextureID,
			/*Position*/  position,
			/*Size    */  size,
			/*Color   */  DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 白
			/*Depth   */  0.7f, // 描画深度 (枠より少し手前)
			/*IsVisible*/ false // ★最初は非表示
		)
	);

	// 生成したIDを静的メンバーにキャッシュ
	s_inventoryItemUI_ID = uiElement;

	return uiElement;
}