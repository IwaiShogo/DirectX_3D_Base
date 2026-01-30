#include "Scene/SceneManager.h"
#include "Scene/ResultScene.h"
#include "Scene/GameScene.h"
#include "ECS/Systems/Gameplay/MapGenerationSystem.h"  

#include "Scene/StageSelectScene.h"

#include "Systems/Input.h"
#include "Scene/StageUnlockProgress.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Rendering/EffectSystem.h"

#include "ECS/Systems/Core/ScreenTransition.h"
#include "ECS/Components/Rendering/AnimationComponent.h"
#include "ECS/Components/Core/SoundComponent.h"

#include <DirectXMath.h>
#include <iostream>
#include <functional>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <random>
#include <cmath>

using namespace DirectX;
using namespace ECS;

// クラス外の静的変数は削除し、標準的なメンバ変数アクセスで実装します
ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

static ECS::EntityID s_lastHoveredEntity = (ECS::EntityID)-1;
static float s_animTime = 0.0f;

extern std::string GetItemIconPath(const std::string& itemID);

static float Clamp01(float x)
{
	if (x < 0.0f) return 0.0f;
	if (x > 1.0f) return 1.0f;
	return x;
}

static DirectX::XMFLOAT3 MakeSelectCardScale(float s)
{
	return DirectX::XMFLOAT3(-s, s, s);
}

static DirectX::XMFLOAT3 UIToWorld(float sx, float sy, float zWorld)
{
	constexpr float PIXELS_PER_UNIT = 100.0f;
	const float wx = (sx - SCREEN_WIDTH * 0.5f) / PIXELS_PER_UNIT;
	const float wy = -(sy - SCREEN_HEIGHT * 0.5f) / PIXELS_PER_UNIT;
	return DirectX::XMFLOAT3(wx, wy, zWorld);
}

static void SetListCardHitboxVisible(ECS::Coordinator* coord, ECS::EntityID id, bool visible)
{
	if (!coord || id == (ECS::EntityID)-1) return;

	if (coord->HasComponent<UIImageComponent>(id))
	{
		auto& ui = coord->GetComponent<UIImageComponent>(id);
		ui.isVisible = visible;
		ui.color.w = 0.0f;
	}
	if (coord->HasComponent<UIButtonComponent>(id))
	{
		coord->GetComponent<UIButtonComponent>(id).isVisible = visible;
	}
}

static DirectX::XMFLOAT3 Lerp3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
{
	return DirectX::XMFLOAT3(
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	);
}

static float SmoothStep01(float t)
{
	t = Clamp01(t);
	return t * t * (3.0f - 2.0f * t);
}

static bool IsStageSpecificUIAsset(const std::string& assetId)
{
	if (assetId.empty()) return false;
	if (assetId == "UI_TRESURE_CASE") return true;
	if (assetId.rfind("ICO_TREASURE", 0) == 0) return true;
	if (assetId.rfind("UI_ITEM_ICON", 0) == 0) return true;
	if (assetId.rfind("ICO_ITEM", 0) == 0) return true;
	return false;
}

static bool AssetIdContainsTreasure(const std::string& assetId)
{
	if (assetId.empty()) return false;
	if (assetId.find("TREASURE") != std::string::npos) return true;
	if (assetId.find("TRESURE") != std::string::npos) return true;
	return false;
}

static bool IsDetailOnlyUIAsset(const std::string& assetId)
{
	if (assetId.empty()) return false;
	if (AssetIdContainsTreasure(assetId)) return true;
	if (assetId.rfind("UI_ITEM_ICON", 0) == 0) return true;
	if (assetId.rfind("ICO_ITEM", 0) == 0) return true;
	return false;
}

static bool UIFont_GetIndex(char c, int& outIdx)
{
	if (c >= '0' && c <= '9') { outIdx = (int)(c - '0'); return true; }
	switch (c)
	{
	case '-': outIdx = 10; return true;
	case ':': outIdx = 11; return true;
	case '.': outIdx = 12; return true;
	default: break;
	}
	return false;
}

static void UIFont_ApplyChar(ECS::Coordinator* coord, ECS::EntityID id, char c)
{
	if (!coord || id == (ECS::EntityID)-1) return;
	if (!coord->HasComponent<UIImageComponent>(id)) return;

	auto& ui = coord->GetComponent<UIImageComponent>(id);

	int idx = 0;
	if (!UIFont_GetIndex(c, idx))
	{
		ui.isVisible = false;
		ui.color.w = 0.0f;
		return;
	}

	const int r = (idx <= 9) ? (idx / 5) : 2;
	const int col = (idx <= 9) ? (idx % 5) : (idx - 10);

	ui.uvPos = { col * 0.2f,  r * 0.333f };
	ui.uvScale = { 0.2f,     0.333f };
}

static std::string StageNoToLabelText(int stageNo)
{
	const int world = (stageNo - 1) / 3 + 1;
	const int sub = (stageNo - 1) % 3 + 1;
	return std::to_string(world) + "-" + std::to_string(sub);
}

DirectX::XMFLOAT3 StageSelectScene::GetListCardSlotCenterPos(int stageNo) const
{
	if (stageNo < 1 || stageNo > 18)
	{
		return DirectX::XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f);
	}

	int pageIndex = (stageNo - 1) / 6;
	int posIndex = (stageNo - 1) % 6;

	static const DirectX::XMFLOAT3 kPosPage1[6] =
	{
		{ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.28f, 0.0f },
		{ SCREEN_WIDTH * 0.49f, SCREEN_HEIGHT * 0.33f, 0.0f },
		{ SCREEN_WIDTH * 0.70f, SCREEN_HEIGHT * 0.28f, 0.0f },
		{ SCREEN_WIDTH * 0.30f, SCREEN_HEIGHT * 0.60f, 0.0f },
		{ SCREEN_WIDTH * 0.51f, SCREEN_HEIGHT * 0.65f, 0.0f },
		{ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.64f, 0.0f },
	};

	static const DirectX::XMFLOAT3 kPosPage2[6] =
	{
		{ SCREEN_WIDTH * 0.28f, SCREEN_HEIGHT * 0.32f, 0.0f },
		{ SCREEN_WIDTH * 0.50f, SCREEN_HEIGHT * 0.28f, 0.0f },
		{ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.30f, 0.0f },
		{ SCREEN_WIDTH * 0.26f, SCREEN_HEIGHT * 0.65f, 0.0f },
		{ SCREEN_WIDTH * 0.48f, SCREEN_HEIGHT * 0.60f, 0.0f },
		{ SCREEN_WIDTH * 0.70f, SCREEN_HEIGHT * 0.65f, 0.0f },
	};

	static const DirectX::XMFLOAT3 kPosPage3[6] =
	{
		{ SCREEN_WIDTH * 0.30f, SCREEN_HEIGHT * 0.27f, 0.0f },
		{ SCREEN_WIDTH * 0.50f, SCREEN_HEIGHT * 0.35f, 0.0f },
		{ SCREEN_WIDTH * 0.74f, SCREEN_HEIGHT * 0.29f, 0.0f },
		{ SCREEN_WIDTH * 0.26f, SCREEN_HEIGHT * 0.63f, 0.0f },
		{ SCREEN_WIDTH * 0.47f, SCREEN_HEIGHT * 0.65f, 0.0f },
		{ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.61f, 0.0f },
	};

	switch (pageIndex)
	{
	case 0: return kPosPage1[posIndex];
	case 1: return kPosPage2[posIndex];
	case 2: return kPosPage3[posIndex];
	default: return kPosPage1[posIndex];
	}
}

void StageSelectScene::StartCardFocusAnim(ECS::EntityID cardEntity, const DirectX::XMFLOAT3& uiPos)
{
	constexpr float kCardZ = 5.0f;
	constexpr float kStartScale = 0.03f;
	constexpr float kEndScaleMul = 5.0f;
	constexpr float kDuration = 1.5f;

	constexpr float kExtraRollRad = DirectX::XMConvertToRadians(50.0f);

	const float kCenterOffsetPxX = SCREEN_WIDTH * 0.0f;
	const float kCenterOffsetPxY = -SCREEN_HEIGHT * 0.0f;

	m_cardFocus.active = true;
	m_cardFocus.entity = cardEntity;
	m_cardFocus.elapsed = 0.0f;
	m_cardFocus.duration = kDuration;

	if (m_coordinator->HasComponent<TransformComponent>(cardEntity))
	{
		auto& transform = m_coordinator->GetComponent<TransformComponent>(cardEntity);
		m_cardFocus.startPos = UIToWorld(transform.position.x, transform.position.y, kCardZ);
	}
	else
	{
		m_cardFocus.startPos = UIToWorld(uiPos.x, uiPos.y, kCardZ);
	}

	m_cardFocus.endPos = UIToWorld(
		SCREEN_WIDTH * 0.5f + kCenterOffsetPxX,
		SCREEN_HEIGHT * 0.5f + kCenterOffsetPxY,
		kCardZ
	);

	const float endScale = kStartScale * kEndScaleMul;
	m_cardFocus.startScale = MakeSelectCardScale(kStartScale);
	m_cardFocus.endScale = MakeSelectCardScale(endScale);

	m_cardFocus.baseRot = DirectX::XMFLOAT3(0.0f, DirectX::XM_PI, 0.0f);
	m_cardFocus.extraRollRad = kExtraRollRad;

	{
		auto& tr = m_coordinator->GetComponent<TransformComponent>(cardEntity);
		tr.scale = m_cardFocus.startScale;
		tr.rotation = m_cardFocus.baseRot;
	}
}

void StageSelectScene::UpdateCardFocusAnim(float dt)
{
	if (!m_cardFocus.active) return;
	if (m_cardFocus.entity == (ECS::EntityID)-1) { m_cardFocus.active = false; return; }
	if (!m_coordinator->HasComponent<TransformComponent>(m_cardFocus.entity)) { m_cardFocus.active = false; return; }

	m_cardFocus.elapsed += dt;

	const float t = (m_cardFocus.duration > 0.0f) ? (m_cardFocus.elapsed / m_cardFocus.duration) : 1.0f;
	const float e = SmoothStep01(t);

	auto& tr = m_coordinator->GetComponent<TransformComponent>(m_cardFocus.entity);

	tr.position = Lerp3(m_cardFocus.startPos, m_cardFocus.endPos, e);
	tr.scale = Lerp3(m_cardFocus.startScale, m_cardFocus.endScale, e);

	if (t >= 1.0f)
	{
		m_cardFocus.active = false;
		tr.position = m_cardFocus.endPos;
		tr.scale = m_cardFocus.endScale;
	}
}

void StageSelectScene::DestroyFocusCard()
{
	if (!m_coordinator) { m_focusCardEntity = (ECS::EntityID)-1; return; }

	if (m_focusCardEntity != (ECS::EntityID)-1)
	{
		if (m_coordinator->HasComponent<AnimationComponent>(m_focusCardEntity))
		{
			m_coordinator->RemoveComponent<AnimationComponent>(m_focusCardEntity);
		}

		m_coordinator->DestroyEntity(m_focusCardEntity);
		m_focusCardEntity = (ECS::EntityID)-1;
	}

	m_cardFocus.active = false;
	m_cardFocus.entity = (ECS::EntityID)-1;
}

