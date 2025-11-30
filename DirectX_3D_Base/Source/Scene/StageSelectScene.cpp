/*****************************************************************//**
 * @file	StageSelectScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/13	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/StageSelectScene.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include "Scene/StageinformationScene.h"
#include "Systems/Input.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Components/UI/UIInteractableComponent.h>

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用

// ===== 静的メンバー変数の定義 =====
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

using namespace DirectX;

//仮の入力チェック関数
static  bool IsInputStart() {
	//ここに実際の入力チェックロジックが入る
	//今回は遷移テストのため、デバッグで一時的にtrueを返すなどしてもいい
	return false;
}


void StageSelectScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_unique<ECS::Coordinator>();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 4. デモ用Entityの作成 ---
	//ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());	
	m_selectbg = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUIBG"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.0f, 0.0f, -2.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(2.0f, 2.0f, 1.0f)
		),
		UIImageComponent(
			"UI_BG"
		)
	);

	m_selectcork = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUICORK"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.0f, 0.0f, -1.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(1.9f, 1.9f, 1.0f)
		),
		UIImageComponent(
			"UI_CORK"
		)
	);

	m_selectEntity1 = m_coordinator->CreateEntity(
			TagComponent(
				/*Tag*/   "SelectSceneUI1"
			),
			TransformComponent(
				/*Position*/    XMFLOAT3(-0.65f, 0.45f, 0.0f),
				/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
				/*Scale*/       XMFLOAT3(1.0f, 1.0f, 1.0f)
			),
			UIInteractableComponent(-1.0f, -1.0f),
			UIImageComponent(
				"UI_PAPER_1"
			)
		);

	m_selectEntity2 = m_coordinator->CreateEntity(
			TagComponent(
				/*Tag*/   "SelectSceneUI2"
			),
			TransformComponent(
				/*Position*/    XMFLOAT3(-0.05f, 0.6f, 0.0f),
				/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
				/*Scale*/       XMFLOAT3(0.8f, 0.6f, 1.0f)
			),
			UIInteractableComponent(-1.0f, -1.0f),
			UIImageComponent(
				"UI_PAPER_2"
			)
		);

	m_selectEntity3 = m_coordinator->CreateEntity(
			TagComponent(
				/*Tag*/   "SelectSceneUI3"
			),
			TransformComponent(
				/*Position*/    XMFLOAT3(-0.1f, 0.0f, 0.0f),
				/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
				/*Scale*/       XMFLOAT3(1.0f, 0.8f, 1.0f)
			),
			UIInteractableComponent(-1.0f, -1.0f),
			UIImageComponent(
				"UI_PAPER_3"
			)
		);

	m_selectEntity4 = m_coordinator->CreateEntity(
			TagComponent(
				/*Tag*/   "SelectSceneUI4"
			),
			TransformComponent(
				/*Position*/    XMFLOAT3(0.6f, 0.4f, 0.0f),
				/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
				/*Scale*/       XMFLOAT3(0.8f, 0.9f, 1.0f)
			),
			UIInteractableComponent(-1.0f, -1.0f),
			UIImageComponent(
				"UI_PAPER_1"
			)
		);

	m_selectEntity5 = m_coordinator->CreateEntity(
			TagComponent(
				/*Tag*/   "SelectSceneUI5"
			),
			TransformComponent(
				/*Position*/    XMFLOAT3(-0.55f, -0.4f, 0.0f),
				/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
				/*Scale*/       XMFLOAT3(1.2f, 1.0f, 1.0f)
			),
			UIInteractableComponent(-1.0f, -1.0f),
			UIImageComponent(
				"UI_PAPER_2"
			)
		);

	m_selectEntity6 = m_coordinator->CreateEntity(
			TagComponent(
				/*Tag*/   "SelectSceneUI6"
			),
			TransformComponent(
				/*Position*/    XMFLOAT3(0.5f, -0.3f, 0.0f),
				/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
				/*Scale*/       XMFLOAT3(1.8f, 1.8f, 1.0f)
			),
			UIInteractableComponent(-1.0f, -1.0f),
			UIImageComponent(
				"UI_PAPER_3"
			)
		);

	m_cursorEntity = m_coordinator->CreateEntity(
		TagComponent("Cursor"),
		TransformComponent(
			XMFLOAT3(0.0f, 0.0f, -5.0f), // Zを-5にして一番手前に表示
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.1f, 0.1f, 1.0f)   // カーソルのサイズ（適宜調整）
		),
		UIImageComponent(
			"UI_TEST3_OFF" 
		)
	);

	m_selectA = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUIA"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.4f, -0.9f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.3f, 0.1f, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent(
			"UI_TEST2"
		)
	);

	m_selectB = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUIB"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.8f, -0.9f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.3f, 0.1f, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent(
			"UI_TEST2"
		)
	);

	
	//ShowCursor(false);

	std::cout << "StageSelectScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;


}

