/*****************************************************************//**
 * @file	StageSelectScene.cpp
 * @brief
 * * @details
 * * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * * @date	2025/11/13	初回作成日
 * 作業内容：	- 追加：
 * * @update	2025/xx/xx	最終更新日
 * 作業内容：	- XX：
 * * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "Scene/SceneManager.h"
#include "Systems/Input.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
using namespace DirectX;
// これを追加しておくと、ECS:: を省略できます
using namespace ECS;

// ===== 静的メンバー変数の定義 =====
ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

std::string GetStageItemIconPath(const std::string& itemID)
{
	if (itemID == "Takara_Daiya")   return "ICO_TREASURE1";
	if (itemID == "Takara_Crystal") return "ICO_TREASURE2";
	if (itemID == "Takara_Yubiwa")  return "ICO_TREASURE3";
	if (itemID == "Takara_Kaiga1")  return "ICO_TREASURE4";
	if (itemID == "Takara_Kaiga2")  return "ICO_TREASURE5";
	if (itemID == "Takara_Kaiga3")  return "ICO_TREASURE6";

	// デフォルト
	return "ICO_TREASURE";
}

void StageSelectScene::Init()
{
	// 1. 初期化
	m_coordinator = std::make_unique<Coordinator>();
	s_coordinator = m_coordinator.get();
	ECSInitializer::InitECS(m_coordinator);

	LoadStageData();

	// 固定カメラ
	ECS::EntityID cam = ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0, 0, 0 });

	// 背景
	m_coordinator->CreateEntity(
		TransformComponent(
			/* Position */ XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation */ XMFLOAT3(0, 0, 0),
			/* Scale    */ XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
		),
		UIImageComponent(
			/* AssetID	*/	"BG_STAGE_SELECT",
			/* Depth	*/	0.0f
		)
	);

	// ==========================================
	// 1. 一覧画面 (List) UI構築
	// ==========================================
	std::vector<std::string> stageIDs = { "ST_001", "ST_002", "ST_003", "ST_004", "ST_005", "ST_006" };
	float startX = SCREEN_WIDTH * 0.2f;
	float startY = SCREEN_HEIGHT * 0.3f;
	float gapX = 350.0f;
	float gapY = 250.0f;

	for (int i = 0; i < 6; ++i)
	{
		std::string id = (i < stageIDs.size()) ? stageIDs[i] : "ST_001";
		float x = startX + (i % 3) * gapX;
		float y = startY + (i / 3) * gapY;

		// ステージ選択ボタン
		ECS::EntityID btn = m_coordinator->CreateEntity(
			TransformComponent(XMFLOAT3(x, y, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(250, 150, 1)),
			UIImageComponent("BTN_STAGE_SELECT", 1.0f),
			UIButtonComponent(
				ButtonState::Normal,
				true, // Listモードなので最初は表示
				[this, id]() {
					// 詳細モードへ遷移
					this->m_selectedStageID = id;
					this->SwitchState(true);
				}
			)
		);
		m_listUIEntities.push_back(btn);
	}

	// ==========================================
	// 2. 詳細画面 (Detail) UI構築
	// ==========================================

	// --- A. マップ画像 (左側) ---
	// 位置: 画面左寄り, サイズ: 大きめ
	ECS::EntityID mapImg = m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(400, 400, 1)),
		UIImageComponent("UI_STAGE_MAP", 1.0f)
	);
	m_detailUIEntities.push_back(mapImg);

	// --- B. 情報パネル (右側) ---
	// ベストタイムフレーム
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.8f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(550, 160, 1)),
		UIImageComponent("UI_FRAME", 1.0f)
	));

	// ベストタイム（アイコン）
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.15f, SCREEN_HEIGHT * 0.8f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
		UIImageComponent("UI_BEST_TIME", 2.0f)
	));

	// お宝情報 (アイコン)
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.75f, SCREEN_HEIGHT * 0.25f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(500, 200, 1)),
		UIImageComponent("UI_TRESURE", 1.0f)
	));
	// お宝数 (本来は数字画像を表示するが、ここではログ出力で代用)

	// 警備情報 (アイコン)
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.75f, SCREEN_HEIGHT * 0.6f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(500, 260, 1)),
		UIImageComponent("UI_STAGE_ENEMY", 1.0f)
	));


	// --- C. ボタン類 ---

	// [決定] ボタン (右下)
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.65f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
		UIImageComponent("UI_START_NORMAL", 1.0f)
	));

	ECS::EntityID startBtn = m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.65f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 80, 1)),
		UIImageComponent("BTN_DECISION", 2.0f),
		UIButtonComponent(
			ButtonState::Normal,
			false, // Detailモード用なので最初は非表示
			[this]() {
				std::cout << "Game Start: " << m_selectedStageID << std::endl;
				GameScene::SetStageNo(m_selectedStageID);
				SceneManager::ChangeScene<GameScene>();
			}
		)
	);
	m_detailUIEntities.push_back(startBtn);

	// [戻る] ボタン (決定ボタンの左隣、あるいは左下)
	// 「戻る」ボタン
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.85f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
		UIImageComponent("UI_FINISH_NORMAL", 1.0f)
	));

	ECS::EntityID backBtn = m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.85f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(160, 80, 1)),
		UIImageComponent("BTN_REBERSE", 2.0f),
		UIButtonComponent(
			ButtonState::Normal,
			false, // Detailモード用
			[this]() {
				// 一覧へ戻る
				this->SwitchState(false);
			}
		)
	);
	m_detailUIEntities.push_back(backBtn);

	// 5. カーソル作成
	{
		m_coordinator->CreateEntity(
			TransformComponent(
				/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(64.0f, 64.0f, 1.0f)
			),
			UIImageComponent(
				/* AssetID	*/	"ICO_CURSOR",
				/* Depth	*/	10.0f
			),
			UICursorComponent()
		);
	}
	SwitchState(false);

	std::cout << "StageSelectScene::Init() - ECS Initialized." << std::endl;
}

