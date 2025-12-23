/*****************************************************************//**
 * @file	StageSelectScene.cpp
 *********************************************************************/

#include "Scene/SceneManager.h"
#include "Scene/ResultScene.h"
#include "Scene/GameScene.h"

#include "Systems/Input.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Rendering/EffectSystem.h"

#include <DirectXMath.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <cmath>



using namespace DirectX;
using namespace ECS;

ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

extern std::string GetStageItemIconPath(const std::string& itemID);

static float Clamp01(float x)
{
	if (x < 0.0f) return 0.0f;
	if (x > 1.0f) return 1.0f;
	return x;
}

void StageSelectScene::Init()
{
	m_coordinator = std::make_shared<Coordinator>();
	s_coordinator = m_coordinator.get();
	ECSInitializer::InitECS(m_coordinator);

	LoadStageData();

	// カメラ（EffectSystemが探すので作っておく）
	EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0,0,0 });

	// ★重要：StageSelectはUI座標でエフェクトを出したいのでスクリーン座標カメラを上書き
	if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
	{
		effectSystem->SetScreenSpaceCamera((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	}

	// 流れ星用乱数
	{
		std::random_device rd;
		m_rng = std::mt19937(rd());
		std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
		m_nextShootingStarWait = dist(m_rng);
		m_shootingStarTimer = 0.0f;
	}

	// =====================
	// List UI（一覧）
	// =====================
	std::vector<std::string> stageIDs = { "ST_001", "ST_002", "ST_003", "ST_004", "ST_005", "ST_006" };

	float startX = SCREEN_WIDTH * 0.2f;
	float startY = SCREEN_HEIGHT * 0.3f;
	float gapX = 350.0f;
	float gapY = 250.0f;

	EntityID listBg = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_STAGE_SELECT", 0.0f)
	);
	m_listUIEntities.push_back(listBg);

	for (int i = 0; i < 6; ++i)
	{
		std::string id = (i < (int)stageIDs.size()) ? stageIDs[i] : "ST_001";
		float x = startX + (i % 3) * gapX;
		float y = startY + (i / 3) * gapY;

		EntityID btn = m_coordinator->CreateEntity(
			TransformComponent({ x, y, 0.0f }, { 0,0,0 }, { 250, 150, 1 }),
			UIImageComponent("BTN_STAGE_SELECT", 1.0f),
			UIButtonComponent(
				ButtonState::Normal,
				true,
				[this, id]() {
					if (m_inputLocked) return;

					m_selectedStageID = id;

					// 一覧→詳細：フェードアウト→切替→フェードイン
					StartFadeOut(1.0f, [this]() { SwitchState(true); }, true);
				}
			)
		);

		m_listUIEntities.push_back(btn);
	}

	// =====================
	// Detail UI（情報/詳細）
	// =====================
	EntityID infoBg1 = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_INFO1", 0.0f)
	);
	m_detailUIEntities.push_back(infoBg1);

	EntityID infoBg2 = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_INFO2", 0.0f)
	);
	m_detailUIEntities.push_back(infoBg2);

	// UI_STAGE_MAP
	EntityID mapImg = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 0.0f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE_MAP", 0.9f) // ★0.9f: EffectSystem描画(流れ星)を「マップの上」に見せるため、UI前半側へ
	);
	m_detailUIEntities.push_back(mapImg);
	m_stageMapEntity = mapImg;

	// 下の情報パネル
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.83f, 0.0f }, { 0,0,0 }, { 500, 160, 1 }),
		UIImageComponent("UI_FRAME", 1.0f)
	));

	// BEST TIME
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.16f, SCREEN_HEIGHT * 0.83f, 0.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_BEST_TIME", 2.0f)
	));

	// 右上トレジャー枠
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.2f, 0.0f }, { 0,0,0 }, { 550, 220, 1 }),
		UIImageComponent("UI_TRESURE", 1.0f)
	));

	// ★スター（ResultData）
	{
		const ResultData& rd = ResultScene::GetResultData();
		const bool hasValidResult = (rd.isCleared && (rd.stageID == m_selectedStageID));

		bool stars[3] = {
			hasValidResult && !rd.wasSpotted,
			hasValidResult && rd.collectedAllOrdered,
			hasValidResult && rd.clearedInTime
		};

		const char* conditionTex[3] = { "STAR_TEXT1","STAR_TEXT2","STAR_TEXT3" };

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

			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 320.0f, 60.0f, 1.0f }),
				UIImageComponent(conditionTex[i], 1.0f, true, { 1,1,1,1 })
			));

			const float offSize = (i == 0) ? STAR_OFF_SIZE_TOP : STAR_OFF_SIZE_LOW;
			const float onSize = (i == 0) ? STAR_ON_SIZE_TOP : STAR_ON_SIZE_LOW;

			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { offSize, offSize, 1.0f }),
				UIImageComponent("ICO_STAR_OFF", 1.0f, true, { 1,1,1,1 })
			));

			if (stars[i])
			{
				m_detailUIEntities.push_back(m_coordinator->CreateEntity(
					TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
					UIImageComponent("ICO_STAR_ON", 2.0f, true, { 1,1,1,1 })
				));
			}
		}

		// START UI + ボタン
		m_detailUIEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 0.0f }, { 0,0,0 }, { 200, 100, 1 }),
			UIImageComponent("UI_START_NORMAL", 1.0f)
		));

		EntityID startBtn = m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 0.0f }, { 0,0,0 }, { 200, 100, 1 }),
			UIImageComponent("BTN_DECISION", 2.0f),
			UIButtonComponent(
				ButtonState::Normal,
				true,
				[this]() {
					if (m_inputLocked) return;

					StartFadeOut(
						1.0f,
						[this]() {
							GameScene::SetStageNo(m_selectedStageID);
							SceneManager::ChangeScene<GameScene>();
						},
						false
					);
				}
			)
		);
		m_detailUIEntities.push_back(startBtn);

		// BACK UI + ボタン
		m_detailUIEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 0.0f }, { 0,0,0 }, { 200, 100, 1 }),
			UIImageComponent("UI_FINISH_NORMAL", 1.0f)
		));

		EntityID backBtn = m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 0.0f }, { 0,0,0 }, { 160, 80, 1 }),
			UIImageComponent("BTN_REBERSE", 2.0f),
			UIButtonComponent(
				ButtonState::Normal,
				true,
				[this]() {
					if (m_inputLocked) return;
					StartFadeOut(1.0f, [this]() { SwitchState(false); }, true);
				}
			)
		);
		m_detailUIEntities.push_back(backBtn);
	}
	// =====================
	// カーソル（常駐）
	// =====================
	m_cursorEntity = m_coordinator->CreateEntity(
		TransformComponent({ 0.0f, 0.0f, 0.0f }, { 0,0,0 }, { 64.0f, 64.0f, 1.0f }),
		UIImageComponent("ICO_CURSOR", 99999.0f),
		UICursorComponent()
	);

	// 初期：一覧
	SwitchState(false);

	// フェード
	CreateFadeOverlay();
	ApplyFadeAlpha(1.0f);
	StartFadeIn(1.0f);
}