void StageSelectScene::Init()
{
	m_coordinator = std::make_shared<Coordinator>();
	s_coordinator = m_coordinator.get();
	ECSInitializer::InitECS(m_coordinator);

	// 前のシーンの音を止める
	for (auto const& entity : m_coordinator->GetActiveEntities()) {
		if (m_coordinator->HasComponent<SoundComponent>(entity)) {
			m_coordinator->GetComponent<SoundComponent>(entity).RequestStop();
		}
	}

	m_bgm = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_STAGESELECT", 0.5f);

	s_lastHoveredEntity = (ECS::EntityID)-1;
	s_animTime = 0.0f;

	LoadStageData();

	StageUnlockProgress::Load();
	m_maxUnlockedStage = StageUnlockProgress::GetMaxUnlockedStage();
	m_pendingRevealStage = StageUnlockProgress::ConsumePendingRevealStage();

	m_selectedStageID.clear();

	EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0,0,0 });

	if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
	{
		effectSystem->SetScreenSpaceCamera((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	}

	{
		std::random_device rd;
		m_rng = std::mt19937(rd());
		std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
		m_nextShootingStarWait = dist(m_rng);
		m_shootingStarTimer = 0.0f;
	}

	m_currentPage = 0;
	m_pages.resize(TOTAL_PAGES);

	std::vector<std::string> stageIDs;
	for (int i = 1; i <= TOTAL_STAGES; ++i)
	{
		char buf[16];
		sprintf_s(buf, "ST_%03d", i);
		stageIDs.push_back(buf);
	}

	m_listStageNos.clear();
	m_listStageNos.reserve(TOTAL_STAGES);

	m_listCardModelEntities.clear();
	m_listCardModelEntities.reserve(TOTAL_STAGES);

	m_listUIEntities.clear();
	m_listUIEntities.reserve(TOTAL_STAGES);

	float startX = SCREEN_WIDTH * 0.2f;
	float startY = SCREEN_HEIGHT * 0.3f;
	float gapX = 350.0f;
	float gapY = 250.0f;

	m_listBgEntity = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 5.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_STAGE_SELECT", 0.0f)
	);

	for (int i = 0; i < TOTAL_STAGES; ++i)
	{
		int pageIndex = i / STAGES_PER_PAGE;
		int indexInPage = i % STAGES_PER_PAGE;

		std::string id = stageIDs[i];

		float x = startX + (indexInPage % 3) * gapX;
		float y = startY + (indexInPage / 3) * gapY;

		{
			std::string modelID = kSelectCardModel[i];
			constexpr float kListCardZ = 4.8f;
			constexpr float kListCardScale = 0.03f;

			static const float kCardTiltPage1[6] = {
				DirectX::XMConvertToRadians(-7.0f),
				DirectX::XMConvertToRadians(5.0f),
				DirectX::XMConvertToRadians(-10.0f),
				DirectX::XMConvertToRadians(8.0f),
				DirectX::XMConvertToRadians(-4.0f),
				DirectX::XMConvertToRadians(6.0f),
			};
			static const float kCardTiltPage2[6] = {
				DirectX::XMConvertToRadians(6.0f),
				DirectX::XMConvertToRadians(-9.0f),
				DirectX::XMConvertToRadians(4.0f),
				DirectX::XMConvertToRadians(-6.0f),
				DirectX::XMConvertToRadians(10.0f),
				DirectX::XMConvertToRadians(-5.0f),
			};
			static const float kCardTiltPage3[6] = {
				DirectX::XMConvertToRadians(-5.0f),
				DirectX::XMConvertToRadians(8.0f),
				DirectX::XMConvertToRadians(-8.0f),
				DirectX::XMConvertToRadians(5.0f),
				DirectX::XMConvertToRadians(-6.0f),
				DirectX::XMConvertToRadians(9.0f),
			};
			float tilt = 0.0f;
			switch (pageIndex) {
			case 0: tilt = kCardTiltPage1[indexInPage]; break;
			case 1: tilt = kCardTiltPage2[indexInPage]; break;
			case 2: tilt = kCardTiltPage3[indexInPage]; break;
			}

			ECS::EntityID cardModel = m_coordinator->CreateEntity(
				TagComponent("list_card_model"),
				TransformComponent(UIToWorld(x, y, kListCardZ), { 0.0f, DirectX::XM_PI, tilt }, MakeSelectCardScale(kListCardScale)),
				RenderComponent(MESH_MODEL, { 1.0f, 1.0f, 1.0f, 1.0f }, CullMode::Front),
				ModelComponent(modelID, 5.0f, Model::None)
			);

			m_listCardModelEntities.push_back(cardModel);
			m_pages[pageIndex].cardModelEntities.push_back(cardModel);

			m_listCardModelBaseScale[cardModel] = MakeSelectCardScale(kListCardScale);
		}

		const int stageNo = i + 1;
		EntityID btn = m_coordinator->CreateEntity(
			TagComponent("list_card_hitbox"),
			TransformComponent({ x, y, 5.0f }, { 0,0,0 }, { 250, 150, 1 }),
			UIButtonComponent(
				ButtonState::Normal,
				true,
				[this, id, i, stageNo, pageIndex]() {
					if (m_inputLocked || m_isWaitingForTransition) return;
					if (m_isDetailMode) return;

					EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_DECISION", 0.5f);

					ResetSelectToDetailAnimState(false, true);

					ClearStageInfoUI();

					m_selectedStageID = id;
					UpdateBestTimeDigitsByStageId(m_selectedStageID);

					UpdateStarIconsByStageId(std::string());
					m_starRevealPending = true;
					m_starRevealStageId = m_selectedStageID;

					m_inputLocked = true;

					for (size_t j = 0; j < m_pages[m_currentPage].hitboxEntities.size(); ++j)
					{
						SetUIVisible(m_pages[m_currentPage].hitboxEntities[j], false);
						int targetStageNo = m_currentPage * STAGES_PER_PAGE + (int)j + 1;
						SetStageNumberLabelVisible(targetStageNo, false);
					}
					for (auto cardEntity : m_pages[m_currentPage].cardModelEntities)
					{
						SetUIVisible(cardEntity, false);
					}

					if (m_prevPageBtnEntity != (ECS::EntityID)-1) SetUIVisible(m_prevPageBtnEntity, false);
					if (m_nextPageBtnEntity != (ECS::EntityID)-1) SetUIVisible(m_nextPageBtnEntity, false);

					for (int p = 0; p < TOTAL_PAGES; ++p)
					{
						if (m_pageIndicatorEntities[p] != (ECS::EntityID)-1)
							SetUIVisible(m_pageIndicatorEntities[p], false);
					}

					EntityID currentBtnID = m_pages[m_currentPage].hitboxEntities[i % STAGES_PER_PAGE];
					m_lastHiddenListCardEntity = currentBtnID;
					m_lastHiddenListStageNo = stageNo;
					m_lastHiddenListCardModelEntity = m_listCardModelEntities[i];

					DirectX::XMFLOAT3 uiPos = GetListCardSlotCenterPos(stageNo);
					ECS::EntityID oldFocus = m_focusCardEntity;

					std::string focusModelID = kSelectCardModel[i];
					m_focusCardEntity = m_coordinator->CreateEntity(
						TagComponent("focus_card"),
						TransformComponent({ uiPos.x, uiPos.y, 5.0f }, { 0,0,0 }, { 1.0f, 1.0f, 1.0f }),
						RenderComponent(MESH_MODEL, { 1.0f, 1.0f, 1.0f, 1.0f }, CullMode::Front),
						ModelComponent(focusModelID, 5.0f, Model::None),
						AnimationComponent({ "A_CARD_COMEON" })
					);

					if (oldFocus != (ECS::EntityID)-1)
					{
						if (m_coordinator->HasComponent<AnimationComponent>(oldFocus))
						{
							m_coordinator->RemoveComponent<AnimationComponent>(oldFocus);
						}
						m_coordinator->DestroyEntity(oldFocus);
					}

					auto effectSystem = ECSInitializer::GetSystem<EffectSystem>();
					for (auto& pair : m_activeEyeLights)
					{
						if (pair.first != (ECS::EntityID)-1)
						{
							if (effectSystem) effectSystem->StopEffectImmediate(pair.first);
							m_coordinator->DestroyEntity(pair.first);
						}
					}
					m_activeEyeLights.clear();
					m_eyeLightTimer = 0.0f;
					m_eyeLightNextInterval = 1.0f;

					for (auto& fx : m_uiSelectFx)
					{
						if (fx.entity != (ECS::EntityID)-1)
						{
							if (effectSystem) effectSystem->StopEffectImmediate(fx.entity);
							m_coordinator->DestroyEntity(fx.entity);
						}
					}
					m_uiSelectFx.clear();

					KillAllShootingStars();

					if (m_debugStarEntity != (ECS::EntityID)-1)
					{
						if (effectSystem) effectSystem->StopEffectImmediate(m_debugStarEntity);
						m_coordinator->DestroyEntity(m_debugStarEntity);
						m_debugStarEntity = (ECS::EntityID)-1;
					}

					StartCardFocusAnim(m_focusCardEntity, uiPos);

					const float animSpeed = LIST_TO_DETAIL_ANIM_SPEED;
					if (m_coordinator->HasComponent<AnimationComponent>(m_focusCardEntity))
					{
						auto& anim = m_coordinator->GetComponent<AnimationComponent>(m_focusCardEntity);
						anim.Play("A_CARD_COMEON", false, animSpeed);
					}

					m_isWaitingForTransition = true;
					m_transitionWaitTimer = 0.0f;
					m_transitionDelayTime = LIST_TO_DETAIL_DELAY;
					m_pendingStageID = id;
				}
			)
		);

		m_listUIEntities.push_back(btn);
		m_pages[pageIndex].hitboxEntities.push_back(btn);
		m_listStageNos.push_back(stageNo);

		const bool unlocked = IsStageUnlocked(stageNo);
		if (!unlocked)
		{
			SetUIVisible(btn, false);
			SetUIVisible(m_listCardModelEntities[i], false);
		}

		if (pageIndex != 0)
		{
			SetUIVisible(m_listCardModelEntities[i], false);
			SetListCardHitboxVisible(m_coordinator.get(), btn, false);
		}
	}

	BuildStageNumberLabels();
	SyncStageNumberLabels(true);

	ReflowUnlockedCardsLayout();

	CreateNavigationButtons();
	CreatePageIndicators();
	UpdateNavigationButtons();
	UpdatePageIndicators();

	const float baseDepth = 110000.0f;

	EntityID mapBack = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 10.0f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE_MAPBACK", baseDepth + 1.0f)
	);
	m_detailUIEntities.push_back(mapBack);
	m_stageMapEntity = mapBack;

	EntityID mapOverlay = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 9.5f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE1", baseDepth + 1.5f)
	);
	if (m_coordinator->HasComponent<UIImageComponent>(mapOverlay))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(mapOverlay);
		ui.isVisible = false;
		ui.color.w = 0.0f;
	}
	m_detailUIEntities.push_back(mapOverlay);
	m_stageMapOverlayEntity = mapOverlay;

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.83f, 0.0f }, { 0,0,0 }, { 500, 160, 1 }),
		UIImageComponent("UI_FRAME", baseDepth + 3.0f)
	));

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.16f, SCREEN_HEIGHT * 0.83f, 0.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_BEST_TIME", baseDepth + 4.0f)
	));

	{
		m_bestTimeDigitEntities.clear();
		m_bestTimeDigitEntities.reserve(7);

		const float digitW = 32.0f;
		const float digitH = 48.0f;
		const float stepX = 34.0f;
		const float y = SCREEN_HEIGHT * 0.83f;
		const float labelCenterX = SCREEN_WIDTH * 0.16f;
		const float labelW = 200.0f;
		const float startX = (labelCenterX + labelW * 0.5f + 20.0f);

		for (int i = 0; i < 7; ++i)
		{
			EntityID d = m_coordinator->CreateEntity(
				TransformComponent({ startX + i * stepX, y, 0.0f }, { 0,0,0 }, { digitW, digitH, 1.0f }),
				UIImageComponent("UI_FONT", baseDepth + 4.5f, true, { 1,1,1,1 })
			);

			auto& ui = m_coordinator->GetComponent<UIImageComponent>(d);
			int idx = 10;
			if (i == 2) idx = 11;
			else if (i == 5) idx = 12;
			const int r = (idx <= 9) ? (idx / 5) : 2;
			const int c = (idx <= 9) ? (idx % 5) : (idx - 10);
			ui.uvPos = { c * 0.2f,  r * 0.333f };
			ui.uvScale = { 0.2f,     0.333f };

			if (i >= 5) ui.isVisible = false;

			m_bestTimeDigitEntities.push_back(d);
			m_detailUIEntities.push_back(d);
		}

		UpdateBestTimeDigitsByStageId(std::string());
		UpdateStarIconsByStageId(std::string());
	}

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.2f, 0.0f }, { 0,0,0 }, { 550, 220, 1 }),
		UIImageComponent("UI_TRESURE_BACK", baseDepth + 3.0f)
	));

	{
		const char* conditionTex[3] = { "STAR_TEXT1","STAR_TEXT2","STAR_TEXT3_3MINUTE" };

		float baseY = SCREEN_HEIGHT * 0.50f;
		float gapY = 70.0f;
		float starX = SCREEN_WIDTH * 0.6f;
		float captionX = SCREEN_WIDTH * 0.80f;

		for (int i = 0; i < 3; ++i)
		{
			float y = baseY + i * gapY;

			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 520.0f, 95.0f, 1.0f }),
				UIImageComponent(conditionTex[i], baseDepth + 5.0f, true, { 1,1,1,1 })
			));

			float offSize = (i == 0) ? 70.0f : 55.0f;
			float onSize = (i == 0) ? 70.0f : 55.0f;

			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { offSize, offSize, 1.0f }),
				UIImageComponent("ICO_STAR_OFF", baseDepth + 5.0f, true, { 0.0f,0.0f,0.0f,1.0f })
			));

			ECS::EntityID starOn = m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
				UIImageComponent("ICO_STAR_ON", baseDepth + 6.0f, true, { 1,1,1,1 })
			);
			if (m_coordinator->HasComponent<UIImageComponent>(starOn))
			{
				m_coordinator->GetComponent<UIImageComponent>(starOn).isVisible = false;
			}
			m_detailStarOnEntities[i] = starOn;
			m_detailUIEntities.push_back(starOn);
		}

		UpdateStarIconsByStageId(std::string());
	}

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_START_NORMAL", baseDepth + 5.0f)
	));

	EntityID startBtn = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("BTN_DECISION", baseDepth + 6.0f),
		UIButtonComponent(
			ButtonState::Normal,
			true,
			[this]() {
				if (m_inputLocked) return;
				EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_DECISION", 0.5f);

				PlayUISelectEffect(m_startBtnEntity, "EFK_SELECTOK", 35.0f);
				m_inputLocked = true;

				m_isWaitingForGameStart = true;
				m_gameStartTimer = 0.0f;

				ScreenTransition::RequestFadeOutEx(
					m_coordinator.get(), m_blackTransitionEntity, 0.15f, 0.35f, 0.45f,
					[this]() {
						m_coordinator->GetComponent<SoundComponent>(m_bgm).RequestStop();

						GameScene::SetStageNo(m_selectedStageID);
						SceneManager::ChangeScene<GameScene>();
					},
					false, nullptr, 0.0f, 0.35f, false, false
				);
			}
		)
	);
	m_startBtnEntity = startBtn;
	m_detailUIEntities.push_back(startBtn);

	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_FINISH_NORMAL", baseDepth + 5.0f)
	));

	EntityID backBtn = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 160, 80, 1 }),
		UIImageComponent("BTN_REBERSE", baseDepth + 6.0f),
		UIButtonComponent(
			ButtonState::Normal,
			true,
			[this]() {
				if (m_inputLocked) return;
				PlayUISelectEffect(m_finishBtnEntity, "EFK_SELECTBACK", 35.0f);
				m_inputLocked = true;
				ScreenTransition::RequestFadeOutEx(
					m_coordinator.get(), m_blackTransitionEntity, 0.15f, 0.35f, 0.45f,
					[this]() { SwitchState(false); },
					true, [this]() { m_inputLocked = false; }, 0.0f, 0.35f, false, false
				);
			}
		)
	);
	m_finishBtnEntity = backBtn;
	m_detailUIEntities.push_back(backBtn);

	for (auto id : m_detailUIEntities) { CacheDetailBaseTransform(id); }

	m_cursorEntity = m_coordinator->CreateEntity(
		TransformComponent({ 0.0f, 0.0f, 0.0f }, { 0,0,0 }, { 64.0f, 64.0f, 1.0f }),
		UIImageComponent("ICO_CURSOR", 200000.0f),
		UICursorComponent()
	);

	SwitchState(false);

	const float fadeX = SCREEN_WIDTH * 0.5f;
	const float fadeY = SCREEN_HEIGHT * 0.5f;
	const float fadeZ = 0.0f;
	const float fadeW = SCREEN_WIDTH;
	const float fadeH = SCREEN_HEIGHT;

	m_transitionEntity = m_coordinator->CreateEntity(
		TransformComponent({ fadeX, fadeY, fadeZ }, { 0,0,0 }, { fadeW, fadeH, 1.0f }),
		UIImageComponent("FADE", 200000.0f),
		ScreenTransitionComponent()
	);

	m_blackTransitionEntity = m_coordinator->CreateEntity(
		TransformComponent({ fadeX, fadeY, fadeZ }, { 0,0,0 }, { fadeW, fadeH, 1.0f }),
		UIImageComponent("BLACK_FADE", 200001.0f),
		ScreenTransitionComponent()
	);

	m_inputLocked = true;

	ScreenTransition::RequestFadeIn(
		m_coordinator.get(),
		m_blackTransitionEntity,
		0.0f,
		[this]() { m_inputLocked = false; },
		0.5f
	);

	if (m_pendingRevealStage != -1)
	{
		int targetPage = (m_pendingRevealStage - 1) / STAGES_PER_PAGE;

		if (targetPage != m_currentPage && targetPage >= 0 && targetPage < TOTAL_PAGES)
		{
			m_currentPage = targetPage;

			for (int p = 0; p < TOTAL_PAGES; ++p)
			{
				if (p != m_currentPage)
				{
					HidePage(p);
				}
			}

			ShowPage(m_currentPage);
			UpdateNavigationButtons();
			UpdatePageIndicators();
		}

		m_scheduledRevealStage = m_pendingRevealStage;
		m_pendingRevealStage = -1;
	}

	{
		std::uniform_real_distribution<float> dist(1.0f, 3.0f);
		m_eyeLightNextInterval = dist(m_rng);
		m_eyeLightTimer = 0.0f;
	}

	if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
	{
		effectSystem->SetScreenSpaceCamera((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	}

	for (int p = 1; p < TOTAL_PAGES; ++p)
	{
		HidePage(p);
	}
}


void StageSelectScene::ResetSelectToDetailAnimState(bool unlockInput, bool keepFocusCard)
{
	m_isWaitingForTransition = false;
	m_transitionWaitTimer = 0.0f;
	m_transitionDelayTime = 1.0f;
	m_pendingStageID.clear();
	m_starRevealPending = false;
	m_starRevealStageId.clear();

	m_cardFocus.active = false;
	m_cardFocus.elapsed = 0.0f;
	m_cardFocus.entity = (ECS::EntityID)-1;

	if (!keepFocusCard)
	{
		DestroyFocusCard();
	}

	if (m_lastHiddenListCardEntity != (ECS::EntityID)-1)
	{
		SetUIVisible(m_lastHiddenListCardEntity, true);
		m_lastHiddenListCardEntity = (ECS::EntityID)-1;
	}

	if (m_lastHiddenListCardModelEntity != (ECS::EntityID)-1)
	{
		SetUIVisible(m_lastHiddenListCardModelEntity, true);
		m_lastHiddenListCardModelEntity = (ECS::EntityID)-1;
	}
	if (m_lastHiddenListStageNo != -1)
	{
		SetStageNumberLabelVisible(m_lastHiddenListStageNo, true);
		m_lastHiddenListStageNo = -1;
	}

	m_detailAppearActive = false;
	m_detailAppearTimer = 0.0f;

	if (unlockInput)
	{
		m_inputLocked = false;
	}
}
void StageSelectScene::Uninit()
{
	if (m_coordinator)
	{
		for (auto const& entity : m_coordinator->GetActiveEntities()) {
			if (m_coordinator->HasComponent<SoundComponent>(entity)) {
				m_coordinator->GetComponent<SoundComponent>(entity).RequestStop();
			}
		}

		// ★追加: AudioSystemを強制的に更新し、上記の停止リクエストを即座に処理させる
		// これを行わないと、リクエストが処理される前にシーンが破棄されて音が残り続けます
		if (auto audioSystem = ECSInitializer::GetSystem<AudioSystem>())
		{
			audioSystem->Update(0.0f);
		}
	}

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
	float dtSec = deltaTime;
	if (dtSec >= 10.0f)
	{
		dtSec *= 0.001f;
	}
	else if (dtSec >= 0.9f && dtSec <= 1.1f)
	{
		dtSec = 1.0f / 60.0f;
	}

	if (!m_isDetailMode && !m_isWaitingForTransition)
	{
		s_animTime += dtSec;
	}

	if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
	{
		effectSystem->SetScreenSpaceCamera((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	}

	UpdatePageNavigation(dtSec);

	for (auto& pair : m_stageReveal)
	{
		if (pair.second.active && pair.second.elapsed == 0.0f)
		{
			int revealStageNo = pair.first;
			int targetPage = (revealStageNo - 1) / STAGES_PER_PAGE;

			if (targetPage != m_currentPage && targetPage >= 0 && targetPage < TOTAL_PAGES)
			{
				SwitchToPage(targetPage);
			}
		}
	}


	m_coordinator->UpdateSystems(dtSec);

	UpdateShootingStar(dtSec);
	UpdateActiveShootingStars(dtSec);
	UpdateEyeLight(dtSec);
	UpdateUISelectFx(dtSec);
	UpdateDetailAppear(dtSec);
	UpdateButtonHoverScale(dtSec);

	SyncStageNumberLabels();

	if (m_scheduledRevealStage != -1)
	{
		const bool isFading =
			ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
			ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity);

		if (!isFading)
		{
			constexpr float kRevealDelayAfterFadeIn = 0.50f;
			m_revealDelayTimer += dtSec;

			if (m_revealDelayTimer >= kRevealDelayAfterFadeIn)
			{
				BeginStageReveal(m_scheduledRevealStage);
				m_scheduledRevealStage = -1;
				m_revealDelayTimer = 0.0f;
			}
		}
		else
		{
			m_revealDelayTimer = 0.0f;
		}
	}

	UpdateStageReveal(dtSec);
	UpdateCardFocusAnim(dtSec);

	if (m_isWaitingForTransition)
	{
		m_transitionWaitTimer += dtSec;

		if (m_transitionWaitTimer >= m_transitionDelayTime)
		{
			m_isWaitingForTransition = false;
			m_inputLocked = false;

			SwitchState(true);
		}
	}

	if (m_isWaitingForGameStart)
	{
		m_gameStartTimer += dtSec;

		if (m_gameStartTimer >= 1.0f)
		{
			m_isWaitingForGameStart = false;

			ScreenTransition::RequestFadeOutEx(
				m_coordinator.get(), m_blackTransitionEntity, 0.15f, 0.35f, 0.45f,
				[this]() {
					m_coordinator->GetComponent<SoundComponent>(m_bgm).RequestStop();

					GameScene::SetStageNo(m_selectedStageID);
					SceneManager::ChangeScene<GameScene>();
				},
				false, nullptr, 0.0f, 0.35f, false, false
			);
		}
	}

	if (!m_isDetailMode && !m_isWaitingForTransition)
	{
		for (int i = 0; i < (int)m_listCardModelEntities.size(); ++i)
		{
			ECS::EntityID mdl = m_listCardModelEntities[i];
			if (mdl == (ECS::EntityID)-1) continue;
			if (!m_coordinator->HasComponent<TransformComponent>(mdl)) continue;
			if (!m_coordinator->HasComponent<RenderComponent>(mdl)) continue;

			if (m_coordinator->GetComponent<RenderComponent>(mdl).color.w <= 0.0f) continue;

			const int stageNo = i + 1;
			DirectX::XMFLOAT3 baseUiPos = GetListCardSlotCenterPos(stageNo);
			DirectX::XMFLOAT3 baseWorldPos = UIToWorld(baseUiPos.x, baseUiPos.y, 4.8f);

			float phase = s_animTime * 1.5f + (float)i * 0.7f;
			float floatY = std::sin(phase) * 0.05f;

			auto& tr = m_coordinator->GetComponent<TransformComponent>(mdl);
			tr.position.y = baseWorldPos.y + floatY;
			tr.rotation.y = DirectX::XM_PI + std::sin(phase * 0.5f) * 0.05f;
		}
	}
}


void StageSelectScene::Draw()
{
	auto ui = ECSInitializer::GetSystem<UIRenderSystem>();
	auto rs = ECSInitializer::GetSystem<RenderSystem>();
	auto fx = ECSInitializer::GetSystem<EffectSystem>();

	if (!ui)
	{
		if (rs) { rs->DrawSetup(); rs->DrawEntities(); }
		if (fx) fx->Render();
		return;
	}

	const float kNormalUIEnd = 100000.0f;
	const float kFadeStart = 200000.0f;

	struct VisState { ECS::EntityID id; bool uiVis; bool btnVis; };
	std::vector<VisState> savedStates;

	std::vector<ECS::EntityID> allTargets = m_listUIEntities;

	int startIdx = m_currentPage * STAGES_PER_PAGE;
	int endIdx = startIdx + STAGES_PER_PAGE;
	if (endIdx > TOTAL_STAGES) endIdx = TOTAL_STAGES;

	for (int i = startIdx; i < endIdx; ++i)
	{
		if (i < (int)m_listStageLabelEntities.size())
		{
			auto& v = m_listStageLabelEntities[i];
			allTargets.insert(allTargets.end(), v.begin(), v.end());
		}
		if (i < (int)m_listStageLabelBoldEntities.size())
		{
			auto& v = m_listStageLabelBoldEntities[i];
			allTargets.insert(allTargets.end(), v.begin(), v.end());
		}
		if (i < (int)m_listStageLabelBgEntities.size())
		{
			ECS::EntityID bgId = m_listStageLabelBgEntities[i];
			if (bgId != (ECS::EntityID)-1) allTargets.push_back(bgId);
		}
	}

	for (auto& kv : m_stageReveal)
	{
		if (kv.second.active)
		{
			int idx = kv.first - 1;
			if (idx < startIdx || idx >= endIdx)
			{
				if (idx < (int)m_listStageLabelEntities.size())
				{
					auto& v = m_listStageLabelEntities[idx];
					allTargets.insert(allTargets.end(), v.begin(), v.end());
				}
				if (idx < (int)m_listStageLabelBoldEntities.size())
				{
					auto& v = m_listStageLabelBoldEntities[idx];
					allTargets.insert(allTargets.end(), v.begin(), v.end());
				}
				if (idx < (int)m_listStageLabelBgEntities.size())
				{
					ECS::EntityID bgId = m_listStageLabelBgEntities[idx];
					if (bgId != (ECS::EntityID)-1) allTargets.push_back(bgId);
				}
			}
		}
	}

	allTargets.insert(allTargets.end(), m_detailUIEntities.begin(), m_detailUIEntities.end());
	if (m_cursorEntity != (ECS::EntityID)-1) allTargets.push_back(m_cursorEntity);
	if (m_listBgEntity != (ECS::EntityID)-1) allTargets.push_back(m_listBgEntity);
	if (m_transitionEntity != (ECS::EntityID)-1) allTargets.push_back(m_transitionEntity);
	if (m_blackTransitionEntity != (ECS::EntityID)-1) allTargets.push_back(m_blackTransitionEntity);

	for (auto id : allTargets)
	{
		if (id == (ECS::EntityID)-1) continue;
		VisState s{ id,false,false };
		if (m_coordinator->HasComponent<UIImageComponent>(id))
			s.uiVis = m_coordinator->GetComponent<UIImageComponent>(id).isVisible;
		if (m_coordinator->HasComponent<UIButtonComponent>(id))
			s.btnVis = m_coordinator->GetComponent<UIButtonComponent>(id).isVisible;
		savedStates.push_back(s);
	}

	auto SetVisible = [&](ECS::EntityID id, bool v)
		{
			if (id == (ECS::EntityID)-1) return;
			if (m_coordinator->HasComponent<UIImageComponent>(id))
				m_coordinator->GetComponent<UIImageComponent>(id).isVisible = v;
			if (m_coordinator->HasComponent<UIButtonComponent>(id))
				m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = v;
		};

	auto GetDepth = [&](ECS::EntityID id) -> float
		{
			if (!m_coordinator->HasComponent<UIImageComponent>(id)) return 0.0f;
			return m_coordinator->GetComponent<UIImageComponent>(id).depth;
		};

	auto DrawUIOnce = [&]()
		{
			ui->Render(true);
			ui->Render(false);
		};

	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;
		bool draw = false;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = (d <= 0.0f) && s.uiVis;
		}
		SetVisible(s.id, draw);
	}
	DrawUIOnce();

	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;
		bool draw = false;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = (d > 0.0f && d <= kNormalUIEnd) && s.uiVis;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = draw || (s.btnVis && (d > 0.0f && d <= kNormalUIEnd));
		}
		SetVisible(s.id, draw);
	}
	DrawUIOnce();

	if (rs)
	{
		rs->DrawSetup();
		rs->DrawEntities();
	}

	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;
		bool draw = false;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = (d > kNormalUIEnd && d < kFadeStart) && s.uiVis;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = draw || (s.btnVis && (d > kNormalUIEnd && d < kFadeStart));
		}
		SetVisible(s.id, draw);
	}
	DrawUIOnce();

	if (fx)
	{
		fx->Render();
	}

	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;
		bool draw = false;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = (d >= kFadeStart) && s.uiVis;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = draw || (s.btnVis && (d >= kFadeStart));
		}
		SetVisible(s.id, draw);
	}
	DrawUIOnce();

	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
			m_coordinator->GetComponent<UIImageComponent>(s.id).isVisible = s.uiVis;
		if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
			m_coordinator->GetComponent<UIButtonComponent>(s.id).isVisible = s.btnVis;
	}
}
void StageSelectScene::UpdateShootingStar(float dt)
{
	if (!m_enableShootingStar) return;
	if (!m_isDetailMode) return;

	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;

	EnsureDebugEffectOnMap();

	if (m_stageMapEntity == (ECS::EntityID)-1) return;
	if (!m_coordinator->HasComponent<UIImageComponent>(m_stageMapEntity)) return;

	if (m_spawnStarOnEnterDetail)
	{
		m_spawnStarOnEnterDetail = false;
		SpawnShootingStar();

		m_shootingStarTimer = 0.0f;
		std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
		m_nextShootingStarWait = dist(m_rng);
		return;
	}

	m_shootingStarTimer += dt;
	if (m_shootingStarTimer < m_nextShootingStarWait) return;

	m_shootingStarTimer = 0.0f;
	std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
	m_nextShootingStarWait = dist(m_rng);

	SpawnShootingStar();
}

