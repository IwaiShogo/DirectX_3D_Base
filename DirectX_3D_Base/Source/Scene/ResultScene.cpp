/*****************************************************************//**
 * @file	ResultScene.cpp
 * @brief
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author
 * ------------------------------------------------------------
 *
 * @date	2025/11/08	初回作成日
 * 			作業内容：	-
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード  =====
#include "Scene/ResultScene.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIButtonComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include <ECS/Systems/Core/ResultControlSystem.h>

using namespace DirectX;
using namespace std;
using namespace ECS;

//仮の入力チェック関数
bool IsInputTitle() { return false; }
bool ResultScene::isClear = false;
int ResultScene::finalItenCount = 0;
ResultData ResultScene::s_resultData = {};


//===== ResultScene メンバー関数の実装 =====

void ResultScene::Init()
{
	// ECS初期化
	m_coordinator = std::make_shared<ECS::Coordinator>();
	ECS::ECSInitializer::InitECS(m_coordinator);
	
	// カメラ
	ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), {0.0f, 0.0f, 0.0f});

	// クリア判定
	bool isClear = s_resultData.isCleared;

	// ============================================================
	// ゲームクリア時のUI
	// ============================================================
	if (isClear)
	{
		// 背景
		m_coordinator->CreateEntity(
			TransformComponent(
				{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
				{ 0, 0, 0 },
				{ SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
			),
			UIImageComponent(
				"BG_GAME_CLEAR",
				0.0f,
				true,
				{ 1.0f, 1.0f, 1.0f, 1.0f }
			)
		);

		// タイトル文字
		m_coordinator->CreateEntity(
			TransformComponent(
				{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.1f, 0 },
				{ 0, 0, 0 },
				{ 760, 96, 1 }
			),
			UIImageComponent(
				"UI_GAME_CLEAR",
				0.0f,
				true,
				{ 1.0f, 1.0f, 1.0f, 1.0f }
			)
		);

		// A. タイム表示 (Font画像使用)
		// "TIME: " の文字画像があればそれを置く。ここでは数字のみ配置例
		float timeY = SCREEN_HEIGHT * 0.35f;
		CreateTimeDisplay(s_resultData.clearTime, { SCREEN_WIDTH * 0.4f, timeY });

		// B. 星評価 (3つ)
		// 条件チェック
		bool stars[3] = {
			!s_resultData.wasSpotted,       // 1. ノーアラート
			s_resultData.collectedAllOrdered, // 2. 順序コンプ (または全回収)
			s_resultData.clearedInTime      // 3. タイムアタック
		};

		float starY = SCREEN_HEIGHT * 0.30f;
		float starGap = 120.0f;
		float startX = SCREEN_WIDTH * 0.55f;

		for (int i = 0; i < 3; ++i)
		{
			// 下地（枠）としての星 (Off)
			m_coordinator->CreateEntity(
				TransformComponent({ startX, starY + i * starGap, 0 }, { 0,0,0 }, { 100, 100, 1 }),
				UIImageComponent("ICO_STAR_OFF", 1.0f, true, { 1,1,1,1 })
			);

			// 獲得した星 (On) - 最初は非表示 or スケール0にしてアニメーションさせる
			if (stars[i])
			{
				EntityID star = m_coordinator->CreateEntity(
					TransformComponent({ startX, starY + i * starGap, 0 }, { 0,0,0 }, { 0, 0, 1 }), // 初期サイズ0
					UIImageComponent("ICO_STAR_ON", 2.0f, true, { 1,1,1,1 }),
					TagComponent("AnimStar") // アニメーション用タグ
				);
				// 何番目の星か識別するためスケールZ等にインデックスを埋め込むか、専用コンポーネントを作る
				// ここではTagComponentと作成順(EntityID順)を利用してSystem側で制御します
			}
		}

		// C. スタンプ (EXCELLENT!)
		if (stars[0] && stars[1] && stars[2]) // 3つ星ならスタンプ
		{
			m_coordinator->CreateEntity(
				TransformComponent({ SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.4f, 0 }, { 0,0,0 }, { 500, 500, 1 }), // 初期巨大
				UIImageComponent("ICO_STAMP1", 3.0f, true, { 1,1,1,0 }), // 最初は透明
				TagComponent("AnimStamp")
			);
		}
		else
		{
			m_coordinator->CreateEntity(
				TransformComponent({ SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.4f, 0 }, { 0,0,0 }, { 500, 500, 1 }), // 初期巨大
				UIImageComponent("ICO_STAMP2", 3.0f, true, { 1,1,1,0 }), // 最初は透明
				TagComponent("AnimStamp")
			);
		}
	}
	// ============================================================
	// ゲームオーバー時のUI
	// ============================================================
	else
	{
		// 背景
		m_coordinator->CreateEntity(
			TransformComponent(
				{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
				{ 0, 0, 0 },
				{ SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
			),
			UIImageComponent(
				"BG_GAME_OVER",
				0.0f,
				true,
				{ 1.0f, 1.0f, 1.0f, 1.0f }
			)
		);

		// タイトル文字
		m_coordinator->CreateEntity(
			TransformComponent(
				{ SCREEN_WIDTH * 0.3f, SCREEN_HEIGHT * 0.15f, 0 },
				{ 0, 0, 0 },
				{ 680, 96, 1 }
			),
			UIImageComponent(
				"UI_GAME_OVER",
				0.0f,
				true,
				{ 1.0f, 1.0f, 1.0f, 1.0f }
			)
		);

		// 獲得アイテム一覧 (取れたものは明るく、取れなかったものは暗く)
		int total = s_resultData.totalItems;
		int collected = s_resultData.collectedCount;

		float itemY = SCREEN_HEIGHT * 0.5f;
		float itemW = 64.0f;
		float startX = SCREEN_WIDTH / 2 - (total * itemW) / 2 + itemW / 2;

		for (int i = 0; i < total; ++i)
		{
			bool isGot = (i < collected); // 簡易判定(個数)

			m_coordinator->CreateEntity(
				TransformComponent({ startX + i * (itemW + 10), itemY, 0 }, { 0,0,0 }, { itemW, itemW, 1 }),
				UIImageComponent(
					"ICO_TREASURE1", // 本来はIDに対応した画像
					1.0f,
					true,
					isGot ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f)
				)
			);
		}
	}

	// 3. ボタン類 (共通)
	CreateButtons();

	// 4. カーソル
	m_coordinator->CreateEntity(
		TransformComponent({ 0,0,0 }, { 0,0,0 }, { 64, 64, 1 }),
		UIImageComponent("ICO_CURSOR", 5.0f),
		UICursorComponent()
	);

	std::cout << "ResultScene::Init() - ResultUISystem Ready." << std::endl;
}

void ResultScene::Uninit()
{
	//このシーンで作成したエンティティを破棄
	std::cout << "ResultScene::Uninit() - Result Scene Systems Destroyed." << std::endl;
}

void ResultScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);
}

void ResultScene::Draw()
{
	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render(true);
	}

	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render(false);
	}
}

// ヘルパー: タイム表示作成
void ResultScene::CreateTimeDisplay(float time, DirectX::XMFLOAT2 pos)
{
	int tInt = (int)(time * 10.0f); // 0.1秒単位
	int min = (tInt / 600) % 100;
	int sec = (tInt / 10) % 60;
	int dsec = tInt % 10;

	int digits[] = {
		min / 10, min % 10,
		11, // :
		sec / 10, sec % 10,
		12, // .
		dsec
	};

	float w = 40.0f, h = 60.0f;
	float startX = pos.x - (7 * w) / 2;

	for (int i = 0; i < 7; ++i) {
		EntityID d = m_coordinator->CreateEntity(
			TransformComponent({ startX + i * w, pos.y, 0 }, { 0,0,0 }, { w, h, 1 }),
			UIImageComponent("UI_FONT", 1.0f, true, { 1,1,1,1 })
		);
		// UV設定
		int idx = digits[i];
		int r = (idx <= 9) ? idx / 5 : 2;
		int c = (idx <= 9) ? idx % 5 : (idx - 10);
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(d);
		ui.uvPos = { c * 0.2f, r * 0.333f };
		ui.uvScale = { 0.2f, 0.333f };
	}
}

// ヘルパー: ボタン作成
void ResultScene::CreateButtons()
{
	float y = SCREEN_HEIGHT * 0.90f;
	float w = 200, h = 100;
	float gap = 200;

	// Retry
	m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.75f - gap, y, 0 }, { 0,0,0 }, { w, h, 1 }),
		UIImageComponent("BTN_RETRY", 2.0f),
		UIButtonComponent(ButtonState::Normal, true, []() {
			GameScene::SetStageNo(ResultScene::s_resultData.stageID);
			SceneManager::ChangeScene<GameScene>();
			})
	);
	// Stage Select
	m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.75f, y, 0 }, { 0,0,0 }, { w, h, 1 }),
		UIImageComponent("BTN_BACK_STAGE_SELECT", 2.0f),
		UIButtonComponent(ButtonState::Normal, true, []() {
			SceneManager::ChangeScene<StageSelectScene>();
			})
	);
	// Title
	m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.75f + gap, y, 0 }, { 0,0,0 }, { w, h, 1 }),
		UIImageComponent("BTN_BACK_TITLE", 2.0f),
		UIButtonComponent(ButtonState::Normal, true, []() {
			SceneManager::ChangeScene<TitleScene>();
			})
	);
}