void StageSelectScene::Uninit()
{
	// 上書き解除（安全）
	if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
	{
		effectSystem->ClearOverrideCamera();
	}

	auto effectSystem = ECSInitializer::GetSystem<EffectSystem>();
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

	UpdateFade(deltaTime);
	UpdateShootingStar(deltaTime);
	// ★追加：生成済みの流れ星（★+軌跡）を移動・寿命管理
	UpdateActiveShootingStars(deltaTime);
}

void StageSelectScene::Draw()
{
	if (auto system = ECSInitializer::GetSystem<UIRenderSystem>())
		system->Render(true);

	if (auto system = ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

#ifdef _DEBUG
	// ★ここに DebugDrawSystem の描画を入れる
	if (auto dbg = ECSInitializer::GetSystem<DebugDrawSystem>())
	{
		
	}
#endif

	if (auto system = ECSInitializer::GetSystem<UIRenderSystem>())
		system->Render(false);

	// ★UIの上にエフェクトを重ねたいのでUI描画の後で描く
	if (auto system = ECSInitializer::GetSystem<EffectSystem>())
		system->Render();
}

void StageSelectScene::UpdateShootingStar(float dt)
{
	if (!m_enableShootingStar) return;
	if (!m_isDetailMode) return;

	// フェード中は出さない（黒に隠れる）
	if (m_fadeState != FadeState::None) return;

	// デバッグ切り分け（任意）
	EnsureDebugEffectOnMap();

	if (m_stageMapEntity == (ECS::EntityID)-1) return;
	if (!m_coordinator->HasComponent<UIImageComponent>(m_stageMapEntity)) return;

	// 詳細に入った直後：1回だけ確実に出す
	if (m_spawnStarOnEnterDetail)
	{
		m_spawnStarOnEnterDetail = false;
		SpawnShootingStar();

		m_shootingStarTimer = 0.0f;
		std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
		m_nextShootingStarWait = dist(m_rng);
		return;
	}

	// 通常のランダム発生
	m_shootingStarTimer += dt;
	if (m_shootingStarTimer < m_nextShootingStarWait) return;

	m_shootingStarTimer = 0.0f;
	std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
	m_nextShootingStarWait = dist(m_rng);

	SpawnShootingStar();
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

			if (val.contains("items") && val["items"].is_array())
			{
				for (const auto& item : val["items"]) d.items.push_back(item.get<std::string>());
			}

			if (val.contains("gimmicks") && val["gimmicks"].is_array())
			{
				for (const auto& gim : val["gimmicks"])
				{
					d.gimmicks.push_back({ gim.value("type", "Unknown"), gim.value("count", 0) });
				}
			}
			else
			{
				int guards = val.value("guardCount", 0);
				if (guards > 0) d.gimmicks.push_back({ "Guard", guards });
			}

			m_stageDataMap[el.key()] = d;
		}
	}
}