void StageSelectScene::Uninit()
{
	ECSInitializer::UninitECS();
	m_coordinator.reset();
	s_coordinator = nullptr;
}

void StageSelectScene::Update(float deltaTime)
{
	// 1. システム更新
	m_coordinator->UpdateSystems(deltaTime);
}

void StageSelectScene::Draw()
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

void StageSelectScene::LoadStageData()
{
	std::ifstream i("Assets/Config/map_config.json");
	if (i.is_open())
	{
		json j;
		i >> j;
		for (auto& el : j.items())
		{
			auto& val = el.value();
			StageData d;
			d.name = val.value("name", "Unknown Stage");
			d.imageID = val.value("image", "default"); // デフォルト画像
			d.timeLimitStar = val.value("timeLimitStar", 180.0f);

			// アイテムリスト
			if (val.contains("items") && val["items"].is_array()) {
				for (const auto& item : val["items"]) d.items.push_back(item.get<std::string>());
			}

			// ギミック情報
			if (val.contains("gimmicks") && val["gimmicks"].is_array()) {
				for (const auto& gim : val["gimmicks"]) {
					d.gimmicks.push_back({ gim.value("type", "Unknown"), gim.value("count", 0) });
				}
			}
			else {
				// 互換性: guardCountがあればギミックとして追加
				int guards = val.value("guardCount", 0);
				if (guards > 0) d.gimmicks.push_back({ "Guard", guards });
			}

			m_stageDataMap[el.key()] = d;
		}
	}
}

void StageSelectScene::SwitchState(bool toDetail)
{
	// 一覧画面UIの制御
	for (auto id : m_listUIEntities)
	{
		// 画像の表示切替
		if (m_coordinator->HasComponent<UIImageComponent>(id)) {
			m_coordinator->GetComponent<UIImageComponent>(id).isVisible = !toDetail;
		}
		// ボタンの有効化切替
		if (m_coordinator->HasComponent<UIButtonComponent>(id)) {
			m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = !toDetail;
		}
	}

	// 詳細画面UIの制御
	for (auto id : m_detailUIEntities)
	{
		if (m_coordinator->HasComponent<UIImageComponent>(id)) {
			m_coordinator->GetComponent<UIImageComponent>(id).isVisible = toDetail;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(id)) {
			m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = toDetail;
		}
	}
}