void StageSelectScene::UpdateUISelectFx(float dt)
{
	if (!m_coordinator) return;

	for (int i = (int)m_uiSelectFx.size() - 1; i >= 0; --i)
	{
		auto& fx = m_uiSelectFx[i];
		fx.remaining -= dt;

		if (fx.entity == (ECS::EntityID)-1)
		{
			m_uiSelectFx.erase(m_uiSelectFx.begin() + i);
			continue;
		}

		if (!m_coordinator->HasComponent<EffectComponent>(fx.entity))
		{
			m_uiSelectFx.erase(m_uiSelectFx.begin() + i);
			continue;
		}

		auto& ec = m_coordinator->GetComponent<EffectComponent>(fx.entity);

		if (fx.remaining <= 0.0f && ec.handle == -1)
		{
			m_coordinator->DestroyEntity(fx.entity);
			m_uiSelectFx.erase(m_uiSelectFx.begin() + i);
		}
	}
}

void StageSelectScene::UpdateButtonHoverScale(float dt)
{
	if (!m_coordinator) return;
	if (m_cursorEntity == (ECS::EntityID)-1) return;
	if (!m_coordinator->HasComponent<TransformComponent>(m_cursorEntity)) return;

	const auto& curTr = m_coordinator->GetComponent<TransformComponent>(m_cursorEntity);
	const float cx = curTr.position.x;
	const float cy = curTr.position.y;

	const bool allowHover = !(m_inputLocked ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity));
	const float lerpK = 14.0f;
	const float a = Clamp01(lerpK * dt);

	const float hoverMulBtn = 1.10f;
	const float hoverMulModel = 1.10f;

	bool anyHovered = false;

	auto updateOne = [&](ECS::EntityID id)
		{
			if (id == (ECS::EntityID)-1) return;
			if (!m_coordinator->HasComponent<UIButtonComponent>(id)) return;

			auto& btn = m_coordinator->GetComponent<UIButtonComponent>(id);
			if (!btn.isVisible) return;

			if (!m_coordinator->HasComponent<TransformComponent>(id)) return;
			auto& tr = m_coordinator->GetComponent<TransformComponent>(id);

			auto it = m_buttonBaseScale.find(id);
			if (it == m_buttonBaseScale.end())
			{
				it = m_buttonBaseScale.emplace(id, tr.scale).first;
			}
			const DirectX::XMFLOAT3 base = it->second;

			const float left = tr.position.x - base.x * 0.5f;
			const float right = tr.position.x + base.x * 0.5f;
			const float top = tr.position.y - base.y * 0.5f;
			const float bottom = tr.position.y + base.y * 0.5f;

			const bool hovered = allowHover && (cx >= left && cx <= right && cy >= top && cy <= bottom);

			if (hovered)
			{
				anyHovered = true;
				if (s_lastHoveredEntity != id)
				{
					EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_CURSOR", 0.5f);
					s_lastHoveredEntity = id;
				}
			}

			const float targetMul = hovered ? hoverMulBtn : 1.0f;
			const float curMul = (base.x != 0.0f) ? (tr.scale.x / base.x) : 1.0f;
			const float newMul = curMul + (targetMul - curMul) * a;

			tr.scale.x = base.x * newMul;
			tr.scale.y = base.y * newMul;
			tr.scale.z = base.z;
		};

	if (!m_isDetailMode)
	{
		const int n = (int)std::min(m_listUIEntities.size(), m_listCardModelEntities.size());
		for (int i = 0; i < n; ++i)
		{
			const ECS::EntityID hit = m_listUIEntities[i];
			const ECS::EntityID mdl = m_listCardModelEntities[i];

			if (hit == (ECS::EntityID)-1 || mdl == (ECS::EntityID)-1) continue;
			if (!m_coordinator->HasComponent<UIButtonComponent>(hit)) continue;
			if (!m_coordinator->HasComponent<TransformComponent>(hit)) continue;
			if (!m_coordinator->HasComponent<TransformComponent>(mdl)) continue;

			auto& btn = m_coordinator->GetComponent<UIButtonComponent>(hit);
			if (!btn.isVisible) continue;

			auto& hitTr = m_coordinator->GetComponent<TransformComponent>(hit);

			auto itHitBase = m_buttonBaseScale.find(hit);
			if (itHitBase == m_buttonBaseScale.end())
			{
				itHitBase = m_buttonBaseScale.emplace(hit, hitTr.scale).first;
			}
			const DirectX::XMFLOAT3 hitBase = itHitBase->second;

			const float left = hitTr.position.x - hitBase.x * 0.5f;
			const float right = hitTr.position.x + hitBase.x * 0.5f;
			const float top = hitTr.position.y - hitBase.y * 0.5f;
			const float bottom = hitTr.position.y + hitBase.y * 0.5f;

			const bool hovered = allowHover && (cx >= left && cx <= right && cy >= top && cy <= bottom);

			if (hovered)
			{
				anyHovered = true;
				if (s_lastHoveredEntity != hit)
				{
					EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_CURSOR", 0.5f);
					s_lastHoveredEntity = hit;
				}
			}

			auto& mdlTr = m_coordinator->GetComponent<TransformComponent>(mdl);
			auto itMdlBase = m_listCardModelBaseScale.find(mdl);
			if (itMdlBase == m_listCardModelBaseScale.end())
			{
				itMdlBase = m_listCardModelBaseScale.emplace(mdl, mdlTr.scale).first;
			}
			const DirectX::XMFLOAT3 mdlBase = itMdlBase->second;

			const float targetMul = hovered ? hoverMulModel : 1.0f;
			const float curMul = (mdlBase.x != 0.0f) ? (mdlTr.scale.x / mdlBase.x) : 1.0f;
			const float newMul = curMul + (targetMul - curMul) * a;

			mdlTr.scale.x = mdlBase.x * newMul;
			mdlTr.scale.y = mdlBase.y * newMul;
			mdlTr.scale.z = mdlBase.z * newMul;
		}

		if (m_prevPageBtnEntity != (ECS::EntityID)-1) updateOne(m_prevPageBtnEntity);
		if (m_nextPageBtnEntity != (ECS::EntityID)-1) updateOne(m_nextPageBtnEntity);
	}
	for (auto id : m_detailUIEntities) updateOne(id);

	if (!anyHovered)
	{
		s_lastHoveredEntity = (ECS::EntityID)-1;
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
	const bool wasDetail = m_isDetailMode;
	m_isDetailMode = toDetail;

	std::cout << "[StageSelectScene] SwitchState toDetail=" << (toDetail ? 1 : 0) << " wasDetail=" << (wasDetail ? 1 : 0) << std::endl;

	ResetSelectToDetailAnimState(false, toDetail);

	if (!toDetail)
	{
		DestroyFocusCard();

		ClearStageInfoUI();
		KillAllShootingStars();
		if (m_debugStarEntity != (ECS::EntityID)-1)
		{
			m_coordinator->DestroyEntity(m_debugStarEntity);
			m_debugStarEntity = (ECS::EntityID)-1;
		}
		m_selectedStageID.clear();
		UpdateBestTimeDigitsByStageId(std::string());
		UpdateStarIconsByStageId(std::string());
		m_starRevealPending = false;
		m_starRevealStageId.clear();
		m_spawnStarOnEnterDetail = false;

		for (int p = 0; p < TOTAL_PAGES; ++p)
		{
			HidePage(p);
		}
	}

	SetUIVisible(m_listBgEntity, !toDetail);

	if (toDetail)
	{
		for (int i = 0; i < (int)m_listUIEntities.size(); ++i)
		{
			SetUIVisible(m_listUIEntities[i], false);
		}
		for (int i = 0; i < (int)m_listCardModelEntities.size(); ++i)
		{
			SetUIVisible(m_listCardModelEntities[i], false);
		}
		for (int stageNo = 1; stageNo <= TOTAL_STAGES; ++stageNo)
		{
			SetStageNumberLabelVisible(stageNo, false);
		}
	}

	if (!toDetail)
	{
		ReflowUnlockedCardsLayout();
		ShowPage(m_currentPage);
		UpdateNavigationButtons();
		UpdatePageIndicators();
	}

	for (auto id : m_detailUIEntities)
	{
		SetUIVisible(id, toDetail);
	}

	if (!toDetail)
	{
		std::vector<ECS::EntityID> forcedHidden;
		for (auto id : m_coordinator->GetActiveEntities())
		{
			if (id == (ECS::EntityID)-1) continue;
			if (!m_coordinator->HasComponent<UIImageComponent>(id)) continue;

			auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
			if (!IsDetailOnlyUIAsset(ui.assetID)) continue;

			ui.isVisible = false;
			ui.color.w = 0.0f;

			if (m_coordinator->HasComponent<UIButtonComponent>(id))
			{
				m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
			}

			forcedHidden.push_back(id);
		}

		if (!forcedHidden.empty())
		{
			std::cout << "[StageSelectScene] SwitchState forced-hide detail-only count=" << forcedHidden.size() << " ids=";
			for (auto id : forcedHidden) { std::cout << id << " "; }
			std::cout << std::endl;
		}
	}

	if (toDetail)
	{
		UpdateStarIconsByStageId(std::string());

		if (!m_selectedStageID.empty())
		{
			m_starRevealPending = true;
			m_starRevealStageId = m_selectedStageID;
		}
		else
		{
			m_starRevealPending = false;
			m_starRevealStageId.clear();
		}
	}


	if (m_cursorEntity != (ECS::EntityID)-1)
	{
		if (m_coordinator->HasComponent<UIImageComponent>(m_cursorEntity))
		{
			m_coordinator->GetComponent<UIImageComponent>(m_cursorEntity).isVisible = true;
		}
	}

	if (toDetail)
	{
		auto effectSystem = ECSInitializer::GetSystem<EffectSystem>();

		for (auto& pair : m_activeEyeLights)
		{
			if (pair.first != (ECS::EntityID)-1)
			{
				if (effectSystem)
				{
					effectSystem->StopEffectImmediate(pair.first);
				}

				m_coordinator->DestroyEntity(pair.first);
			}
		}
		m_activeEyeLights.clear();

		if (!wasDetail)
		{
			ApplyStageMapTextureByStageId(m_selectedStageID);
			CreateStageInfoUI(m_selectedStageID);
			BeginDetailAppear();
			m_shootingStarTimer = 0.0f;
			std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
			m_nextShootingStarWait = dist(m_rng);
			m_spawnStarOnEnterDetail = true;

			m_eyeLightTimer = 0.0f;
			m_eyeLightNextInterval = 1.0f;
		}
	}
}


