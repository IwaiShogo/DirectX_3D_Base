/*****************************************************************//**
 * @file	StageSelectScene.cpp
 * @brief
 * * @details
 * * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * * @date	2025/11/13
 * * @update	2025/xx/xx
 *********************************************************************/

 // ===== インクルード =====
#include "Scene/SceneManager.h"
#include "Scene/ResultScene.h"
#include "Systems/Input.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
using namespace DirectX;
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

	// ==========================================
	// 1. 一覧画面 (List) UI構築
	// ==========================================
	std::vector<std::string> stageIDs = { "ST_001", "ST_002", "ST_003", "ST_004", "ST_005", "ST_006" };
	float startX = SCREEN_WIDTH * 0.2f;
	float startY = SCREEN_HEIGHT * 0.3f;
	float gapX = 350.0f;
	float gapY = 250.0f;

	// 背景（一覧）
	ECS::EntityID listBg = m_coordinator->CreateEntity(
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
	m_listUIEntities.push_back(listBg);

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

	// 背景　白
	ECS::EntityID infoBg1 = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position */ XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation */ XMFLOAT3(0, 0, 0),
			/* Scale    */ XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
		),
		UIImageComponent(
			/* AssetID	*/	"BG_INFO1",
			/* Depth	*/	0.0f
		)
	);
	m_detailUIEntities.push_back(infoBg1);

	// 背景　城前
	ECS::EntityID infoBg2 = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position */ XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation */ XMFLOAT3(0, 0, 0),
			/* Scale    */ XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
		),
		UIImageComponent(
			/* AssetID	*/	"BG_INFO2",
			/* Depth	*/	0.0f
		)
	);
	m_detailUIEntities.push_back(infoBg2);

	// --- A. マップ画像 (左側) ---
	ECS::EntityID mapImg = m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(500, 480, 1)),
		UIImageComponent("UI_STAGE_MAP", 1.0f)
	);
	m_detailUIEntities.push_back(mapImg);

	// --- B. 情報パネル (右側) ---
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.83f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(500, 160, 1)),
		UIImageComponent("UI_FRAME", 1.0f)
	));

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.16f, SCREEN_HEIGHT * 0.83f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
		UIImageComponent("UI_BEST_TIME", 2.0f)
	));

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.2f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(550, 220, 1)),
		UIImageComponent("UI_TRESURE", 1.0f)
	));

	// 右ページ：★3つ（※セレクト画面ではアニメしない）
	{
		const ResultData& rd = ResultScene::GetResultData();

		// 「クリア済み」かつ「今見ているステージの結果」のときだけ星を出す
		const bool hasValidResult = (rd.isCleared && (rd.stageID == m_selectedStageID));

		bool stars[3] = {
			hasValidResult && !rd.wasSpotted,
			hasValidResult && rd.collectedAllOrdered,
			hasValidResult && rd.clearedInTime
		};

		const char* conditionTex[3] = {
			"STAR_TEXT1",
			"STAR_TEXT2",
			"STAR_TEXT3"
		};

		float baseY = SCREEN_HEIGHT * 0.50f;
		float gapY = 55.0f;
		float starX = SCREEN_WIDTH * 0.6f;
		float captionX = SCREEN_WIDTH * 0.75f;

		const float STAR_OFF_SIZE_TOP = 50.0f;
		const float STAR_OFF_SIZE_LOW = 34.0f;
		const float STAR_ON_SIZE_TOP = 50.0f;
		const float STAR_ON_SIZE_LOW = 34.0f;

		for (int i = 0; i < 3; ++i)
		{
			float y = baseY + i * gapY;

			// 条件テキスト（タグ無し＝アニメしない）
			ECS::EntityID condEnt = m_coordinator->CreateEntity(
				TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 320.0f, 60.0f, 1.0f }),
				UIImageComponent(conditionTex[i], 1.0f, true, { 1,1,1,1 })
			);
			m_detailUIEntities.push_back(condEnt);

			const float offSize = (i == 0) ? STAR_OFF_SIZE_TOP : STAR_OFF_SIZE_LOW;
			const float onSize = (i == 0) ? STAR_ON_SIZE_TOP : STAR_ON_SIZE_LOW;

			// ★ Off
			ECS::EntityID offEnt = m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { offSize, offSize, 1.0f }),
				UIImageComponent("ICO_STAR_OFF", 1.0f, true, { 1,1,1,1 })
			);
			m_detailUIEntities.push_back(offEnt);

			// ★ On（達成時のみ / タグ無し＝アニメしない）
			if (stars[i])
			{
				ECS::EntityID onEnt = m_coordinator->CreateEntity(
					TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
					UIImageComponent("ICO_STAR_ON", 2.0f, true, { 1,1,1,1 })
				);
				m_detailUIEntities.push_back(onEnt);
			}
		}

		// --- C. ボタン類 ---

		m_detailUIEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
			UIImageComponent("UI_START_NORMAL", 1.0f)
		));

		ECS::EntityID startBtn = m_coordinator->CreateEntity(
			TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
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

		m_detailUIEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(200, 100, 1)),
			UIImageComponent("UI_FINISH_NORMAL", 1.0f)
		));

		ECS::EntityID backBtn = m_coordinator->CreateEntity(
			TransformComponent(XMFLOAT3(SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(160, 80, 1)),
			UIImageComponent("BTN_REBERSE", 2.0f),
			UIButtonComponent(
				ButtonState::Normal,
				false, // Detailモード用
				[this]() {
					this->SwitchState(false);
				}
			)
		);
		m_detailUIEntities.push_back(backBtn);

		// カーソル作成
		{
			m_coordinator->CreateEntity(
				TransformComponent(
					XMFLOAT3(0.0f, 0.0f, 0.0f),
					XMFLOAT3(0.0f, 0.0f, 0.0f),
					XMFLOAT3(64.0f, 64.0f, 1.0f)
				),
				UIImageComponent(
					"ICO_CURSOR",
					10.0f
				),
				UICursorComponent()
			);
		}

		SwitchState(false);
		std::cout << "StageSelectScene::Init() - ECS Initialized." << std::endl;
	}
}

void StageSelectScene::Uninit()
{
	auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>();
	if (effectSystem)
	{
		effectSystem->Uninit();
	}

	ECSInitializer::UninitECS();
	m_coordinator.reset();
	s_coordinator = nullptr;
}

void StageSelectScene::Update(float deltaTime)
{
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

	if (auto system = ECS::ECSInitializer::GetSystem<EffectSystem>())
	{
		system->Render();
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
			d.imageID = val.value("image", "default");
			d.timeLimitStar = val.value("timeLimitStar", 180.0f);

			if (val.contains("items") && val["items"].is_array()) {
				for (const auto& item : val["items"]) d.items.push_back(item.get<std::string>());
			}

			if (val.contains("gimmicks") && val["gimmicks"].is_array()) {
				for (const auto& gim : val["gimmicks"]) {
					d.gimmicks.push_back({ gim.value("type", "Unknown"), gim.value("count", 0) });
				}
			}
			else {
				int guards = val.value("guardCount", 0);
				if (guards > 0) d.gimmicks.push_back({ "Guard", guards });
			}

			m_stageDataMap[el.key()] = d;
		}
	}
}

void StageSelectScene::SwitchState(bool toDetail)
{
	for (auto id : m_listUIEntities)
	{
		if (m_coordinator->HasComponent<UIImageComponent>(id)) {
			m_coordinator->GetComponent<UIImageComponent>(id).isVisible = !toDetail;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(id)) {
			m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = !toDetail;
		}
	}

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