void StageSelectScene::Uninit()
{
	// 1. ECS Systemの静的リソースを解放
	ECS::ECSInitializer::UninitECS();

	// Coordinatorの破棄（unique_ptrが自動的にdeleteを実行）
	m_coordinator.reset();

	// 静的ポインタをクリア
	s_coordinator = nullptr;

	std::cout << "StageSelectScene::Uninit() - ECS Destroyed." << std::endl;
}

void StageSelectScene::Update(float deltaTime)
{


	m_coordinator->UpdateSystems(deltaTime);

	if (m_selectEntity1 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity1);
		if (comp.isClicked)
		{
			std::cout << "Button 1 Clicked!" << std::endl;
			GameScene::SetStageNo(1);
			SceneManager::ChangeScene<StageinformationScene>(); // 遷移先を変えたい場合はここを変更
		}
	}

	// --- ボタン2の判定 ---
	if (m_selectEntity2 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity2);
		if (comp.isClicked)
		{
			std::cout << "Button 2 Clicked!" << std::endl;
			GameScene::SetStageNo(2);
			SceneManager::ChangeScene<StageinformationScene>();
		}
	}

	// --- ボタン3の判定 ---
	if (m_selectEntity3 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity3);
		if (comp.isClicked)
		{
			std::cout << "Button 3 Clicked!" << std::endl;
			GameScene::SetStageNo(3);
			SceneManager::ChangeScene<StageinformationScene>();
		}
	}

	// ボタン4, 5, 6 も同様に記述...
	if (m_selectEntity4 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity4);
		if (comp.isClicked)
		{
			std::cout << "Button 4 Clicked!" << std::endl;
			GameScene::SetStageNo(4);
			SceneManager::ChangeScene<StageinformationScene>();
		}
	}

	if (m_selectEntity5 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity5);
		if (comp.isClicked)
		{
			std::cout << "Button 5 Clicked!" << std::endl;
			GameScene::SetStageNo(5);
			SceneManager::ChangeScene<StageinformationScene>();
		}
	}

	if (m_selectEntity6 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity6);
		if (comp.isClicked)
		{
			std::cout << "Button 6 Clicked!" << std::endl;
			GameScene::SetStageNo(6);
			SceneManager::ChangeScene<StageinformationScene>();
		}
	}

	if (m_selectA != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectA);

		// システム側で「Aボタン押し＝クリック」に変換されているので、これで反応します
		if (comp.isClicked)
		{
			std::cout << "Button A Clicked!" << std::endl;
			// 遷移したいシーンへ（例：StageinformationScene）
			SceneManager::ChangeScene<StageinformationScene>();
		}
	}

	if (m_selectB != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectB);
		if (comp.isClicked)
		{
			std::cout << "Button B Clicked!" << std::endl;
			SceneManager::ChangeScene<TitleScene>();
		}
	}

	if (m_cursorEntity != ECS::INVALID_ENTITY_ID)
	{
		// 1. マウス座標を取得 (Windows API)
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(GetActiveWindow(), &p); // スクリーン座標をウィンドウ内座標へ変換

		// 2. ウィンドウサイズを取得 (本来は定数や設定クラスから取るべきですが、仮定値を入れます)
		// ※自分のプロジェクトの画面サイズに合わせて変更してください（例: 1280x720）
		float screenWidth = 1280.0f;
		float screenHeight = 720.0f;

		// 3. 座標変換 (ピクセル → -1.0～1.0 の座標系)
		// X: 0～1280 → -1.0～1.0
		float ndcX = (static_cast<float>(p.x) / screenWidth) * 2.0f - 1.0f;

		// Y: 0～720 → 1.0～-1.0 (Y軸は上がプラスなので反転が必要)
		float ndcY = -((static_cast<float>(p.y) / screenHeight) * 2.0f - 1.0f);

		// 4. Transformコンポーネントを更新
		auto& transform = m_coordinator->GetComponent<TransformComponent>(m_cursorEntity);

		// 補正（画像の中心がズレる場合はここで微調整）
		transform.position.x = ndcX;
		transform.position.y = ndcY;
	}

	//ECS::EntityID Select2Entity = ECS::FindFirstEntityWithComponent<UIInteractableComponent>(m_coordinator.get());


	//if (Select2Entity != ECS::INVALID_ENTITY_ID)
	//{
	//	const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(Select2Entity);

	//	if (comp.isClicked)
	//	{
	//		std::cout << "Button Clicked! -> GameScene" << std::endl;
	//		SceneManager::ChangeScene<GameScene>();
	//	}

	//}
	//ECS::EntityID Select3Entity = ECS::FindFirstEntityWithComponent<UIInteractableComponent>(m_coordinator.get());

	//if (Select3Entity != ECS::INVALID_ENTITY_ID)
	//{
	//	const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(Select3Entity);

	//	if (comp.isClicked)
	//	{
	//		std::cout << "Button Clicked! -> GameScene" << std::endl;
	//		SceneManager::ChangeScene<GameScene>();
	//	}

	//}

}

void StageSelectScene::Draw()
{
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}

}