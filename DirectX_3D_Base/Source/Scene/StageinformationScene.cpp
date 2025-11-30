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
#include "Scene/StageinformationScene.h"
#include "Scene/GameScene.h"
#include "ECS/Components/ScoreManager.h"
#include "Systems/Input.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Components/UI/UIInteractableComponent.h>

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
#include <string>
#include <iomanip> // 桁数合わせ用

// ===== 静的メンバー変数の定義 =====
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* StageinformationScene::s_coordinator = nullptr;

using namespace DirectX;

//仮の入力チェック関数
static  bool IsInputStart() {
	//ここに実際の入力チェックロジックが入る
	//今回は遷移テストのため、デバッグで一時的にtrueを返すなどしてもいい
	return false;
}

void StageinformationScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_unique<ECS::Coordinator>();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 4. デモ用Entityの作成 ---
	//ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());	

	m_bg = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "InformationBG"
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

	m_MapImage = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUIMapImage"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(-0.5, 0.35f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.9f, 1.2f, 1.0f)
		),
		UIImageComponent(
			"UI_STAGEMAP"
		)
	);



	

	m_Treasure = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUITREASURE"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.4f, 0.65f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.6f, 0.55f, 1.0f)
		),
		UIImageComponent(
			"UI_BG"
		)
	);
	
	m_Security = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUISECURITY"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.4f, 0.0f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.6f, 0.55f, 1.0f)
		),
		UIImageComponent(
			"UI_BG"
		)
	);

	m_OK = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUIOK"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.4f, -0.55f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.6f, 0.2f, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent(
			"UI_BG"
		)
	);

	m_BACK = m_coordinator->CreateEntity(
		TagComponent(
			/*Tag*/   "SelectSceneUIBACK"
		),
		TransformComponent(
			/*Position*/    XMFLOAT3(0.4f, -0.85f, 0.0f),
			/*Rotation*/    XMFLOAT3(0.0f, 0.0f, 0.0f),
			/*Scale*/       XMFLOAT3(0.6f, 0.2f, 1.0f)
		),
		UIInteractableComponent(-1.0f, -1.0f),
		UIImageComponent(
			"UI_BG"
		)
	);

	


	// 配置座標の基準 (BestTimeラベルの横あたりに設定)
	float startX = -0.7f;
	float startY = -0.7f;
	float stepX = 0.1f; // 数字の間隔

	for (int i = 0; i < 5; ++i)
	{
		std::string initAsset = (i == 2) ? "UI_COLON" : "UI_NUM_0"; // 3番目はコロン、他は0で初期化

		m_timeDigits[i] = m_coordinator->CreateEntity(
			TagComponent("TimeDigit" + std::to_string(i)),
			TransformComponent(
				XMFLOAT3(startX + (i * stepX), startY, 0.0f),
				XMFLOAT3(-0.5f, 0.0f, 0.0f),
				XMFLOAT3(0.1f, 0.2f, 1.0f) // 数字のサイズ
			),
			UIImageComponent(initAsset)
		);
	}

	// --- 保存されているベストタイムを取得して表示に反映 ---
	// GameScene::s_StageNo で現在選択中のステージ番号が取れる前提
	float bestTime = ScoreManager::GetBestTime(GameScene::s_StageNo);
	UpdateTimeDisplay(bestTime);

}

void StageinformationScene::Uninit()
{
	// 1. ECS Systemの静的リソースを解放
	ECS::ECSInitializer::UninitECS();

	// Coordinatorの破棄（unique_ptrが自動的にdeleteを実行）
	m_coordinator.reset();

	// 静的ポインタをクリア
	s_coordinator = nullptr;

	std::cout << "StageSelectScene::Uninit() - ECS Destroyed." << std::endl;
}

void StageinformationScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);

	if (m_OK != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_OK);
		if (comp.isClicked)
		{
			std::cout << "Button OK Clicked!" << std::endl;
			//GameScene::SetStageNo(6);
			SceneManager::ChangeScene<GameScene>();
		}
	}

	if (m_BACK != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_BACK);
		if (comp.isClicked)
		{
			std::cout << "Button BACK Clicked!" << std::endl;
			//GameScene::SetStageNo(6);
			SceneManager::ChangeScene<StageSelectScene>();
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

}

void StageinformationScene::Draw()
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

void StageinformationScene::UpdateTimeDisplay(float time)
{
	// タイムが0なら 00:00 のまま
	int minutes = 0;
	int seconds = 0;

	if (time > 0.0f)
	{
		minutes = static_cast<int>(time) / 60;
		seconds = static_cast<int>(time) % 60;
	}

	// 各桁の数値を計算
	int m10 = minutes / 10;
	int m1 = minutes % 10;
	int s10 = seconds / 10;
	int s1 = seconds % 10;

	// 画像アセットIDを更新
	// 配列: [0]分10, [1]分1, [2]コロン, [3]秒10, [4]秒1

	// 分の10の位
	m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[0]).assetID = "UI_NUM_" + std::to_string(m10);
	// 分の1の位
	m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[1]).assetID = "UI_NUM_" + std::to_string(m1);

	// [2]はコロンなので変更なし

	// 秒の10の位
	m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[3]).assetID = "UI_NUM_" + std::to_string(s10);
	// 秒の1の位
	m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[4]).assetID = "UI_NUM_" + std::to_string(s1);
}