int StageSelectScene::StageIdToStageNo(const std::string& stageId) const
{
	if (stageId.size() >= 6 && stageId.rfind("ST_", 0) == 0)
	{
		try
		{
			int n = std::stoi(stageId.substr(3));
			return n;
		}
		catch (...) {}
	}

	if (stageId.rfind("Stage_", 0) == 0)
	{
		try
		{
			int n = std::stoi(stageId.substr(6));
			return n;
		}
		catch (...) {}
	}

	return -1;
}

void StageSelectScene::UpdateBestTimeDigitsByStageId(const std::string& stageId)
{
	if (!m_coordinator) return;
	if (m_bestTimeDigitEntities.size() != 7) return;

	const int stageNo = StageIdToStageNo(stageId);

	uint32_t bestMs = 0;
	if (stageNo >= 1 && stageNo <= 18)
	{
		bestMs = StageUnlockProgress::GetBestTimeMs(stageNo);
	}

	int digits[7] = { 10,10,11,10,10,12,10 };
	bool showDecimal = false;

	if (bestMs != 0)
	{
		const int t10 = static_cast<int>((bestMs + 50) / 100);
		const int mm = (t10 / 600) % 100;
		const int ss = (t10 / 10) % 60;
		const int ds = (t10 % 10);

		digits[0] = (mm / 10) % 10;
		digits[1] = (mm % 10);
		digits[2] = 11;
		digits[3] = (ss / 10) % 10;
		digits[4] = (ss % 10);
		digits[5] = 12;
		digits[6] = ds;
		showDecimal = true;
	}

	for (int i = 0; i < 7; ++i)
	{
		EntityID e = m_bestTimeDigitEntities[i];
		if (e == (ECS::EntityID)-1) continue;
		if (!m_coordinator->HasComponent<UIImageComponent>(e)) continue;

		auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);

		const int idx = digits[i];
		const int r = (idx <= 9) ? (idx / 5) : 2;
		const int c = (idx <= 9) ? (idx % 5) : (idx - 10);
		ui.uvPos = { c * 0.2f,  r * 0.333f };
		ui.uvScale = { 0.2f,     0.333f };

		if (i >= 5) ui.isVisible = showDecimal;
		else ui.isVisible = true;
	}
}


void StageSelectScene::UpdateStarIconsByStageId(const std::string& stageId)
{
	if (!m_coordinator) return;

	const int stageNo = StageIdToStageNo(stageId);
	const std::uint8_t mask =
		(stageNo >= 1 && stageNo <= 18) ? StageUnlockProgress::GetStageStarMask(stageNo) : 0;

	for (int i = 0; i < 3; ++i)
	{
		ECS::EntityID e = m_detailStarOnEntities[i];
		if (e == (ECS::EntityID)-1) continue;
		if (!m_coordinator->HasComponent<UIImageComponent>(e)) continue;

		auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);
		const bool on = ((mask >> i) & 0x1) != 0;

		ui.isVisible = on;
		ui.color.w = on ? 1.0f : 0.0f;
	}
}

static int ExtractStageNoFromStageId(const std::string& stageId)
{
	int v = 0;
	bool inDigits = false;
	for (char c : stageId)
	{
		if (c >= '0' && c <= '9')
		{
			inDigits = true;
			v = v * 10 + (c - '0');
		}
		else if (inDigits)
		{
			break;
		}
	}
	return (v >= 1 && v <= 18) ? v : 0;
}


std::string StageSelectScene::GetStageMapTextureAssetId(int stageNo) const
{
	switch (stageNo)
	{
	case 1:  return "UI_STAGE1";
	case 2:  return "UI_STAGE2";
	case 3:  return "UI_STAGE3";
	case 4:  return "UI_STAGE4";
	case 5:  return "UI_STAGE5";
	case 6:  return "UI_STAGE6";
	case 7:  return "UI_STAGE1";
	case 8:  return "UI_STAGE2";
	case 9:  return "UI_STAGE3";
	case 10: return "UI_STAGE4";
	case 11: return "UI_STAGE5";
	case 12: return "UI_STAGE6";
	case 13: return "UI_STAGE1";
	case 14: return "UI_STAGE2";
	case 15: return "UI_STAGE3";
	case 16: return "UI_STAGE4";
	case 17: return "UI_STAGE5";
	case 18: return "UI_STAGE6";
	default: return "UI_STAGE1";
	}
}


void StageSelectScene::ApplyStageMapTextureByStageId(const std::string& stageId)
{
	if (m_stageMapOverlayEntity == (ECS::EntityID)-1) return;

	int stageNo = ExtractStageNoFromStageId(stageId);

	const bool valid = (stageNo >= 1 && stageNo <= 18);

	const std::string texId = valid ? GetStageMapTextureAssetId(stageNo) : std::string("UI_STAGE1");

	const ECS::EntityID oldId = m_stageMapOverlayEntity;

	if (!m_coordinator->HasComponent<TransformComponent>(oldId) ||
		!m_coordinator->HasComponent<UIImageComponent>(oldId))
	{
		return;
	}

	const auto oldTr = m_coordinator->GetComponent<TransformComponent>(oldId);
	const auto oldUi = m_coordinator->GetComponent<UIImageComponent>(oldId);

	ECS::EntityID newId = m_coordinator->CreateEntity(
		TransformComponent(oldTr.position, oldTr.rotation, oldTr.scale),
		UIImageComponent(texId, oldUi.depth)
	);

	if (m_coordinator->HasComponent<UIImageComponent>(newId))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(newId);
		ui.isVisible = valid;
		ui.color.w = valid ? 1.0f : 0.0f;
	}

	m_detailUIEntities.push_back(newId);

	for (auto it = m_detailUIEntities.begin(); it != m_detailUIEntities.end(); ++it)
	{
		if (*it == oldId)
		{
			m_detailUIEntities.erase(it);
			break;
		}
	}

	m_coordinator->DestroyEntity(oldId);

	m_stageMapOverlayEntity = newId;
}

void StageSelectScene::CacheDetailBaseTransform(ECS::EntityID id)
{
	if (!m_coordinator) return;
	if (!m_coordinator->HasComponent<TransformComponent>(id)) return;

	const auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
	m_detailBaseScale[id] = tr.scale;
	m_detailBasePos[id] = tr.position;
}

