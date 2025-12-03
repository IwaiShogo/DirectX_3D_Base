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
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.3f, SCREEN_HEIGHT * 0.45f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(500, 350, 1)),
		UIImageComponent("UI_STAGE_MAP", 1.0f)
	);
	m_detailUIEntities.push_back(mapImg);

	// --- B. 情報パネル (右側) ---
	float infoX = SCREEN_WIDTH * 0.7f;
	float infoY_Base = SCREEN_HEIGHT * 0.3f;
	float infoGap = 80.0f;

	// お宝情報 (アイコン)
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(infoX, infoY_Base, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(64, 64, 1)),
		UIImageComponent("UI_TRESURE", 1.0f)
	));
	// お宝数 (本来は数字画像を表示するが、ここではログ出力で代用)

	// 警備情報 (アイコン)
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(infoX, infoY_Base + infoGap, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(64, 64, 1)),
		UIImageComponent("UI_STAGE_ENEMY", 1.0f)
	));

	// ベストタイム (アイコン)
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(infoX, infoY_Base + infoGap * 2, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(64, 64, 1)),
		UIImageComponent("UI_BEST_TIME", 1.0f)
	));


	// --- C. ボタン類 ---

	// [決定] ボタン (右下)
	ECS::EntityID startBtn = m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.85f, SCREEN_HEIGHT * 0.85f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 80, 1)),
		UIImageComponent("BTN_DECISION", 1.0f),
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
	ECS::EntityID backBtn = m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.15f, SCREEN_HEIGHT * 0.85f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 80, 1)),
		UIImageComponent("BTN_REBERSE", 1.0f),
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
				/* Depth	*/	1.0f
			),
			UICursorComponent()
		);
	}

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
	if (auto system = ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}
	if (auto system = ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
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
			StageData d;
			d.name = el.value().value("name", "Unknown");
			d.itemCount = el.value().value("itemCount", 0);
			d.guardCount = el.value().value("guardCount", 0);
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

	// 詳細表示時、コンソールに情報を出力（テキスト表示の代わり）
	if (toDetail && m_stageDataMap.count(m_selectedStageID))
	{
		const auto& data = m_stageDataMap[m_selectedStageID];
		std::cout << "=== Detail View ===" << std::endl;
		std::cout << "Stage: " << data.name << std::endl;
		std::cout << "Items: " << data.itemCount << std::endl;
		std::cout << "Guards: " << data.guardCount << std::endl;
		// ここで本来は、数字画像のEntityのテクスチャを差し替える等の処理を行う
	}
}