void StageSelectScene::SwitchState(bool toDetail)
{
	// ★遷移判定用：切り替え前の状態を保持
	const bool wasDetail = m_isDetailMode;

	// 状態更新
	m_isDetailMode = toDetail;

	// 一覧
	for (auto id : m_listUIEntities)
	{
		SetUIVisible(id, !toDetail);
	}

	// 詳細
	for (auto id : m_detailUIEntities)
	{
		SetUIVisible(id, toDetail);
	}

	// カーソルは常に表示（消さない）
	if (m_cursorEntity != (ECS::EntityID)-1)
	{
		if (m_coordinator->HasComponent<UIImageComponent>(m_cursorEntity))
		{
			m_coordinator->GetComponent<UIImageComponent>(m_cursorEntity).isVisible = true;
		}
	}

	// ===== ここが本命：遷移したときだけ処理する =====
	if (toDetail)
	{
		// ★「一覧→詳細」に入った瞬間だけ
		if (!wasDetail)
		{
			m_shootingStarTimer = 0.0f;
			std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
			m_nextShootingStarWait = dist(m_rng);

			// ★フェード明け1フレーム目で必ず1回出す
			m_spawnStarOnEnterDetail = true;
		}
	}
	else
	{
		// ★「詳細→一覧」に戻った瞬間だけ
		if (wasDetail)
		{
			m_spawnStarOnEnterDetail = false;

			// ★残っている流れ星（本体＋軌跡）を全消し（一覧に持ち越さない）
			for (auto& s : m_activeShootingStars)
			{
				if (s.star != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.star);
				for (int k = 0; k < 3; ++k)
				{
					if (s.trails[k] != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.trails[k]);
				}
			}
			m_activeShootingStars.clear();

			// ★デバッグ常駐エフェクトを破棄（詳細画面でのみ表示）
			if (m_debugStarEntity != (ECS::EntityID)-1)
			{
				m_coordinator->DestroyEntity(m_debugStarEntity);
				m_debugStarEntity = (ECS::EntityID)-1;
			}
		}
	}
}