void StageSelectScene::BeginDetailAppear()
{
	m_detailAppearActive = true;
	m_detailAppearTimer = 0.0f;
	m_inputLocked = true;
	UpdateStarIconsByStageId(std::string());



	for (auto id : m_detailUIEntities)
	{
		if (!m_coordinator->HasComponent<TransformComponent>(id)) continue;

		auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
		auto itS = m_detailBaseScale.find(id);
		auto itP = m_detailBasePos.find(id);
		if (itS == m_detailBaseScale.end() || itP == m_detailBasePos.end()) continue;

		const float k0 = 0.90f;
		tr.scale = DirectX::XMFLOAT3(itS->second.x * k0, itS->second.y * k0, itS->second.z);
		tr.position = DirectX::XMFLOAT3(itP->second.x, itP->second.y + 12.0f, itP->second.z);
	}
}

void StageSelectScene::UpdateDetailAppear(float dt)
{
	if (!m_detailAppearActive) return;

	if (!m_isDetailMode)
	{
		m_detailAppearActive = false;
		m_detailAppearTimer = 0.0f;
		return;
	}

	m_detailAppearTimer += dt;

	float t = (DETAIL_APPEAR_DURATION <= 0.0f) ? 1.0f : (m_detailAppearTimer / DETAIL_APPEAR_DURATION);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	const float s = t * t * (3.0f - 2.0f * t);

	const float k0 = 0.90f;
	const float k = k0 + (1.0f - k0) * s;
	const float yOff = (1.0f - s) * 12.0f;

	for (auto id : m_detailUIEntities)
	{
		if (!m_coordinator->HasComponent<TransformComponent>(id)) continue;

		auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
		auto itS = m_detailBaseScale.find(id);
		auto itP = m_detailBasePos.find(id);
		if (itS == m_detailBaseScale.end() || itP == m_detailBasePos.end()) continue;

		tr.scale = DirectX::XMFLOAT3(itS->second.x * k, itS->second.y * k, itS->second.z);
		tr.position = DirectX::XMFLOAT3(itP->second.x, itP->second.y + yOff, itP->second.z);
	}

	if (t >= 1.0f)
	{
		for (auto id : m_detailUIEntities)
		{
			if (!m_coordinator->HasComponent<TransformComponent>(id)) continue;

			auto itS = m_detailBaseScale.find(id);
			auto itP = m_detailBasePos.find(id);
			if (itS == m_detailBaseScale.end() || itP == m_detailBasePos.end()) continue;

			auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
			tr.scale = itS->second;
			tr.position = itP->second;
		}

		if (m_starRevealPending && m_isDetailMode && !m_starRevealStageId.empty())
		{
			UpdateStarIconsByStageId(m_starRevealStageId);
			m_starRevealPending = false;
		}


		m_detailAppearActive = false;
		m_detailAppearTimer = 0.0f;
		m_inputLocked = false;
	}
}

void StageSelectScene::BeginStageReveal(int stageNo)
{
	if (stageNo < 1 || stageNo > TOTAL_STAGES) return;
	if (!IsStageUnlocked(stageNo)) return;

	int targetPage = (stageNo - 1) / STAGES_PER_PAGE;

	if (targetPage != m_currentPage && targetPage >= 0 && targetPage < TOTAL_PAGES)
	{
		SwitchToPage(targetPage);
	}

	int globalIdx = stageNo - 1;
	if (globalIdx < 0 || globalIdx >= (int)m_listCardModelEntities.size())
		return;

	ECS::EntityID cardEntity = m_listCardModelEntities[globalIdx];
	if (cardEntity == (ECS::EntityID)-1)
		return;

	auto& rev = m_stageReveal[stageNo];
	if (rev.active) return;

	rev.active = true;
	rev.entity = cardEntity;
	rev.elapsed = 0.0f;
	rev.duration = 0.90f;

	DirectX::XMFLOAT3 slotCenter = GetListCardSlotCenterPos(stageNo);
	rev.startY = slotCenter.y - 100.0f;
	rev.endY = slotCenter.y;

	rev.startAlpha = 0.0f;
	rev.endAlpha = 1.0f;

	if (m_coordinator->HasComponent<TransformComponent>(cardEntity))
	{
		rev.baseScale = m_coordinator->GetComponent<TransformComponent>(cardEntity).scale;
	}

	if (m_coordinator->HasComponent<TransformComponent>(cardEntity))
	{
		auto& tr = m_coordinator->GetComponent<TransformComponent>(cardEntity);
		DirectX::XMFLOAT3 worldPos = UIToWorld(slotCenter.x, rev.startY, 4.8f);
		tr.position = worldPos;
	}

	if (m_coordinator->HasComponent<RenderComponent>(cardEntity))
	{
		auto& rc = m_coordinator->GetComponent<RenderComponent>(cardEntity);
		rc.type = MESH_MODEL;
		rc.color.w = rev.startAlpha;
	}

	SetUIVisible(cardEntity, true);

	int pageIndex = targetPage;
	int indexInPage = (stageNo - 1) % STAGES_PER_PAGE;

	if (pageIndex >= 0 && pageIndex < (int)m_pages.size())
	{
		if (indexInPage >= 0 && indexInPage < (int)m_pages[pageIndex].hitboxEntities.size())
		{
			SetListCardHitboxVisible(m_coordinator.get(), m_pages[pageIndex].hitboxEntities[indexInPage], true);
		}
	}

	SetStageNumberLabelVisible(stageNo, true);
}

void StageSelectScene::UpdateStageReveal(float dt)
{
	if (!m_coordinator) return;
	if (m_stageReveal.empty()) return;

	for (auto& kv : m_stageReveal)
	{
		StageRevealAnim& anim = kv.second;
		if (!anim.active) continue;
		if (anim.entity == (ECS::EntityID)-1) { anim.active = false; continue; }

		if (!m_coordinator->HasComponent<TransformComponent>(anim.entity) ||
			!m_coordinator->HasComponent<RenderComponent>(anim.entity))
		{
			anim.active = false;
			continue;
		}

		anim.elapsed += dt;
		float t = (anim.duration > 0.0f) ? (anim.elapsed / anim.duration) : 1.0f;
		if (t > 1.0f) t = 1.0f;
		const float e = SmoothStep01(t);

		auto& tr = m_coordinator->GetComponent<TransformComponent>(anim.entity);
		auto& rc = m_coordinator->GetComponent<RenderComponent>(anim.entity);

		tr.position.y = anim.startY + (anim.endY - anim.startY) * e;

		const float a = anim.startAlpha + (anim.endAlpha - anim.startAlpha) * e;
		rc.color.w = a;

		if (t >= 1.0f)
		{
			tr.position.y = anim.endY;
			if (m_coordinator->HasComponent<RenderComponent>(anim.entity))
				m_coordinator->GetComponent<RenderComponent>(anim.entity).color.w = anim.endAlpha;

			if (m_coordinator->HasComponent<UIButtonComponent>(anim.entity))
			{
				m_coordinator->GetComponent<UIButtonComponent>(anim.entity).isVisible = true;
			}

			if (m_coordinator->HasComponent<RenderComponent>(anim.entity))
			{
				m_coordinator->GetComponent<RenderComponent>(anim.entity).color.w = anim.endAlpha;
			}

			anim.active = false;
		}
	}
}

void StageSelectScene::ApplyListVisibility(bool listVisible)
{
	if (!m_coordinator) return;

	for (int i = 0; i < (int)m_listUIEntities.size(); ++i)
	{
		const int stageNo = (i < (int)m_listStageNos.size()) ? m_listStageNos[i] : (i + 1);
		ECS::EntityID id = m_listUIEntities[i];

		const bool unlocked = IsStageUnlocked(stageNo);
		const bool show = listVisible && unlocked;

		auto itReveal = m_stageReveal.find(stageNo);
		const bool revealActive = (itReveal != m_stageReveal.end() && itReveal->second.active);

		if (!show)
		{
			SetUIVisible(id, false);
			continue;
		}

		if (stageNo == m_scheduledRevealStage)
		{
			SetUIVisible(id, false);
			continue;
		}

		if (revealActive)
		{
			if (m_coordinator->HasComponent<UIImageComponent>(id))
				m_coordinator->GetComponent<UIImageComponent>(id).isVisible = true;
			if (m_coordinator->HasComponent<UIButtonComponent>(id))
				m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
		}
		else
		{
			SetUIVisible(id, true);
		}
	}
}


void StageSelectScene::ReflowUnlockedCardsLayout()
{
	if (!m_coordinator) return;

	const int kTotalStages = TOTAL_STAGES;

	if ((int)m_listCardModelEntities.size() < kTotalStages)
	{
		m_listCardModelEntities.resize(kTotalStages, (ECS::EntityID)-1);
	}

	constexpr float kListCardZ = 4.8f;

	constexpr float kListCardScale = 0.03f;

	for (int stageNo = 1; stageNo <= kTotalStages; ++stageNo)
	{
		const int idx = stageNo - 1;
		const bool unlocked = IsStageUnlocked(stageNo);

		const int pageOfStage = (stageNo - 1) / STAGES_PER_PAGE;
		const bool isCurrentPage = (pageOfStage == m_currentPage);

		const DirectX::XMFLOAT3 uiPos = GetListCardSlotCenterPos(stageNo);
		const DirectX::XMFLOAT3 wpos = UIToWorld(uiPos.x, uiPos.y, kListCardZ);

		ECS::EntityID cardModel = m_listCardModelEntities[idx];

		const bool needRecreate =
			(cardModel == (ECS::EntityID)-1) ||
			(!m_coordinator->HasComponent<TransformComponent>(cardModel)) ||
			(!m_coordinator->HasComponent<ModelComponent>(cardModel)) ||
			(!m_coordinator->HasComponent<RenderComponent>(cardModel));

		if (needRecreate)
		{
			const char* modelId = kSelectCardModel[idx];
			cardModel = m_coordinator->CreateEntity(
				TagComponent("list_card_model"),
				TransformComponent(wpos, { 0.0f, DirectX::XM_PI, 0.0f }, MakeSelectCardScale(kListCardScale)),
				RenderComponent(MESH_MODEL, { 1.0f, 1.0f, 1.0f, 1.0f }),
				ModelComponent(modelId, 5.0f, Model::None)
			);
			m_listCardModelEntities[idx] = cardModel;
		}
		else
		{
			auto& tr = m_coordinator->GetComponent<TransformComponent>(cardModel);
			tr.position = wpos;
			tr.rotation = DirectX::XMFLOAT3(0.0f, DirectX::XM_PI, 0.0f);
			tr.scale = MakeSelectCardScale(kListCardScale);
		}

		const bool showModel = (!m_isDetailMode) && unlocked && isCurrentPage;
		SetUIVisible(cardModel, showModel);

		if (idx < (int)m_listUIEntities.size())
		{
			const ECS::EntityID hit = m_listUIEntities[idx];
			if (hit != (ECS::EntityID)-1 && m_coordinator->HasComponent<TransformComponent>(hit))
			{
				auto& tr = m_coordinator->GetComponent<TransformComponent>(hit);
				tr.position.x = uiPos.x;
				tr.position.y = uiPos.y;
				tr.position.z = 5.0f;
				tr.scale = DirectX::XMFLOAT3(250.0f, 150.0f, 1.0f);
				tr.rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			}

			const bool showHitbox = (!m_isDetailMode) && unlocked && isCurrentPage;
			SetUIVisible(hit, showHitbox);
		}

		SetStageNumberLabelVisible(stageNo, (!m_isDetailMode) && unlocked && isCurrentPage);
	}

	SyncStageNumberLabels(true);
}
void StageSelectScene::SetUIVisible(ECS::EntityID id, bool visible)
{
	if (!m_coordinator) return;

	const float targetAlpha = visible ? 1.0f : 0.0f;

	if (m_coordinator->HasComponent<UIImageComponent>(id))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
		ui.isVisible = visible;

		if (!visible)
		{
			ui.color.w = 0.0f;
		}
		else
		{
			if (ui.color.w <= 0.001f) ui.color.w = targetAlpha;
		}
	}

	if (m_coordinator->HasComponent<UIButtonComponent>(id))
	{
		m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = visible;
	}


	if (m_coordinator->HasComponent<RenderComponent>(id))
	{
		auto& rc = m_coordinator->GetComponent<RenderComponent>(id);

		const bool isModelEntity =
			m_coordinator->HasComponent<ModelComponent>(id) &&
			!m_coordinator->HasComponent<UIImageComponent>(id);

		if (isModelEntity)
		{
			rc.type = visible ? MESH_MODEL : MESH_NONE;
		}

		if (!visible)
		{
			rc.color.w = 0.0f;
		}
		else
		{
			if (rc.color.w <= 0.001f) rc.color.w = targetAlpha;
		}
	}
}


static const char* StageMoji_GetAssetId(char c)
{
	static const char* kDigits[10] =
	{
		"UI_STAGEMOJI0",
		"UI_STAGEMOJI1",
		"UI_STAGEMOJI2",
		"UI_STAGEMOJI3",
		"UI_STAGEMOJI4",
		"UI_STAGEMOJI5",
		"UI_STAGEMOJI6",
		"UI_STAGEMOJI7",
		"UI_STAGEMOJI8",
		"UI_STAGEMOJI9",
	};

	if (c >= '0' && c <= '9') return kDigits[(int)(c - '0')];
	if (c == '-') return "UI_STAGEMOJI_";
	return nullptr;
}

static constexpr float kStageNoScale = 1.75f;
static constexpr float kStageNoMarginBelow = 5.0f;
static constexpr float kStageNoDepth = 5.0f;
static constexpr float kStageNoBaseZ = 0.10f;

static constexpr float kStageNoBgWidth = 180.0f;
static constexpr float kStageNoBgHeight = 75.0f;
static constexpr float kStageNoBgDepth = 4.9f;
static constexpr float kStageNoBgOffsetX = 10.0f;
static constexpr float kStageNoBgOffsetY = -35.0f;


struct StageNoOffset
{
	float textOffsetX;
	float textOffsetY;
	float bgOffsetX;
	float bgOffsetY;
};

static const StageNoOffset kStageNoOffsets[18] =
{
	{ 100.0f,  10.0f,  -90.0f,  -10.0f },
	{ 20.0f,   10.0f,   3.0f,  -10.0f },
	{ 110.0f,  10.0f,   92.0f,  -10.0f },
	{ -85.0f,  75.0f,   -90.0f,  55.0f },
	{ 20.0f,   75.0f,   3.0f,  55.0f },
	{ 110.0f,  75.0f,   92.0f,  55.0f },

	{ 100.0f,  10.0f,  -90.0f,  -10.0f },
	{ 20.0f,   10.0f,   3.0f,  -10.0f },
	{ 110.0f,  10.0f,   92.0f,  -10.0f },
	{ -85.0f,  75.0f,   -90.0f,  55.0f },
	{ 20.0f,   75.0f,   3.0f,  55.0f },
	{ 110.0f,  75.0f,   92.0f,  55.0f },

	{ 100.0f,  10.0f,  -90.0f,  -10.0f },
	{ 20.0f,   10.0f,   3.0f,  -10.0f },
	{ 110.0f,  10.0f,   92.0f,  -10.0f },
	{ -85.0f,  75.0f,   -90.0f,  55.0f },
	{ 20.0f,   75.0f,   3.0f,  55.0f },
	{ 110.0f,  75.0f,   92.0f,  55.0f },
};
struct StageNoCharOffset2
{
	float x;
	float y;
};