//void StageSelectScene::CreateDetailUI()
//{
//	// 既存のUIを削除
//	for (auto id : m_detailUIEntities) m_coordinator->DestroyEntity(id);
//	m_detailUIEntities.clear();
//
//	if (m_stageDataMap.find(m_selectedStageID) == m_stageDataMap.end()) return;
//	const auto& data = m_stageDataMap[m_selectedStageID];
//
//	// A. ステージイメージ (左側)
//	ECS::EntityID mapImg = m_coordinator->CreateEntity(
//		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.3f, SCREEN_HEIGHT * 0.45f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(500, 350, 1)),
//		UIImageComponent(data.imageID) // JSONのimageIDを使用
//	);
//	m_detailUIEntities.push_back(mapImg);
//
//	// B. 情報パネル (右側)
//	float infoX = SCREEN_WIDTH * 0.7f;
//	float startY = SCREEN_HEIGHT * 0.3f;
//
//	// B-1. 目標タイム表示
//	// (本来はフォント描画ですが、ここでは概念的にアイコンと枠を表示)
//	// 「TARGET TIME: 03:00」のような表示が必要
//	// ... (フォント描画ロジックを入れるか、簡易アイコンのみ) ...
//
//	// B-2. 出現アイテム一覧
//	float itemSize = 50.0f;
//	for (size_t i = 0; i < data.items.size(); ++i) {
//		std::string path = GetStageItemIconPath(data.items[i]);
//		ECS::EntityID itemIcon = m_coordinator->CreateEntity(
//			TransformComponent(
//				XMFLOAT3(infoX + (i % 3) * (itemSize + 10), startY + (i / 3) * (itemSize + 10), 0),
//				XMFLOAT3(0, 0, 0), XMFLOAT3(itemSize, itemSize, 1)
//			),
//			UIImageComponent(path)
//		);
//		m_detailUIEntities.push_back(itemIcon);
//	}
//
//	// B-3. ギミック情報 (Guardアイコン x 個数 など)
//	float gimY = startY + 150.0f;
//	for (size_t i = 0; i < data.gimmicks.size(); ++i) {
//		auto& gim = data.gimmicks[i];
//		std::string path = "ICO_TASER"; // 仮
//		if (gim.type == "Guard") path = "ICO_TASER";
//
//		// アイコン
//		ECS::EntityID gimIcon = m_coordinator->CreateEntity(
//			TransformComponent(XMFLOAT3(infoX, gimY + i * 60, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(50, 50, 1)),
//			UIImageComponent(path)
//		);
//		m_detailUIEntities.push_back(gimIcon);
//
//		// 個数表示 (簡易的にアイコンを並べるか、本来は数字フォント)
//		// ここではアイコンを個数分並べる簡易実装
//		for (int k = 1; k < gim.count && k < 5; ++k) {
//			ECS::EntityID subIcon = m_coordinator->CreateEntity(
//				TransformComponent(XMFLOAT3(infoX + k * 40, gimY + i * 60, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(40, 40, 1)),
//				UIImageComponent(path)
//			);
//			m_detailUIEntities.push_back(subIcon);
//		}
//	}
//
//	// C. ボタン類
//	// [Start]
//	ECS::EntityID startBtn = m_coordinator->CreateEntity(
//		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.85f, SCREEN_HEIGHT * 0.85f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 80, 1)),
//		UIImageComponent("BTN_DECISION"),
//		UIButtonComponent(ButtonState::Normal, true, [this]() {
//			GameScene::SetStageNo(m_selectedStageID);
//			SceneManager::ChangeScene<GameScene>();
//			})
//	);
//	m_detailUIEntities.push_back(startBtn);
//
//	// [Back]
//	ECS::EntityID backBtn = m_coordinator->CreateEntity(
//		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.15f, SCREEN_HEIGHT * 0.85f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 80, 1)),
//		UIImageComponent("BTN_REBERSE"),
//		UIButtonComponent(ButtonState::Normal, true, [this]() {
//			this->SwitchState(false);
//			})
//	);
//	m_detailUIEntities.push_back(backBtn);
//}