void StageSelectScene::SetUIVisible(ECS::EntityID id, bool visible)
{
	if (!m_coordinator) return;

	if (m_coordinator->HasComponent<UIImageComponent>(id))
	{
		m_coordinator->GetComponent<UIImageComponent>(id).isVisible = visible;
	}

	if (m_coordinator->HasComponent<UIButtonComponent>(id))
	{
		m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = visible;
	}
}

// ===== Fade =====

void StageSelectScene::CreateFadeOverlay()
{
	m_fadeEntity = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_INFO1", 10000.0f, true, { 0,0,0,0 })
	);

	ApplyFadeAlpha(0.0f);
}

void StageSelectScene::StartFadeIn(float durationSec)
{
	m_fadeDuration = durationSec;
	m_fadeTimer = 0.0f;
	m_fadeState = FadeState::FadingIn;
	m_inputLocked = true;
}

void StageSelectScene::StartFadeOut(float durationSec, std::function<void()> onBlack, bool autoFadeIn)
{
	if (m_fadeState != FadeState::None) return;

	m_fadeDuration = durationSec;
	m_fadeTimer = 0.0f;
	m_fadeState = FadeState::FadingOut;
	m_inputLocked = true;

	m_onBlack = std::move(onBlack);
	m_autoFadeInAfterBlack = autoFadeIn;
}

void StageSelectScene::UpdateFade(float dt)
{
	if (m_fadeState == FadeState::None) return;

	m_fadeTimer += dt;
	float t = Clamp01(m_fadeTimer / m_fadeDuration);

	if (m_fadeState == FadeState::FadingOut)
	{
		ApplyFadeAlpha(t);

		if (t >= 1.0f)
		{
			if (m_onBlack) m_onBlack();
			m_onBlack = nullptr;

			if (m_autoFadeInAfterBlack)
			{
				m_fadeTimer = 0.0f;
				m_fadeState = FadeState::FadingIn;
			}
			else
			{
				m_fadeState = FadeState::None;
				m_inputLocked = false;
			}
		}
	}
	else if (m_fadeState == FadeState::FadingIn)
	{
		ApplyFadeAlpha(1.0f - t);

		if (t >= 1.0f)
		{
			ApplyFadeAlpha(0.0f);
			m_fadeState = FadeState::None;
			m_inputLocked = false;
		}
	}
}

void StageSelectScene::ApplyFadeAlpha(float a)
{
	m_fadeAlpha = Clamp01(a);

	if (!m_coordinator) return;
	if (!m_coordinator->HasComponent<UIImageComponent>(m_fadeEntity)) return;

	auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_fadeEntity);
	ui.isVisible = true;
	ui.color = { 0.0f, 0.0f, 0.0f, m_fadeAlpha };
}

// ===== Shooting Star =====

bool StageSelectScene::GetUIRect(ECS::EntityID id, float& left, float& top, float& right, float& bottom) const
{
	if (!m_coordinator) return false;
	if (id == (ECS::EntityID)-1) return false;
	if (!m_coordinator->HasComponent<TransformComponent>(id)) return false;

	const auto& tr = m_coordinator->GetComponent<TransformComponent>(id);

	const float cx = tr.position.x;
	const float cy = tr.position.y;
	const float w = tr.scale.x;
	const float h = tr.scale.y;

	left = cx - w * 0.5f;
	right = cx + w * 0.5f;
	top = cy - h * 0.5f;
	bottom = cy + h * 0.5f;
	return true;
}