static const StageNoCharOffset2 kStageNoCharOffsets[18][3] =
{
	{ { -90.0f, -17.0f }, { -220.0f, -10.0f }, { -190.0f, -17.0f } },
	{ { 80.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	{ { 80.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },
	{ { 75.0f, -17.0f }, { -40.0f, -10.0f }, { -10.0f, -17.0f } },
	{ { 65.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	{ { 65.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },

	{ { -120.0f, -17.0f }, { -220.0f, -10.0f }, { -190.0f, -17.0f } },
	{ { 50.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	{ { 50.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },
	{ { 45.0f, -17.0f }, { -40.0f, -10.0f }, { -10.0f, -17.0f } },
	{ { 35.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	{ { 35.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },

	{ { -150.0f, -17.0f }, { -220.0f, -10.0f }, { -190.0f, -17.0f } },
	{ { 20.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	{ { 20.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },
	{ { 95.0f, -77.0f }, { -40.0f, -10.0f }, { -10.0f, -17.0f } },
	{ { 85.0f, -77.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	{ { 85.0f, -77.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },
};
static constexpr float kStageNoGlyphWBase = 80.0f;
static constexpr float kStageNoGlyphHBase = 75.0f;
static constexpr float kStageNoGlyphAdvanceBase = 52.0f;

static constexpr float kStageNoHyphenYOffsetBase = -5.0f;
static constexpr float kStageNoHyphenXOffsetBase = 0.0f;

static inline void StageNo_GetGlyphMetrics(char c, float& outW, float& outH, float& outAdv)
{
	(void)c;
	outW = kStageNoGlyphWBase * kStageNoScale;
	outH = kStageNoGlyphHBase * kStageNoScale;
	outAdv = kStageNoGlyphAdvanceBase * kStageNoScale;
}

static constexpr bool  kStageNoBoldEnabled = true;
static constexpr int   kStageNoBoldPasses = 9;
static constexpr float kStageNoBoldOffsetPxBase = 1.0f;

void StageSelectScene::BuildStageNumberLabels()
{
	if (!m_coordinator) return;

	for (auto& v : m_listStageLabelEntities)
	{
		for (auto id : v)
		{
			if (id != (ECS::EntityID)-1) m_coordinator->DestroyEntity(id);
		}
	}
	for (auto& v : m_listStageLabelBoldEntities)
	{
		for (auto id : v)
		{
			if (id != (ECS::EntityID)-1) m_coordinator->DestroyEntity(id);
		}
	}

	for (auto id : m_listStageLabelBgEntities)
	{
		if (id != (ECS::EntityID)-1) m_coordinator->DestroyEntity(id);
	}

	m_listStageLabelEntities.clear();
	m_listStageLabelBoldEntities.clear();
	m_listStageLabelEntities.resize(TOTAL_STAGES);
	m_listStageLabelBoldEntities.resize(TOTAL_STAGES);
	m_listStageLabelBgEntities.clear();
	m_listStageLabelBgEntities.resize(TOTAL_STAGES, (ECS::EntityID)-1);


	const float maxW = kStageNoGlyphWBase * kStageNoScale;
	const float maxH = kStageNoGlyphHBase * kStageNoScale;

	for (int stageNo = 1; stageNo <= TOTAL_STAGES; ++stageNo)
	{
		const std::string text = StageNoToLabelText(stageNo);

		const int idx = stageNo - 1;
		m_listStageLabelBgEntities[idx] = m_coordinator->CreateEntity(
			TransformComponent({ 0.0f, 0.0f, kStageNoBaseZ }, { 0,0,0 }, { kStageNoBgWidth, kStageNoBgHeight, 1.0f }),
			UIImageComponent("UI_STAGE_NUMBER", kStageNoBgDepth, true, { 1,1,1,1 })
		);


		auto& vec = m_listStageLabelEntities[idx];
		auto& vecBold = m_listStageLabelBoldEntities[idx];
		vec.clear();
		vecBold.clear();
		vec.reserve((int)text.size());
		if (kStageNoBoldEnabled && kStageNoBoldPasses >= 2)
		{
			vecBold.reserve((int)text.size() * (kStageNoBoldPasses - 1));
		}

		for (size_t i = 0; i < text.size(); ++i)
		{
			const char c = text[i];
			const char* assetId = StageMoji_GetAssetId(c);
			const std::string tex = assetId ? std::string(assetId) : std::string("UI_STAGEMOJI0");

			ECS::EntityID e = m_coordinator->CreateEntity(
				TransformComponent({ 0.0f, 0.0f, kStageNoBaseZ }, { 0,0,0 }, { maxW, maxH, 1.0f }),
				UIImageComponent(tex, kStageNoDepth, true, { 1,1,1,1 })
			);

			if (!assetId && m_coordinator->HasComponent<UIImageComponent>(e))
			{
				auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);
				ui.isVisible = false;
				ui.color.w = 0.0f;
			}

			vec.push_back(e);

			if (kStageNoBoldEnabled && kStageNoBoldPasses >= 2)
			{
				for (int p = 0; p < (kStageNoBoldPasses - 1); ++p)
				{
					const float passDepth = kStageNoDepth + 0.0001f * (float)(p + 1);
					ECS::EntityID eb = m_coordinator->CreateEntity(
						TransformComponent({ 0.0f, 0.0f, kStageNoBaseZ + 0.0001f * (float)(p + 1) }, { 0,0,0 }, { maxW, maxH, 1.0f }),
						UIImageComponent(tex, passDepth, true, { 1,1,1,1 })
					);

					if (!assetId && m_coordinator->HasComponent<UIImageComponent>(eb))
					{
						auto& ui = m_coordinator->GetComponent<UIImageComponent>(eb);
						ui.isVisible = false;
						ui.color.w = 0.0f;
					}
					vecBold.push_back(eb);
				}
			}
		}

		SyncStageNumberLabel(stageNo);

		int pageIndex = (stageNo - 1) / STAGES_PER_PAGE;
		if (pageIndex >= 0 && pageIndex < TOTAL_PAGES)
		{
			m_pages[pageIndex].labelEntities.push_back(vec);
			m_pages[pageIndex].labelBoldEntities.push_back(vecBold);
			m_pages[pageIndex].labelBgEntities.push_back(m_listStageLabelBgEntities[idx]);
		}
	}
}

void StageSelectScene::SetStageNumberLabelVisible(int stageNo, bool visible)
{
	if (stageNo < 1 || stageNo >(int)m_listStageLabelEntities.size()) return;

	for (auto id : m_listStageLabelEntities[stageNo - 1]) SetUIVisible(id, visible);

	if (stageNo >= 1 && stageNo <= (int)m_listStageLabelBoldEntities.size())
	{
		for (auto id : m_listStageLabelBoldEntities[stageNo - 1]) SetUIVisible(id, visible);

		if (stageNo >= 1 && stageNo <= (int)m_listStageLabelBgEntities.size())
		{
			const int idx = stageNo - 1;
			if (m_listStageLabelBgEntities[idx] != (ECS::EntityID)-1)
			{
				SetUIVisible(m_listStageLabelBgEntities[idx], visible);
			}
		}
	}
}

void StageSelectScene::SyncStageNumberLabels(bool force)
{
	if (!force && (m_isDetailMode || m_isWaitingForTransition))
	{
		return;
	}

	if (force)
	{
		for (int stageNo = 1; stageNo <= TOTAL_STAGES; ++stageNo)
		{
			SyncStageNumberLabel(stageNo);
		}
		return;
	}

	int startStage = m_currentPage * STAGES_PER_PAGE + 1;
	int endStage = startStage + STAGES_PER_PAGE - 1;

	if (startStage < 1) startStage = 1;
	if (endStage > TOTAL_STAGES) endStage = TOTAL_STAGES;

	for (int stageNo = startStage; stageNo <= endStage; ++stageNo)
	{
		SyncStageNumberLabel(stageNo);
	}

	for (auto& kv : m_stageReveal)
	{
		if (kv.second.active)
		{
			if (kv.first < startStage || kv.first > endStage)
			{
				SyncStageNumberLabel(kv.first);
			}
		}
	}
}
void StageSelectScene::SyncStageNumberLabel(int stageNo)
{
	if (!m_coordinator) return;
	if (stageNo < 1 || stageNo > TOTAL_STAGES) return;

	const int idx = stageNo - 1;
	if (idx < 0 || idx >= (int)m_listUIEntities.size()) return;
	if (idx >= (int)m_listStageLabelEntities.size()) return;

	const ECS::EntityID cardId = m_listUIEntities[idx];
	if (cardId == (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(cardId, l, t, r, b)) return;

	bool cardVis = true;
	float cardAlpha = 1.0f;

	if (m_coordinator->HasComponent<UIButtonComponent>(cardId))
	{
		const auto& btn = m_coordinator->GetComponent<UIButtonComponent>(cardId);
		cardVis = btn.isVisible;
	}

	if (idx >= 0 && idx < (int)m_listCardModelEntities.size())
	{
		const ECS::EntityID modelId = m_listCardModelEntities[idx];
		if (modelId != (ECS::EntityID)-1 && m_coordinator->HasComponent<RenderComponent>(modelId))
		{
			const auto& rc = m_coordinator->GetComponent<RenderComponent>(modelId);
			cardAlpha = rc.color.w;
			if (rc.type == MESH_NONE || rc.color.w <= 0.001f)
			{
				cardVis = false;
			}
		}
	}

	if (m_coordinator->HasComponent<UIImageComponent>(cardId))
	{
		const auto& ui = m_coordinator->GetComponent<UIImageComponent>(cardId);
		cardVis = ui.isVisible;
		cardAlpha = ui.color.w;
	}
	else if (m_coordinator->HasComponent<RenderComponent>(cardId))
	{
		const auto& rc = m_coordinator->GetComponent<RenderComponent>(cardId);
		cardAlpha = rc.color.w;
		if (rc.type == MESH_NONE || rc.color.w <= 0.001f)
		{
			cardVis = false;
		}
	}

	const float digitH = kStageNoGlyphHBase * kStageNoScale;
	const float hyphenYOffset = kStageNoHyphenYOffsetBase * kStageNoScale;
	const float hyphenXOffset = kStageNoHyphenXOffsetBase * kStageNoScale;

	const float baseY = b + kStageNoMarginBelow;

	const std::string text = StageNoToLabelText(stageNo);
	auto& vec = m_listStageLabelEntities[idx];

	float totalW = 0.0f;
	for (size_t i = 0; i < text.size(); ++i)
	{
		float w = 0.0f, h = 0.0f, adv = 0.0f;
		StageNo_GetGlyphMetrics(text[i], w, h, adv);

		if (i == text.size() - 1)
		{
			totalW += w;
		}
		else
		{
			totalW += adv;
		}
	}

	const float centerX = (l + r) * 0.5f;
	const float baseX = centerX - totalW * 0.5f;

	int patternIdx = idx;

	const float stageTextOffsetX = (patternIdx >= 0 && patternIdx < 18) ? kStageNoOffsets[patternIdx].textOffsetX : 0.0f;
	const float stageTextOffsetY = (patternIdx >= 0 && patternIdx < 18) ? kStageNoOffsets[patternIdx].textOffsetY : 0.0f;
	const float stageBgOffsetX = (patternIdx >= 0 && patternIdx < 18) ? kStageNoOffsets[patternIdx].bgOffsetX : 0.0f;
	const float stageBgOffsetY = (patternIdx >= 0 && patternIdx < 18) ? kStageNoOffsets[patternIdx].bgOffsetY : 0.0f;

	const float boldOffset = kStageNoBoldOffsetPxBase;

	static const DirectX::XMFLOAT2 kOffsets[8] =
	{
		{ +1.0f,  0.0f },
		{ -1.0f,  0.0f },
		{  0.0f, +1.0f },
		{  0.0f, -1.0f },
		{ +1.0f, +1.0f },
		{ +1.0f, -1.0f },
		{ -1.0f, +1.0f },
		{ -1.0f, -1.0f },
	};

	float commonWaveOffset = 0.0f;
	float bgWaveOffset = 0.0f;
	float bgRoll = 0.0f;

	if (!m_isDetailMode && !m_isWaitingForTransition)
	{
		float phase = s_animTime * 2.0f + (float)idx * 0.5f;
		commonWaveOffset = std::sin(phase) * 3.0f;
		bgWaveOffset = std::sin(phase - 0.2f) * 3.0f;
		bgRoll = std::sin(phase * 0.7f) * 0.03f;
	}


	float cursor = 0.0f;
	for (int i = 0; i < (int)vec.size() && i < (int)text.size(); ++i)
	{
		const ECS::EntityID e = vec[i];
		if (e == (ECS::EntityID)-1) continue;

		const char c = text[(size_t)i];
		const bool isHyphen = (c == '-');

		float w = 0.0f, h = 0.0f, adv = 0.0f;
		StageNo_GetGlyphMetrics(c, w, h, adv);

		const float yOff = isHyphen ? hyphenYOffset : 0.0f;
		const float xOff = isHyphen ? hyphenXOffset : 0.0f;

		float perCharOffsetX = 0.0f;
		float perCharOffsetY = 0.0f;
		if (patternIdx >= 0 && patternIdx < 18 && i >= 0 && i < 3)
		{
			perCharOffsetX = kStageNoCharOffsets[patternIdx][i].x;
			perCharOffsetY = kStageNoCharOffsets[patternIdx][i].y;
		}

		float charWave = 0.0f;
		if (!m_isDetailMode && !m_isWaitingForTransition)
		{
			charWave = std::sin(s_animTime * 3.0f + (float)i * 0.8f + (float)idx) * 2.0f;
		}

		const float cx = baseX + cursor + w * 0.5f + xOff + stageTextOffsetX + perCharOffsetX;
		const float cy = (baseY + digitH * 0.5f) + yOff + stageTextOffsetY + perCharOffsetY + commonWaveOffset + charWave;

		if (m_coordinator->HasComponent<TransformComponent>(e))
		{
			auto& tr = m_coordinator->GetComponent<TransformComponent>(e);
			tr.position.x = cx;
			tr.position.y = cy;
			tr.position.z = kStageNoBaseZ;
			tr.scale = DirectX::XMFLOAT3(w, h, 1.0f);
			tr.rotation.z = bgRoll * 0.5f;
		}

		if (m_coordinator->HasComponent<UIImageComponent>(e))
		{
			auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);
			ui.isVisible = cardVis;
			ui.color.w = cardVis ? std::max(0.95f, cardAlpha) : 0.0f;
		}

		if (kStageNoBoldEnabled && kStageNoBoldPasses >= 2 && idx < (int)m_listStageLabelBoldEntities.size())
		{
			auto& vb = m_listStageLabelBoldEntities[idx];
			const int perChar = (kStageNoBoldPasses - 1);

			for (int p = 0; p < perChar; ++p)
			{
				const int bi = i * perChar + p;
				if (bi < 0 || bi >= (int)vb.size()) continue;

				const ECS::EntityID eb = vb[bi];
				if (eb == (ECS::EntityID)-1) continue;

				const int oi = std::min(p, 7);
				const float ox = kOffsets[oi].x * boldOffset;
				const float oy = kOffsets[oi].y * boldOffset;
				const float z = kStageNoBaseZ + 0.0001f * (float)(p + 1);

				if (m_coordinator->HasComponent<TransformComponent>(eb))
				{
					auto& trb = m_coordinator->GetComponent<TransformComponent>(eb);
					trb.position.x = cx + ox;
					trb.position.y = cy + oy;
					trb.position.z = z;
					trb.scale = DirectX::XMFLOAT3(w, h, 1.0f);
					trb.rotation.z = bgRoll * 0.5f;
				}

				if (m_coordinator->HasComponent<UIImageComponent>(eb))
				{
					auto& uib = m_coordinator->GetComponent<UIImageComponent>(eb);
					uib.isVisible = cardVis;
					uib.color.w = cardVis ? std::max(0.95f, cardAlpha) : 0.0f;
				}
			}
		}

		cursor += adv;

		if (idx >= 0 && idx < (int)m_listStageLabelBgEntities.size())
		{
			const ECS::EntityID bgId = m_listStageLabelBgEntities[idx];
			if (bgId != (ECS::EntityID)-1)
			{
				const float bgCenterX = centerX + stageBgOffsetX + kStageNoBgOffsetX;
				const float bgCenterY = (baseY + digitH * 0.5f) + stageBgOffsetY + kStageNoBgOffsetY + bgWaveOffset;


				if (m_coordinator->HasComponent<TransformComponent>(bgId))
				{
					auto& tr = m_coordinator->GetComponent<TransformComponent>(bgId);
					tr.position.x = bgCenterX;
					tr.position.y = bgCenterY;
					tr.position.z = kStageNoBaseZ - 0.01f;
					tr.scale = DirectX::XMFLOAT3(kStageNoBgWidth, kStageNoBgHeight, 1.0f);
					tr.rotation.z = bgRoll;
				}

				if (m_coordinator->HasComponent<UIImageComponent>(bgId))
				{
					auto& ui = m_coordinator->GetComponent<UIImageComponent>(bgId);
					ui.isVisible = cardVis;
					ui.color.w = cardVis ? std::max(0.95f, cardAlpha) : 0.0f;
				}
			}
		}
	}
}


void StageSelectScene::PlayUISelectEffect(ECS::EntityID uiEntity, const std::string& effectId, float scale)
{
	if (!m_coordinator) return;

	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;


	float l, t, r, b;
	if (!GetUIRect(uiEntity, l, t, r, b)) return;

	const float cx = (l + r) * 0.5f;
	const float cy = (t + b) * 0.5f;

	const float uiH = (b - t);

	float uiZ = 0.0f;
	if (m_coordinator->HasComponent<TransformComponent>(uiEntity))
	{
		uiZ = m_coordinator->GetComponent<TransformComponent>(uiEntity).position.z;
	}
	const float effectX = cx;
	const float effectY = cy - uiH * 0.30f;

	const float z = uiZ + 0.01f;

	const float finalScale = scale;

	ECS::EntityID fx = m_coordinator->CreateEntity(
		TagComponent("ui_select_fx"),
		TransformComponent({ effectX, effectY, z }, { 0,0,0 }, { 1,1,1 }),
		EffectComponent(effectId, false, true, { 0,0,0 }, finalScale)
	);

	m_uiSelectFx.push_back({ fx, 1.0f });
}

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

void StageSelectScene::UpdateEyeLight(float dt)
{
	for (int i = (int)m_activeEyeLights.size() - 1; i >= 0; --i)
	{
		auto& pair = m_activeEyeLights[i];
		pair.second -= dt;

		if (pair.second <= 0.0f || pair.first == (ECS::EntityID)-1)
		{
			if (pair.first != (ECS::EntityID)-1)
			{
				m_coordinator->DestroyEntity(pair.first);
			}
			m_activeEyeLights.erase(m_activeEyeLights.begin() + i);
		}
	}

	if (m_isDetailMode) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;

	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;

	m_eyeLightTimer += dt;

	if (m_eyeLightTimer >= m_eyeLightNextInterval)
	{
		m_eyeLightTimer = 0.0f;
		std::uniform_real_distribution<float> dist(0.5f, 1.5f);
		m_eyeLightNextInterval = dist(m_rng);

		struct EyePoint { int stageNo; float x; float y; };
		std::vector<EyePoint> points;

		static constexpr DirectX::XMFLOAT2 kEyeStageOffset[18] =
		{
			{ -120.0f, -35.0f },
			{ -30.0f, -25.0f },
			{ 50.0f, -35.0f },
			{ -100.0f, 30.0f },
			{ -20.0f, 40.0f },
			{ 63.0f, 40.0f },
			{ -110.0f, -30.0f },
			{ -30.0f, -35.0f },
			{ 65.0f, -30.0f },
			{ -120.0f, 45.0f },
			{ -35.0f, 35.0f },
			{ 53.0f, 45.0f },
			{ -75.0f, -40.0f },
			{ 0.0f, -25.0f },
			{ 90.0f, -35.0f },
			{ -90.0f, 35.0f },
			{ -10.0f, 40.0f },
			{ 70.0f, 30.0f },
		};
		static constexpr float kEyeJitterX = 0.0f;
		static constexpr float kEyeJitterY = 0.0f;
		static constexpr float kEyeZ = 0.0f;
		static constexpr float kEyeScale = 5.0f;

		int startStage = m_currentPage * STAGES_PER_PAGE + 1;
		int endStage = startStage + STAGES_PER_PAGE - 1;

		for (int stageNo = startStage; stageNo <= endStage; ++stageNo)
		{
			if (stageNo > m_maxUnlockedStage) continue;
			if (stageNo > TOTAL_STAGES) break;

			DirectX::XMFLOAT3 pos = GetListCardSlotCenterPos(stageNo);
			points.push_back({ stageNo, pos.x, pos.y });
		}

		if (points.empty()) return;

		std::uniform_int_distribution<int> idxDist(0, (int)points.size() - 1);
		const int pick = idxDist(m_rng);

		const int stageNo = points[pick].stageNo;
		float cx = points[pick].x;
		float cy = points[pick].y;

		int stageIndex = stageNo - 1;
		if (stageIndex >= 0 && stageIndex < 18)
		{
			cx += kEyeStageOffset[stageIndex].x;
			cy += kEyeStageOffset[stageIndex].y;
		}

		if (kEyeJitterX != 0.0f || kEyeJitterY != 0.0f)
		{
			std::uniform_real_distribution<float> jx(-kEyeJitterX, kEyeJitterX);
			std::uniform_real_distribution<float> jy(-kEyeJitterY, kEyeJitterY);
			cx += jx(m_rng);
			cy += jy(m_rng);
		}

		ECS::EntityID fxEntity = m_coordinator->CreateEntity(
			TagComponent("effect_eyeslight"),
			TransformComponent({ cx, cy, kEyeZ }, { 0,0,0 }, { 1,1,1 }),
			EffectComponent(
				"EFK_EYESLIGHT",
				false,
				true,
				{ 0,0,0 },
				kEyeScale
			)
		);
		m_activeEyeLights.push_back({ fxEntity, 2.0f });
	}
}
void StageSelectScene::EnsureDebugEffectOnMap()
{
	if (!m_debugShowGlowOnMap) return;
	if (!m_coordinator) return;
	if (!m_isDetailMode) return;

	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;
	if (m_stageMapEntity == (ECS::EntityID)-1) return;
	if (m_debugStarEntity != (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(m_stageMapEntity, l, t, r, b)) return;

	const float cx = (l + r) * 0.5f;
	const float cy = (t + b) * 0.5f;

	m_debugStarEntity = m_coordinator->CreateEntity(
		TagComponent("effect_debug"),
		TransformComponent({ cx, cy, 9.5f }, { 0,0,0 }, { 1,1,1 }),
		EffectComponent(
			"EFK_TREASURE_GLOW",
			true,
			true,
			{ 0,0,0 },
			0.3f
		)
	);

	std::cout << "[StageSelect] Debug glow created on map center\n";
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

	float l, t, r, b;
	if (!m_isDetailMode || !GetUIRect(m_stageMapEntity, l, t, r, b))
	{
		KillAllShootingStars();
		return;
	}

	const float PAD = 24.0f;
	const float left = l + PAD;
	const float right = r - PAD;
	const float top = t + PAD;
	const float bottom = b - PAD;

	const float KILL_MARGIN = 2.0f;

	const float killLeft = left + KILL_MARGIN;
	const float killRight = right - KILL_MARGIN;
	const float killTop = top + KILL_MARGIN;
	const float killBottom = bottom - KILL_MARGIN;

	const float FADE_BAND = 90.0f;
	const float FADE_START_X = killLeft + FADE_BAND;

	for (int i = (int)m_activeShootingStars.size() - 1; i >= 0; --i)
	{
		auto& s = m_activeShootingStars[i];

		s.remaining -= dt;

		if (s.star == (ECS::EntityID)-1 || !m_coordinator->HasComponent<TransformComponent>(s.star))
		{
			if (s.star != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.star);
			for (int k = 0; k < 3; ++k)
				if (s.trails[k] != (ECS::EntityID)-1) m_coordinator->DestroyEntity(s.trails[k]);

			m_activeShootingStars.erase(m_activeShootingStars.begin() + i);
			continue;
		}

		auto& starTr = m_coordinator->GetComponent<TransformComponent>(s.star);

		starTr.position.x += s.velocity.x * dt;
		starTr.position.y += s.velocity.y * dt;

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

		float xFadeK = 1.0f;
		if (starTr.position.x <= FADE_START_X)
		{
			const float u = (starTr.position.x - killLeft) / std::max(0.0001f, FADE_BAND);
			xFadeK = Smooth01_Local(Clamp01_Local(u));
		}

		const float life = (s.life > 0.0001f) ? s.life : 0.0001f;
		const float elapsed = (life - s.remaining);
		const float t01 = Clamp01_Local(elapsed / life);

		const float headK = (1.0f + (0.10f - 1.0f) * (t01 * t01)) * xFadeK;
		const float trailK = (1.0f + (0.25f - 1.0f) * (t01)) * xFadeK;

		starTr.scale = { headK, headK, headK };

		const float vx = s.velocity.x;
		const float vy = s.velocity.y;
		const float len = std::sqrt(vx * vx + vy * vy);
		if (len <= 0.0001f) continue;

		const float nx = vx / len;
		const float ny = vy / len;

		const float dir = std::atan2(vy, vx);
		const float tail = dir + DirectX::XM_PI;
		starTr.rotation.z = tail;

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

			const float kMul = 1.0f - 0.15f * (float)k;
			tr.scale = { trailK * kMul, trailK * kMul, trailK * kMul };
		}
	}
}


void StageSelectScene::SpawnShootingStar()
{
	if (!m_coordinator) return;
	if (!m_isDetailMode) return;

	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;
	if (m_stageMapEntity == (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(m_stageMapEntity, l, t, r, b)) return;

	const float PAD = 24.0f;
	const float left = l + PAD;
	const float right = r - PAD;
	const float top = t + PAD;
	const float bottom = b - PAD;
	if (right <= left || bottom <= top) return;

	const float KILL_LEFT = left + 2.0f;

	const float START_X = right - 2.0f;

	std::uniform_real_distribution<float> lifeDist(0.55f, 0.75f);
	const float life = lifeDist(m_rng);

	std::uniform_real_distribution<float> slopeDist(0.28f, 0.42f);
	const float slope = slopeDist(m_rng);

	const float dx = (START_X - KILL_LEFT);
	const float vx = -dx / life;
	const float vy = std::abs(vx) * slope;

	const float Y_TOP_MARGIN = 60.0f;
	const float Y_BOTTOM_MARGIN = 40.0f;

	const float yMin = top + Y_TOP_MARGIN;
	const float yMax = (bottom - Y_BOTTOM_MARGIN) - (vy * life);
	if (yMax <= yMin) return;

	std::uniform_real_distribution<float> ydist(yMin, yMax);
	const float x = START_X;
	const float y = ydist(m_rng);
	const float z = 0.0f;

	const float starScale = 5.0f;
	const float trailScale = 8.0f;

	const float dir = std::atan2(vy, vx);
	const float tail = dir + DirectX::XM_PI;
	const DirectX::XMFLOAT3 tailRot = { 0.0f, 0.0f, tail };

	ECS::EntityID star = m_coordinator->CreateEntity(
		TagComponent("shooting_star_head"),
		TransformComponent({ x, y, z }, tailRot, { 1,1,1 }),
		EffectComponent("EFK_SHOOTINGSTAR", false, true, tailRot, starScale)
	);

	const float vlen = std::sqrt(vx * vx + vy * vy);
	const float nx = (vlen > 0.0001f) ? (vx / vlen) : 0.0f;
	const float ny = (vlen > 0.0001f) ? (vy / vlen) : 0.0f;

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

void StageSelectScene::CreateStageInfoUI(const std::string& stageID)
{
	std::cout << "[StageSelectScene] CreateStageInfoUI begin stageId=" << stageID << std::endl;
	ClearStageInfoUI();

	MapStageConfig config = MapConfigLoader::Load(stageID);
	const std::vector<std::string>& items = config.items;
	const size_t itemCount = items.size();

	if (itemCount == 0)
	{
		std::cout << "[StageSelectScene] CreateStageInfoUI: No items for stageId=" << stageID << std::endl;
		return;
	}

	const float baseDepth = 110000.0f;

	float caseSize = 200.0f;
	float iconSize = 85.0f;
	float spacing = 0.10f;

	if (itemCount >= 6)
	{
		caseSize = 140.0f;
		iconSize = 55.0f;
		spacing = 0.07f;
	}
	else if (itemCount >= 5)
	{
		caseSize = 170.0f;
		iconSize = 70.0f;
		spacing = 0.085f;
	}
	else if (itemCount >= 4)
	{
		caseSize = 170.0f;
		iconSize = 70.0f;
		spacing = 0.10f;
	}

	float totalWidth = (itemCount - 1) * spacing;
	float startX = 0.73f - totalWidth / 2.0f;

	if (itemCount == 1)
	{
		startX = 0.73f;
	}

	for (size_t i = 0; i < itemCount; ++i)
	{
		float xPos = startX + i * spacing;

		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent(
				{ SCREEN_WIDTH * xPos, SCREEN_HEIGHT * 0.25f, 0.0f },
				{ 0, 0, 0 },
				{ caseSize, caseSize, 1 }
			),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));

		std::string iconID = GetItemIconPath(items[i]);

		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent(
				{ SCREEN_WIDTH * xPos, SCREEN_HEIGHT * 0.21f, 0.0f },
				{ 0, 0, 0 },
				{ iconSize, iconSize, 1 }
			),
			UIImageComponent(iconID, baseDepth + 3.2f)
		));
	}

	for (auto id : m_stageSpecificEntities)
	{
		CacheDetailBaseTransform(id);
		m_detailUIEntities.push_back(id);
	}

	std::cout << "[StageSelectScene] CreateStageInfoUI created count=" << m_stageSpecificEntities.size()
		<< " items=" << itemCount << " ids=";
	for (auto id : m_stageSpecificEntities) { std::cout << id << " "; }
	std::cout << std::endl;
}


void StageSelectScene::ClearStageInfoUI()
{
	if (!m_coordinator)
	{
		m_stageSpecificEntities.clear();
		return;
	}

	std::cout << "[StageSelectScene] ClearStageInfoUI start count=" << m_stageSpecificEntities.size() << " ids=";
	for (auto id : m_stageSpecificEntities) { std::cout << id << " "; }
	std::cout << std::endl;

	for (auto id : m_stageSpecificEntities)
	{
		if (id != (ECS::EntityID)-1)
		{
			if (m_coordinator->HasComponent<UIImageComponent>(id))
			{
				auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
				ui.isVisible = false;
				ui.color.w = 0.0f;
			}
			if (m_coordinator->HasComponent<UIButtonComponent>(id))
			{
				m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
			}
			if (m_coordinator->HasComponent<RenderComponent>(id))
			{
				auto& rc = m_coordinator->GetComponent<RenderComponent>(id);
				rc.color.w = 0.0f;
				rc.type = MESH_NONE;
			}

			m_detailBaseScale.erase(id);
			m_detailBasePos.erase(id);

			if (m_coordinator->HasComponent<UIImageComponent>(id)) {
				m_coordinator->RemoveComponent<UIImageComponent>(id);
			}
			if (m_coordinator->HasComponent<UIButtonComponent>(id)) {
				m_coordinator->RemoveComponent<UIButtonComponent>(id);
			}
			if (m_coordinator->HasComponent<RenderComponent>(id)) {
				m_coordinator->RemoveComponent<RenderComponent>(id);
			}
			if (m_coordinator->HasComponent<ModelComponent>(id)) {
				m_coordinator->RemoveComponent<ModelComponent>(id);
			}
			if (m_coordinator->HasComponent<AnimationComponent>(id)) {
				m_coordinator->RemoveComponent<AnimationComponent>(id);
			}

			m_coordinator->DestroyEntity(id);
		}
	}

	if (!m_stageSpecificEntities.empty())
	{
		m_detailUIEntities.erase(
			std::remove_if(m_detailUIEntities.begin(), m_detailUIEntities.end(),
				[this](ECS::EntityID id) {
					return std::find(m_stageSpecificEntities.begin(),
						m_stageSpecificEntities.end(), id) != m_stageSpecificEntities.end();
				}),
			m_detailUIEntities.end()
		);
	}

	{
		std::vector<ECS::EntityID> detailPurgeIds;
		for (auto id : m_detailUIEntities)
		{
			if (id == (ECS::EntityID)-1) continue;
			if (!m_coordinator->HasComponent<UIImageComponent>(id)) continue;

			const auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
			if (!IsStageSpecificUIAsset(ui.assetID)) continue;

			detailPurgeIds.push_back(id);
		}

		if (!detailPurgeIds.empty())
		{
			std::cout << "[StageSelectScene] ClearStageInfoUI purge detailUI count=" << detailPurgeIds.size() << " ids=";
			for (auto id : detailPurgeIds) { std::cout << id << " "; }
			std::cout << std::endl;

			for (auto id : detailPurgeIds)
			{
				if (m_coordinator->HasComponent<UIImageComponent>(id))
				{
					auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
					ui.isVisible = false;
					ui.color.w = 0.0f;
				}
				if (m_coordinator->HasComponent<UIButtonComponent>(id))
				{
					m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
				}
				if (m_coordinator->HasComponent<RenderComponent>(id))
				{
					auto& rc = m_coordinator->GetComponent<RenderComponent>(id);
					rc.color.w = 0.0f;
					rc.type = MESH_NONE;
				}

				if (m_coordinator->HasComponent<UIImageComponent>(id)) {
					m_coordinator->RemoveComponent<UIImageComponent>(id);
				}
				if (m_coordinator->HasComponent<UIButtonComponent>(id)) {
					m_coordinator->RemoveComponent<UIButtonComponent>(id);
				}
				if (m_coordinator->HasComponent<RenderComponent>(id)) {
					m_coordinator->RemoveComponent<RenderComponent>(id);
				}
				if (m_coordinator->HasComponent<ModelComponent>(id)) {
					m_coordinator->RemoveComponent<ModelComponent>(id);
				}
				if (m_coordinator->HasComponent<AnimationComponent>(id)) {
					m_coordinator->RemoveComponent<AnimationComponent>(id);
				}

				m_detailBaseScale.erase(id);
				m_detailBasePos.erase(id);

				m_coordinator->DestroyEntity(id);
			}

			m_detailUIEntities.erase(
				std::remove_if(m_detailUIEntities.begin(), m_detailUIEntities.end(),
					[this, &detailPurgeIds](ECS::EntityID id) {
						return std::find(detailPurgeIds.begin(), detailPurgeIds.end(), id) != detailPurgeIds.end();
					}),
				m_detailUIEntities.end()
			);
		}
	}

	{
		std::vector<ECS::EntityID> purgeIds;
		for (auto id : m_coordinator->GetActiveEntities())
		{
			if (id == (ECS::EntityID)-1) continue;
			if (!m_coordinator->HasComponent<UIImageComponent>(id)) continue;

			const auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
			if (!IsStageSpecificUIAsset(ui.assetID)) continue;

			purgeIds.push_back(id);
		}

		if (!purgeIds.empty())
		{
			std::cout << "[StageSelectScene] ClearStageInfoUI purge asset-matched count=" << purgeIds.size() << " ids=";
			for (auto id : purgeIds) { std::cout << id << " "; }
			std::cout << std::endl;

			for (auto id : purgeIds)
			{
				if (m_coordinator->HasComponent<UIImageComponent>(id))
				{
					auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);
					ui.isVisible = false;
					ui.color.w = 0.0f;
				}
				if (m_coordinator->HasComponent<UIButtonComponent>(id))
				{
					m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
				}
				if (m_coordinator->HasComponent<RenderComponent>(id))
				{
					auto& rc = m_coordinator->GetComponent<RenderComponent>(id);
					rc.color.w = 0.0f;
					rc.type = MESH_NONE;
				}

				if (m_coordinator->HasComponent<UIImageComponent>(id)) {
					m_coordinator->RemoveComponent<UIImageComponent>(id);
				}
				if (m_coordinator->HasComponent<UIButtonComponent>(id)) {
					m_coordinator->RemoveComponent<UIButtonComponent>(id);
				}
				if (m_coordinator->HasComponent<RenderComponent>(id)) {
					m_coordinator->RemoveComponent<RenderComponent>(id);
				}
				if (m_coordinator->HasComponent<ModelComponent>(id)) {
					m_coordinator->RemoveComponent<ModelComponent>(id);
				}
				if (m_coordinator->HasComponent<AnimationComponent>(id)) {
					m_coordinator->RemoveComponent<AnimationComponent>(id);
				}

				m_detailBaseScale.erase(id);
				m_detailBasePos.erase(id);

				m_coordinator->DestroyEntity(id);
			}

			m_detailUIEntities.erase(
				std::remove_if(m_detailUIEntities.begin(), m_detailUIEntities.end(),
					[this, &purgeIds](ECS::EntityID id) {
						return std::find(purgeIds.begin(), purgeIds.end(), id) != purgeIds.end();
					}),
				m_detailUIEntities.end()
			);
		}
	}

	m_stageSpecificEntities.clear();

	std::cout << "[StageSelectScene] ClearStageInfoUI done" << std::endl;
}
void StageSelectScene::UpdatePageNavigation(float dt)
{
	if (m_inputLocked || m_isDetailMode) return;

	if (IsKeyTrigger(VK_LEFT) && m_currentPage > 0)
	{
		SwitchToPage(m_currentPage - 1);
	}

	if (IsKeyTrigger(VK_RIGHT) && m_currentPage < TOTAL_PAGES - 1)
	{
		SwitchToPage(m_currentPage + 1);
	}
}

void StageSelectScene::SwitchToPage(int pageIndex)
{
	if (pageIndex < 0 || pageIndex >= TOTAL_PAGES) return;
	if (pageIndex == m_currentPage) return;

	if (!m_isDetailMode)
	{
		ClearStageInfoUI();
	}

	auto effectSystem = ECSInitializer::GetSystem<EffectSystem>();
	for (auto& pair : m_activeEyeLights)
	{
		if (pair.first != (ECS::EntityID)-1)
		{
			if (effectSystem) effectSystem->StopEffectImmediate(pair.first);
			m_coordinator->DestroyEntity(pair.first);
		}
	}
	m_activeEyeLights.clear();

	m_eyeLightTimer = 0.0f;
	m_eyeLightNextInterval = 0.5f;

	HidePage(m_currentPage);

	m_currentPage = pageIndex;
	ShowPage(m_currentPage);

	UpdateNavigationButtons();
	UpdatePageIndicators();
}

void StageSelectScene::ShowPage(int pageIndex)
{
	if (pageIndex < 0 || pageIndex >= TOTAL_PAGES) return;

	const auto& page = m_pages[pageIndex];

	for (auto entity : page.cardModelEntities)
	{
		int cardIndex = -1;
		for (size_t i = 0; i < m_listCardModelEntities.size(); ++i)
		{
			if (m_listCardModelEntities[i] == entity)
			{
				cardIndex = (int)i;
				break;
			}
		}

		if (cardIndex != -1)
		{
			int stageNo = cardIndex + 1;
			if (IsStageUnlocked(stageNo))
			{
				SetUIVisible(entity, true);
			}
		}
	}

	for (size_t i = 0; i < page.hitboxEntities.size(); ++i)
	{
		int stageNo = pageIndex * STAGES_PER_PAGE + (int)i + 1;
		if (IsStageUnlocked(stageNo))
		{
			SetListCardHitboxVisible(m_coordinator.get(), page.hitboxEntities[i], true);
		}
	}

	for (int i = 0; i < STAGES_PER_PAGE; ++i)
	{
		int stageNo = pageIndex * STAGES_PER_PAGE + i + 1;
		if (stageNo <= TOTAL_STAGES && IsStageUnlocked(stageNo))
		{
			SetStageNumberLabelVisible(stageNo, true);
		}
	}
}

void StageSelectScene::HidePage(int pageIndex)
{
	if (pageIndex < 0 || pageIndex >= TOTAL_PAGES) return;

	const auto& page = m_pages[pageIndex];

	for (auto entity : page.cardModelEntities)
	{
		SetUIVisible(entity, false);
	}

	for (auto entity : page.hitboxEntities)
	{
		SetListCardHitboxVisible(m_coordinator.get(), entity, false);
	}

	for (int i = 0; i < STAGES_PER_PAGE; ++i)
	{
		int stageNo = pageIndex * STAGES_PER_PAGE + i + 1;
		if (stageNo <= TOTAL_STAGES)
		{
			SetStageNumberLabelVisible(stageNo, false);
		}
	}
}

void StageSelectScene::CreateNavigationButtons()
{
	m_prevPageBtnEntity = m_coordinator->CreateEntity(
		TagComponent("prev_page_btn"),
		TransformComponent({ 120.0f, SCREEN_HEIGHT * 0.5f, 4.9f }, { 0,0,0 }, { 100, 100, 1 }),
		UIButtonComponent(
			ButtonState::Normal,
			true,
			[this]() {
				if (m_inputLocked || m_isDetailMode) return;
				if (m_currentPage > 0) {
					EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_DECISION", 0.5f);
					SwitchToPage(m_currentPage - 1);
				}
			}
		),
		UIImageComponent("UI_SELECT_LEFT", 1.0f)
	);

	m_nextPageBtnEntity = m_coordinator->CreateEntity(
		TagComponent("next_page_btn"),
		TransformComponent({ SCREEN_WIDTH - 120.0f, SCREEN_HEIGHT * 0.5f, 4.9f }, { 0,0,0 }, { 100, 100, 1 }),
		UIButtonComponent(
			ButtonState::Normal,
			true,
			[this]() {
				if (m_inputLocked || m_isDetailMode) return;
				if (m_currentPage < TOTAL_PAGES - 1) {
					EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_DECISION", 0.5f);
					SwitchToPage(m_currentPage + 1);
				}
			}
		),
		UIImageComponent("UI_SELECT_RIGHT", 1.0f)
	);

	m_buttonBaseScale[m_prevPageBtnEntity] = { 100, 100, 1 };
	m_buttonBaseScale[m_nextPageBtnEntity] = { 100, 100, 1 };
}

void StageSelectScene::UpdateNavigationButtons()
{
	if (m_prevPageBtnEntity != (ECS::EntityID)-1)
	{
		bool canGoPrev = (m_currentPage > 0);
		SetUIVisible(m_prevPageBtnEntity, canGoPrev);
	}

	if (m_nextPageBtnEntity != (ECS::EntityID)-1)
	{
		bool canGoNext = (m_currentPage < TOTAL_PAGES - 1);
		SetUIVisible(m_nextPageBtnEntity, canGoNext);
	}
}

void StageSelectScene::CreatePageIndicators()
{
	float centerX = SCREEN_WIDTH * 0.5f;
	float y = SCREEN_HEIGHT - 50.0f;
	float spacing = 30.0f;
	float startX = centerX - (TOTAL_PAGES - 1) * spacing * 0.5f;

	for (int i = 0; i < TOTAL_PAGES; ++i)
	{
		float x = startX + i * spacing;

		m_pageIndicatorEntities[i] = m_coordinator->CreateEntity(
			TagComponent("page_indicator"),
			TransformComponent({ x, y, 4.9f }, { 0,0,0 }, { 20, 20, 1 }),
			UIImageComponent("UI_PAGE_DOT", 1.0f)
		);
	}
}

void StageSelectScene::UpdatePageIndicators()
{
	for (int i = 0; i < TOTAL_PAGES; ++i)
	{
		if (m_pageIndicatorEntities[i] == (ECS::EntityID)-1) continue;

		if (m_coordinator->HasComponent<UIImageComponent>(m_pageIndicatorEntities[i]))
		{
			auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_pageIndicatorEntities[i]);
			ui.color = (i == m_currentPage)
				? DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
				: DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 0.7f);
		}
	}
}

int StageSelectScene::GetStageIndexInCurrentPage(int stageNo) const
{
	int globalIndex = stageNo - 1;
	int pageOfStage = globalIndex / STAGES_PER_PAGE;

	if (pageOfStage != m_currentPage) return -1;

	return globalIndex % STAGES_PER_PAGE;
}

int StageSelectScene::GetAbsoluteStageIndex(int stageNo) const
{
	return stageNo - 1;
}