void StageSelectScene::EnsureDebugEffectOnMap()
{
	if (!m_debugShowGlowOnMap) return;
	if (!m_coordinator) return;
	if (!m_isDetailMode) return;
	if (m_fadeState != FadeState::None) return; // 黒フェード中は見えないので作らない
	if (m_stageMapEntity == (ECS::EntityID)-1) return;
	if (m_debugStarEntity != (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(m_stageMapEntity, l, t, r, b)) return;

	const float cx = (l + r) * 0.5f;
	const float cy = (t + b) * 0.5f;

	// ★まず「エフェクト自体が描けるか」を確実に確認するため、動作実績のある EFK_TREASURE_GLOW を常駐表示
	m_debugStarEntity = m_coordinator->CreateEntity(
		TagComponent("effect_debug"),
		TransformComponent({ cx, cy, 0.5f }, { 0,0,0 }, { 1,1,1 }),
		EffectComponent(
			"EFK_TREASURE_GLOW",
			true,   // loop
			true,   // play on create
			{ 0,0,0 },
			0.3f    // TREASUREと同じスケール
		)
	);

	std::cout << "[StageSelect] Debug glow created on map center\n";
}

static float EaseOutQuad(float t)
{
	t = Clamp01(t);
	return 1.0f - (1.0f - t) * (1.0f - t);
}

static float SmoothStep01(float t)
{
	t = Clamp01(t);
	return t * t * (3.0f - 2.0f * t);
}

static DirectX::XMFLOAT2 Bezier2(const DirectX::XMFLOAT2& p0, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, float t)
{
	const float u = 1.0f - t;
	return {
		u * u * p0.x + 2.0f * u * t * p1.x + t * t * p2.x,
		u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y
	};
}

static DirectX::XMFLOAT2 Bezier2Deriv(const DirectX::XMFLOAT2& p0, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, float t)
{
	// B'(t)=2(1-t)(p1-p0)+2t(p2-p1)
	const float u = 1.0f - t;
	return {
		2.0f * u * (p1.x - p0.x) + 2.0f * t * (p2.x - p1.x),
		2.0f * u * (p1.y - p0.y) + 2.0f * t * (p2.y - p1.y)
	};
}

static float Clamp01_Local(float v)
{
	if (v < 0.0f) return 0.0f;
	if (v > 1.0f) return 1.0f;
	return v;
}

static float Smooth01_Local(float t)
{
	t = Clamp01_Local(t);
	return t * t * (3.0f - 2.0f * t);
}

void StageSelectScene::UpdateActiveShootingStars(float dt)
{
	if (!m_coordinator) return;

	// ★UI_STAGE_MAP内だけ：詳細モードでなくなった/矩形が取れないなら全消し
	float l, t, r, b;
	if (!m_isDetailMode || !GetUIRect(m_stageMapEntity, l, t, r, b))
	{
		KillAllShootingStars();
		return;
	}

	// マップ内側の判定（Spawnと同じ基準）
	const float PAD = 24.0f;
	const float left = l + PAD;
	const float right = r - PAD;
	const float top = t + PAD;
	const float bottom = b - PAD;

	// ★ギリギリ調整（小さいほど端で消える）
	const float KILL_MARGIN = 2.0f;

	const float killLeft = left + KILL_MARGIN;   // ★左端で消す
	const float killRight = right - KILL_MARGIN;
	const float killTop = top + KILL_MARGIN;
	const float killBottom = bottom - KILL_MARGIN;

	// ★左端手前でフェードする帯（この幅で 1→0 に落とす）
	const float FADE_BAND = 90.0f;                // 60〜140で好み
	const float FADE_START_X = killLeft + FADE_BAND;

	for (int i = (int)m_activeShootingStars.size() - 1; i >= 0; --i)
	{
		auto& s = m_activeShootingStars[i];

		// 寿命更新
		s.remaining -= dt;

		// 本体が無いなら掃除
		if (s.star == (ECS::EntityID)-1 || !m_coordinator->HasComponent<TransformComponent>(s.star))
		{
			if (s.star != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.star);
			for (int k = 0; k < 3; ++k)
				if (s.trails[k] != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.trails[k]);

			m_activeShootingStars.erase(m_activeShootingStars.begin() + i);
			continue;
		}

		auto& starTr = m_coordinator->GetComponent<TransformComponent>(s.star);

		// 移動
		starTr.position.x += s.velocity.x * dt;
		starTr.position.y += s.velocity.y * dt;

		// ★UI_STAGE_MAPの外へ出たら即消す（左端ギリギリ含む）/ 寿命切れでも消す
		if (s.remaining <= 0.0f ||
			starTr.position.x <= killLeft ||
			starTr.position.x >= killRight ||
			starTr.position.y <= killTop ||
			starTr.position.y >= killBottom)
		{
			if (s.star != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.star);
			for (int k = 0; k < 3; ++k)
				if (s.trails[k] != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.trails[k]);

			m_activeShootingStars.erase(m_activeShootingStars.begin() + i);
			continue;
		}

		// ===== 左端手前のFADE_BANDで 1→0 =====
		float xFadeK = 1.0f;
		if (starTr.position.x <= FADE_START_X)
		{
			// u: 1(遠い) → 0(左端)
			const float u = (starTr.position.x - killLeft) / std::max(0.0001f, FADE_BAND);
			xFadeK = Smooth01_Local(Clamp01_Local(u));
		}

		// ===== 時間ベースの縮小（最終はxFadeで確実に0）=====
		const float life = (s.life > 0.0001f) ? s.life : 0.0001f;
		const float elapsed = (life - s.remaining);
		const float t01 = Clamp01_Local(elapsed / life);

		const float headK = (1.0f + (0.10f - 1.0f) * (t01 * t01)) * xFadeK;
		const float trailK = (1.0f + (0.25f - 1.0f) * (t01)) * xFadeK;

		starTr.scale = { headK, headK, headK };

		// 方向と尾向き
		const float vx = s.velocity.x;
		const float vy = s.velocity.y;
		const float len = std::sqrt(vx * vx + vy * vy);
		if (len <= 0.0001f) continue;

		const float nx = vx / len;
		const float ny = vy / len;

		const float dir = std::atan2(vy, vx);
		const float tail = dir + DirectX::XM_PI;
		starTr.rotation.z = tail;

		// ★軌跡：3本更新（出始めが早い/重なり）
		const float behindArr[3] = { 14.0f, 34.0f, 58.0f };

		for (int k = 0; k < 3; ++k)
		{
			ECS::EntityID tid = s.trails[k];
			if (tid == (ECS::EntityID)-1 || !m_coordinator->HasComponent<TransformComponent>(tid)) continue;

			auto& tr = m_coordinator->GetComponent<TransformComponent>(tid);

			const float behind = behindArr[k];
			tr.position.x = starTr.position.x - nx * behind;
			tr.position.y = starTr.position.y - ny * behind;
			tr.position.z = starTr.position.z;

			tr.rotation.z = tail;

			const float kMul = 1.0f - 0.15f * (float)k; // 0,1,2で少しずつ細く
			tr.scale = { trailK * kMul, trailK * kMul, trailK * kMul };
		}
	}
}


void StageSelectScene::SpawnShootingStar()
{
	if (!m_coordinator) return;
	if (!m_isDetailMode) return;
	if (m_stageMapEntity == (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(m_stageMapEntity, l, t, r, b)) return;

	// UI_STAGE_MAP 内側の“安全マージン”
	const float PAD = 24.0f;
	const float left = l + PAD;
	const float right = r - PAD;
	const float top = t + PAD;
	const float bottom = b - PAD;
	if (right <= left || bottom <= top) return;

	// ★左端ギリギリで消すためのライン（このXで消滅）
	const float KILL_LEFT = left + 2.0f;   // “ギリギリ”調整：0〜6あたりで好み

	// ★右端の“内側”から出す（外に出さない）
	const float START_X = right - 2.0f;

	// ★寿命は短め（流れ星っぽさ）
	std::uniform_real_distribution<float> lifeDist(0.55f, 0.75f);
	const float life = lifeDist(m_rng);

	// ★斜め率（元の -320 : +110 の比率 ≒ 0.34375）を少し揺らす
	std::uniform_real_distribution<float> slopeDist(0.28f, 0.42f);
	const float slope = slopeDist(m_rng); // vy = |vx| * slope

	// ★左端にちょうど到達する vx を計算（vxは負）
	const float dx = (START_X - KILL_LEFT);
	const float vx = -dx / life;
	const float vy = std::abs(vx) * slope;

	// ★Yは「寿命中に下に落ちても bottom を越えない」範囲から選ぶ
	const float Y_TOP_MARGIN = 60.0f;
	const float Y_BOTTOM_MARGIN = 40.0f;

	const float yMin = top + Y_TOP_MARGIN;
	const float yMax = (bottom - Y_BOTTOM_MARGIN) - (vy * life);
	if (yMax <= yMin) return;

	std::uniform_real_distribution<float> ydist(yMin, yMax);
	const float x = START_X;
	const float y = ydist(m_rng);
	const float z = 0.5f;

	// ★サイズ（必要ならここで一括調整）
	const float starScale = 5.0f;
	const float trailScale = 8.0f;

	// 尾方向（進行方向+π）
	const float dir = std::atan2(vy, vx);
	const float tail = dir + DirectX::XM_PI;
	const DirectX::XMFLOAT3 tailRot = { 0.0f, 0.0f, tail };

	// ===== 本体 =====
	ECS::EntityID star = m_coordinator->CreateEntity(
		TagComponent("shooting_star_head"),
		TransformComponent({ x, y, z }, tailRot, { 1,1,1 }),
		EffectComponent("EFK_SHOOTINGSTAR", false, true, tailRot, starScale)
	);

	// ===== 軌跡（早く見せる：最初から後ろに置く＋3本重ねる）=====
	const float vlen = std::sqrt(vx * vx + vy * vy);
	const float nx = (vlen > 0.0001f) ? (vx / vlen) : 0.0f;
	const float ny = (vlen > 0.0001f) ? (vy / vlen) : 0.0f;

	// ★ここを増やすほど「出始めが早い/尾が長い」
	const float behindStart[3] = { 14.0f, 34.0f, 58.0f };
	const float trailScaleArr[3] = { trailScale, trailScale * 0.85f, trailScale * 0.70f };

	ECS::EntityID t0 = m_coordinator->CreateEntity(
		TagComponent("shooting_star_trail0"),
		TransformComponent({ x - nx * behindStart[0], y - ny * behindStart[0], z }, tailRot, { 1,1,1 }),
		EffectComponent("EFK_SHOOTINGSTARPYUU", false, true, tailRot, trailScaleArr[0])
	);

	ECS::EntityID t1 = m_coordinator->CreateEntity(
		TagComponent("shooting_star_trail1"),
		TransformComponent({ x - nx * behindStart[1], y - ny * behindStart[1], z }, tailRot, { 1,1,1 }),
		EffectComponent("EFK_SHOOTINGSTARPYUU", false, true, tailRot, trailScaleArr[1])
	);

	ECS::EntityID t2 = m_coordinator->CreateEntity(
		TagComponent("shooting_star_trail2"),
		TransformComponent({ x - nx * behindStart[2], y - ny * behindStart[2], z }, tailRot, { 1,1,1 }),
		EffectComponent("EFK_SHOOTINGSTARPYUU", false, true, tailRot, trailScaleArr[2])
	);

	ShootingStarInstance inst;
	inst.star = star;
	inst.trails[0] = t0;
	inst.trails[1] = t1;
	inst.trails[2] = t2;

	inst.velocity = { vx, vy };
	inst.life = life;
	inst.remaining = life;

	m_activeShootingStars.push_back(inst);
}

void StageSelectScene::KillAllShootingStars()
{
	if (!m_coordinator) return;
	for (auto& s : m_activeShootingStars)
	{
		if (s.star != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.star);
		for (int k = 0; k < 3; ++k)
			if (s.trails[k] != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.trails[k]);
	}
	m_activeShootingStars.clear();
}
