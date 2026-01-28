
#include "Scene/SceneManager.h"
#include "Scene/ResultScene.h"
#include "Scene/GameScene.h"

#include "Scene/StageSelectScene.h"

#include "Systems/Input.h"
#include "Scene/StageUnlockProgress.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Rendering/EffectSystem.h"

// 共通画面遷移（System + Component）
#include "ECS/Systems/Core/ScreenTransition.h"
#include "ECS/Components/Rendering/AnimationComponent.h"
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

ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

extern std::string GetStageItemIconPath(const std::string& itemID);

static float Clamp01(float x)
{
	if (x < 0.0f) return 0.0f;
	if (x > 1.0f) return 1.0f;
	return x;
}

// ============================================================
// M_SELECT1～6 の左右反転について
//
// ============================================================
// M_SELECT1～6 の左右反転について
//
// ★注意: 現在は反転を無効にしています
// SetCullMode関数をDirectX.h/cppに追加してから、反転を有効にしてください。
// 詳細は「SetCullMode関数追加ガイド.md」を参照
// ============================================================
static constexpr bool kMirrorSelectCardTexture = false;  // ★一時的にfalse

static DirectX::XMFLOAT3 MakeSelectCardScale(float s)
{
	// 反転を無効化（SetCullMode実装後にtrueに変更）
	return DirectX::XMFLOAT3(-s, s, s);
}

static DirectX::XMFLOAT3 UIToWorld(float sx, float sy, float zWorld)
{
	// 1ワールド単位 = 100px くらいから調整開始（必要なら後で詰める）
	constexpr float PIXELS_PER_UNIT = 100.0f;

	const float wx = (sx - SCREEN_WIDTH * 0.5f) / PIXELS_PER_UNIT;
	const float wy = -(sy - SCREEN_HEIGHT * 0.5f) / PIXELS_PER_UNIT; // Y反転（UI↓ / 3D↑）
	return DirectX::XMFLOAT3(wx, wy, zWorld);
}

// 一覧カードのクリック判定用（透明UI）を確実に「見えないまま」表示/非表示する
static void SetListCardHitboxVisible(ECS::Coordinator* coord, ECS::EntityID id, bool visible)
{
	if (!coord || id == (ECS::EntityID)-1) return;

	if (coord->HasComponent<UIImageComponent>(id))
	{
		auto& ui = coord->GetComponent<UIImageComponent>(id);
		ui.isVisible = visible;
		ui.color.w = 0.0f; // 常に透明
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


// ------------------------------------------------------------
// UI_FONT helpers (5x3 grid assumed: uvScale=(0.2,0.333))
// digits 0-9: idx 0..9 (row=idx/5, col=idx%5)
// symbols: '-'=10 ':'=11 '.'=12  (row=2, col=idx-10)
// ------------------------------------------------------------
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
	// isVisible / alpha は呼び出し側で同期する
}

// ST_001..ST_006 を想定して「1-1 / 1-2 / 1-3 / 2-1 ...」へ変換
static std::string StageNoToLabelText(int stageNo)
{
	const int world = (stageNo - 1) / 3 + 1;
	const int sub = (stageNo - 1) % 3 + 1;
	return std::to_string(world) + "-" + std::to_string(sub);
}

/**
 * @brief ステージカード(1～6)の一覧スロット中心座標(UI座標)を返す
 * @param stageNo 1～6
 * @return UI座標（ピクセル）
 */
DirectX::XMFLOAT3 StageSelectScene::GetListCardSlotCenterPos(int stageNo) const
{
	const int n = std::max(1, std::min(6, m_maxUnlockedStage));
	if (stageNo < 1 || stageNo > n)
	{
		return DirectX::XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f);
	}

	// ★ここを好きに変更（UI座標）
	static const DirectX::XMFLOAT3 kPos[6] =
	{
		{ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.32f, 0.0f }, // 1
		{ SCREEN_WIDTH * 0.50f, SCREEN_HEIGHT * 0.32f, 0.0f }, // 2
		{ SCREEN_WIDTH * 0.75f, SCREEN_HEIGHT * 0.32f, 0.0f }, // 3
		{ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.62f, 0.0f }, // 4
		{ SCREEN_WIDTH * 0.50f, SCREEN_HEIGHT * 0.62f, 0.0f }, // 5
		{ SCREEN_WIDTH * 0.75f, SCREEN_HEIGHT * 0.62f, 0.0f }, // 6
	};

	return kPos[stageNo - 1];
}

//--------------------------------------------------------------
// Card Focus Animation（ボタン押下 → カードが中央に来る）
//--------------------------------------------------------------
void StageSelectScene::StartCardFocusAnim(ECS::EntityID cardEntity, const DirectX::XMFLOAT3& uiPos)
{
	constexpr float kCardZ = 5.0f;        // 少し手前
	// ★開始時に「縮小」させない（押した瞬間のチラつき/縮み対策）
	constexpr float kStartScale = 0.03f;
	// ★終了時の拡大率（好みで 1.10f〜1.35f くらい）
	constexpr float kEndScaleMul = 5.0f;
	constexpr float kDuration = 1.5f;

	constexpr float kExtraRollRad = DirectX::XMConvertToRadians(50.0f); // 追加で少しだけZ回転（演出）
	// 画面「中央」補正（+で右 / -で左、-で上 / +で下）
// 画面「中央」補正（+で右 / -で左、-で上 / +で下）
	const float kCenterOffsetPxX = SCREEN_WIDTH * 0.0f;
	const float kCenterOffsetPxY = -SCREEN_HEIGHT * 0.0f;



	m_cardFocus.active = true;
	m_cardFocus.entity = cardEntity;
	m_cardFocus.elapsed = 0.0f;
	m_cardFocus.duration = kDuration;

	// ★修正: カードの実際の表示位置を開始位置として使用
	// uiPos（クリック位置）ではなく、カードエンティティの現在のTransformから取得
	if (m_coordinator->HasComponent<TransformComponent>(cardEntity))
	{
		auto& transform = m_coordinator->GetComponent<TransformComponent>(cardEntity);
		// カードの現在位置をそのまま開始位置にする（UI座標→ワールド座標変換）
		m_cardFocus.startPos = UIToWorld(transform.position.x, transform.position.y, kCardZ);
	}
	else
	{
		// フォールバック: Transformがない場合は従来通り
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

	m_cardFocus.baseRot = DirectX::XMFLOAT3(0.0f, DirectX::XM_PI, 0.0f); // カード表向きの向きに合わせる
	m_cardFocus.extraRollRad = kExtraRollRad;

	{
		auto& tr = m_coordinator->GetComponent<TransformComponent>(cardEntity);
		// ★修正: 位置は既に正しい場所にあるので、スケールと回転だけ初期化
		// tr.position は上書きしない（元のカードの位置を保持）
		tr.scale = m_cardFocus.startScale;
		tr.rotation = m_cardFocus.baseRot;
	}
}

// StageSelectScene.cpp

void StageSelectScene::UpdateCardFocusAnim(float dt)
{
	if (!m_cardFocus.active) return;
	if (m_cardFocus.entity == (ECS::EntityID)-1) { m_cardFocus.active = false; return; }
	if (!m_coordinator->HasComponent<TransformComponent>(m_cardFocus.entity)) { m_cardFocus.active = false; return; }

	m_cardFocus.elapsed += dt;

	const float t = (m_cardFocus.duration > 0.0f) ? (m_cardFocus.elapsed / m_cardFocus.duration) : 1.0f;
	const float e = SmoothStep01(t);

	auto& tr = m_coordinator->GetComponent<TransformComponent>(m_cardFocus.entity);

	// 移動とスケールはシーン側で制御（Lerp）
	tr.position = Lerp3(m_cardFocus.startPos, m_cardFocus.endPos, e);
	tr.scale = Lerp3(m_cardFocus.startScale, m_cardFocus.endScale, e);

	// ★修正: 回転の上書きを削除しました！
	// AnimationSystem がアニメーションデータに基づいて回転(Delta加算)させてくれるので、
	// ここで tr.rotation = ... をすると回転がキャンセルされてしまいます。

	// もし「追加のZ回転(extraRollRad)」を入れたい場合は、
	// アニメーションの回転に「加算」する必要がありますが、
	// まずはアニメーション本来の回転（Y回転など）を確認するため、以下の行はコメントアウトまたは削除します。

	/* tr.rotation = m_cardFocus.baseRot;
	tr.rotation.z += m_cardFocus.extraRollRad * e;
	*/

	// 代わりに、どうしてもZ傾きだけ足したい場合は以下のようにDeltaで足す必要がありますが、
	// 一旦アニメーションを優先するため何もしません。

	if (t >= 1.0f)
	{
		m_cardFocus.active = false;
		tr.position = m_cardFocus.endPos;
		tr.scale = m_cardFocus.endScale;

		// 終了時も回転は強制セットしないほうが自然につながります
		// tr.rotation = m_cardFocus.baseRot; 
	}


}
// StageSelectScene.cpp

void StageSelectScene::DestroyFocusCard()
{
	if (!m_coordinator) { m_focusCardEntity = (ECS::EntityID)-1; return; }

	if (m_focusCardEntity != (ECS::EntityID)-1)
	{
		// ★追加: 念のためコンポーネントを明示的に削除してからEntityを消す
		if (m_coordinator->HasComponent<AnimationComponent>(m_focusCardEntity))
		{
			m_coordinator->RemoveComponent<AnimationComponent>(m_focusCardEntity);
		}

		m_coordinator->DestroyEntity(m_focusCardEntity);
		m_focusCardEntity = (ECS::EntityID)-1;
	}

	// 追従アニメも停止
	m_cardFocus.active = false;
	m_cardFocus.entity = (ECS::EntityID)-1;
}
void StageSelectScene::Init()
{
	m_coordinator = std::make_shared<Coordinator>();
	s_coordinator = m_coordinator.get();
	ECSInitializer::InitECS(m_coordinator);

	LoadStageData();

	// ===== Stage unlock progress =====
	StageUnlockProgress::Load();
	m_maxUnlockedStage = StageUnlockProgress::GetMaxUnlockedStage();
	m_pendingRevealStage = StageUnlockProgress::ConsumePendingRevealStage();

	// ★開始時は未選択扱いにして、BEST TIME は必ず "--:--" から開始する
	m_selectedStageID.clear();


	// カメラ
	EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0,0,0 });

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
	m_listStageNos.clear();
	m_listStageNos.reserve(6);

	m_listCardModelEntities.clear();
	m_listCardModelEntities.reserve(6);


	float startX = SCREEN_WIDTH * 0.2f;
	float startY = SCREEN_HEIGHT * 0.3f;
	float gapX = 350.0f;
	float gapY = 250.0f;

	m_listBgEntity = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 5.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_STAGE_SELECT", 0.0f)
	);

	for (int i = 0; i < 6; ++i)
	{
		std::string id = (i < (int)stageIDs.size()) ? stageIDs[i] : "ST_001";
		float x = startX + (i % 3) * gapX;
		float y = startY + (i / 3) * gapY;

		// --- Card visual (3D model): stage-specific (M_SELECT1..6) ---
		{
			std::string modelID = "M_SELECT" + std::to_string(i + 1);
			constexpr float kListCardZ = 4.8f;
			// ★一覧カードは「UI用カードモデル」を想定しているため、ワールドスケールを小さく固定。
			//   ここが大きいと、起動直後からカードが画面を覆う（今回の症状）。
			constexpr float kListCardScale = 0.03f;
			ECS::EntityID cardModel = m_coordinator->CreateEntity(
				TagComponent("list_card_model"),
				TransformComponent(UIToWorld(x, y, kListCardZ), { 0.0f, DirectX::XM_PI, 0.0f }, MakeSelectCardScale(kListCardScale)),
				RenderComponent(MESH_MODEL, { 1.0f, 1.0f, 1.0f, 1.0f }, CullMode::Front),
				ModelComponent(modelID, 5.0f, Model::None)
			);
			m_listCardModelEntities.push_back(cardModel);
		}

		// --- Hitbox (clickable area): keep it non-visual ---
		EntityID btn = m_coordinator->CreateEntity(
			TagComponent("list_card_hitbox"),
			TransformComponent({ x, y, 5.0f }, { 0,0,0 }, { 250, 150, 1 }),
			UIButtonComponent(
				ButtonState::Normal,
				true,
				[this, id, i]() {
					if (m_inputLocked || m_isWaitingForTransition) return;
					if (m_isDetailMode) return; // 詳細表示中は一覧からの再選択を無効化

					ResetSelectToDetailAnimState(false, true);

					m_selectedStageID = id;
					UpdateBestTimeDigitsByStageId(m_selectedStageID);

					UpdateStarIconsByStageId(std::string());
					m_starRevealPending = true;
					m_starRevealStageId = m_selectedStageID;

					m_inputLocked = true;

					if (i < m_listUIEntities.size())
					{
						// ======================================================
						// ★要求: カードを選択したら「他のカードが見えない」ようにする
						// - 一覧のヒットボックス/UI番号/3Dカードモデルを全て隠す
						// - 選択カードは focus_card として再生成して拡大表示
						// ======================================================
						for (int j = 0; j < (int)m_listUIEntities.size(); ++j)
						{
							SetUIVisible(m_listUIEntities[j], false);
							SetStageNumberLabelVisible(j + 1, false);
						}
						for (int j = 0; j < (int)m_listCardModelEntities.size(); ++j)
						{
							SetUIVisible(m_listCardModelEntities[j], false);
						}

						// 互換: 以前の「選択スロットだけ隠す」情報も更新しておく
						EntityID currentBtnID = m_listUIEntities[i];
						m_lastHiddenListCardEntity = currentBtnID;
						m_lastHiddenListStageNo = (int)i + 1;
						if (i < (int)m_listCardModelEntities.size())
						{
							m_lastHiddenListCardModelEntity = m_listCardModelEntities[i];
						}

						// 2) Create focus card using stage-specific model (M_SELECT1..6)
						DirectX::XMFLOAT3 uiPos = GetListCardSlotCenterPos((int)i + 1);
						ECS::EntityID oldFocus = m_focusCardEntity;

						std::string focusModelID = "M_SELECT" + std::to_string((int)i + 1);
						m_focusCardEntity = m_coordinator->CreateEntity(
							TagComponent("focus_card"),
							TransformComponent({ uiPos.x, uiPos.y, 5.0f }, { 0,0,0 }, { 1.0f, 1.0f, 1.0f }), // ★修正: 元のカード位置で作成
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

						// ★カード選択時に目のエフェクトを即座に消す
						auto effectSystem = ECSInitializer::GetSystem<EffectSystem>();

						// ============================================================
						// ★要求: ステージカードを選択した瞬間に「全てのエフェクト」を停止
						//  - 目の光(EFK_EYESLIGHT)
						//  - UI選択エフェクト(PlayUISelectEffect)
						//  - 詳細マップの流れ星(EFK_SHOOTINGSTAR*)
						//  - デバッグ常駐グロー(EFK_TREASURE_GLOW)
						// ============================================================

						// 1) 目の光
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

						// 2) UI選択ワンショット
						for (auto& fx : m_uiSelectFx)
						{
							if (fx.entity != (ECS::EntityID)-1)
							{
								if (effectSystem) effectSystem->StopEffectImmediate(fx.entity);
								m_coordinator->DestroyEntity(fx.entity);
							}
						}
						m_uiSelectFx.clear();

						// 3) 流れ星（もし残っていたら全消し）
						KillAllShootingStars();

						// 4) デバッグ常駐グロー
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
				}
			)
		);
		m_listUIEntities.push_back(btn);
		m_listStageNos.push_back(i + 1);

		// 初期表示：解放済みのみ表示。未解放は完全非表示。
		const int stageNo = i + 1;
		const bool unlocked = IsStageUnlocked(stageNo);
		if (!unlocked)
		{
			SetUIVisible(btn, false);
			if (i < (int)m_listCardModelEntities.size())
			{
				SetUIVisible(m_listCardModelEntities[i], false);
			}
		}

	}


	// ★一覧カードのステージ番号ラベル（UI_FONT）を生成
	BuildStageNumberLabels();
	SyncStageNumberLabels(true);

	// 解放済みステージの並びを「見えている数」に合わせて中央寄せ
	ReflowUnlockedCardsLayout();

	// 復帰時に「今回解放されたステージ」は“少し後から”浮かび上がり演出（既存解放カードは先に表示）
	if (m_pendingRevealStage >= 2 && m_pendingRevealStage <= 6 && IsStageUnlocked(m_pendingRevealStage))
	{
		const int idx = m_pendingRevealStage - 1;
		if (0 <= idx && idx < (int)m_listUIEntities.size())
		{
			// まずは隠しておき、少し待ってから Reveal を開始する
			SetUIVisible(m_listUIEntities[idx], false);
		}

		m_scheduledRevealStage = m_pendingRevealStage;
		m_revealDelayTimer = 0.0f;
		m_pendingRevealStage = -1; // 1回だけ
	}
	// =====================
	// Detail UI（情報/詳細）
	/*
// =====================
	// ★修正：depthを0.1fにして、モデル(カード)より手前に表示
	EntityID infoBg1 = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 20.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_INFO1", 0.1f)
	);
	m_detailUIEntities.push_back(infoBg1);

	EntityID infoBg2 = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 15.0 }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f }),
		UIImageComponent("BG_INFO2", 0.11f)
	);
	m_detailUIEntities.push_back(infoBg2);
*/
// UI_STAGE_MAP (BACK=10.0, SIRO=9.0)
	// Draw関数内で depth > 100000.0f のものは 3D描画後に重ねて描画されます。
	const float baseDepth = 110000.0f;
	//const float baseDepth = 20.0f;

	// UI_STAGE_MAP (BACK=10.0, SIRO=9.0)
	EntityID mapBack = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 10.0f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE_MAPBACK", baseDepth + 1.0f)
	);
	m_detailUIEntities.push_back(mapBack);
	m_stageMapEntity = mapBack;

	// ★ステージ固有マップ（UI_STAGE1～6）を、MAPBACK の上にオーバーレイ
	EntityID mapOverlay = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 9.5f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE1", baseDepth + 1.5f)
	);
	// 初期状態は未選択扱いなので非表示（押したカードに合わせて差し替える）
	if (m_coordinator->HasComponent<UIImageComponent>(mapOverlay))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(mapOverlay);
		ui.isVisible = false;
		ui.color.w = 0.0f;
	}
	m_detailUIEntities.push_back(mapOverlay);
	m_stageMapOverlayEntity = mapOverlay;

	// 城はそのまま前面
// フレーム
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.83f, 0.0f }, { 0,0,0 }, { 500, 160, 1 }),
		UIImageComponent("UI_FRAME", baseDepth + 3.0f) // 1.0f -> baseDepth + 3.0f
	));

	// BEST TIME
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.16f, SCREEN_HEIGHT * 0.83f, 0.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_BEST_TIME", baseDepth + 4.0f) // 2.0f -> baseDepth + 4.0f
	));

	// BEST TIME digits (mm:ss.d) : UI_BEST_TIME の右に UI_FONT で描画
	// - 開始時/未選択/未記録: "--:--" を表示し、".d" は非表示
	// - 記録あり: "mm:ss.d" を表示
	{
		m_bestTimeDigitEntities.clear();
		m_bestTimeDigitEntities.reserve(7);

		// 枠に収まるように少し小さめ
		const float digitW = 32.0f;
		const float digitH = 48.0f;
		const float stepX = 34.0f; // 文字間隔
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
			// 初期は "--:--"（".d" は非表示）
			int idx = 10; // '-'
			if (i == 2) idx = 11;      // ':'
			else if (i == 5) idx = 12; // '.'
			const int r = (idx <= 9) ? (idx / 5) : 2;
			const int c = (idx <= 9) ? (idx % 5) : (idx - 10);
			ui.uvPos = { c * 0.2f,  r * 0.333f };
			ui.uvScale = { 0.2f,     0.333f };

			// 末尾の .d は初期非表示
			if (i >= 5) ui.isVisible = false;

			m_bestTimeDigitEntities.push_back(d);
			m_detailUIEntities.push_back(d);
		}

		// ★開始時は未選択扱いなので必ず "--:--" に戻す
		UpdateBestTimeDigitsByStageId(std::string());
		UpdateStarIconsByStageId(std::string());
	}

	//トレジャー背景
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.2f, 0.0f }, { 0,0,0 }, { 550, 220, 1 }),
		UIImageComponent("UI_TRESURE_BACK", baseDepth + 3.0f) // 1.0f -> baseDepth + 3.0f
	));

	//// トレジャー枠
	//m_detailUIEntities.push_back(m_coordinator->CreateEntity(
	//	TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.2f, 0.0f }, { 0,0,0 }, { 550, 220, 1 }),
	//	UIImageComponent("UI_TRESURE", baseDepth + 3.0f) // 1.0f -> baseDepth + 3.0f
	//));

	// ★スター表示（保存値を表示：選択中ステージに応じてONを切り替える）
	{
		const char* conditionTex[3] = { "STAR_TEXT1","STAR_TEXT2","STAR_TEXT3" };

		float baseY = SCREEN_HEIGHT * 0.50f;
		float gapY = 55.0f;
		float starX = SCREEN_WIDTH * 0.6f;
		float captionX = SCREEN_WIDTH * 0.75f;

		for (int i = 0; i < 3; ++i)
		{
			float y = baseY + i * gapY;

			// 条件テキスト
			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 320.0f, 60.0f, 1.0f }),
				UIImageComponent(conditionTex[i], baseDepth + 5.0f, true, { 1,1,1,1 })
			));

			// ★サイズ（上だけ大きい）
			float offSize = (i == 0) ? 50.0f : 34.0f;
			float onSize = (i == 0) ? 50.0f : 34.0f;

			// ★ Off（枠）
			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { offSize, offSize, 1.0f }),
				UIImageComponent("ICO_STAR_OFF", baseDepth + 5.0f, true, { 0.0f,0.0f,0.0f,1.0f })
			));

			// ★ On（常に生成しておき、選択ステージで可視/不可視を切り替える）
			ECS::EntityID starOn = m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
				UIImageComponent("ICO_STAR_ON", baseDepth + 6.0f, true, { 1,1,1,1 })
			);
			if (m_coordinator->HasComponent<UIImageComponent>(starOn))
			{
				m_coordinator->GetComponent<UIImageComponent>(starOn).isVisible = false; // 初期はOFF
			}
			m_detailStarOnEntities[i] = starOn;
			m_detailUIEntities.push_back(starOn);
		}

		// ★開始時は未選択なので全部OFF
		UpdateStarIconsByStageId(std::string());
	}

	// STARTボタン
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_START_NORMAL", baseDepth + 5.0f) // 1.0f -> ...
	));

	EntityID startBtn = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("BTN_DECISION", baseDepth + 6.0f), // 2.0f -> ...
		UIButtonComponent(
			ButtonState::Normal,
			true,
			[this]() {
				if (m_inputLocked) return;

				PlayUISelectEffect(m_startBtnEntity, "EFK_SELECTOK", 35.0f);
				m_inputLocked = true;

				m_isWaitingForGameStart = true;
				m_gameStartTimer = 0.0f;

				ScreenTransition::RequestFadeOutEx(
					m_coordinator.get(), m_blackTransitionEntity, 0.15f, 0.35f, 0.45f,
					[this]() { GameScene::SetStageNo(m_selectedStageID); SceneManager::ChangeScene<GameScene>(); },
					false, nullptr, 0.0f, 0.35f, false, false
				);
			}
		)
	);
	m_startBtnEntity = startBtn;
	m_detailUIEntities.push_back(startBtn);

	// BACKボタン
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 200, 100, 1 }),
		UIImageComponent("UI_FINISH_NORMAL", baseDepth + 5.0f) // 1.0f -> ...
	));

	EntityID backBtn = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.86f, 5.0f }, { 0,0,0 }, { 160, 80, 1 }),
		UIImageComponent("BTN_REBERSE", baseDepth + 6.0f), // 2.0f -> ...
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



	// 詳細UIの基準Transformをキャッシュ（ふわっと出す用）
	for (auto id : m_detailUIEntities) { CacheDetailBaseTransform(id); }

	// カーソル
	m_cursorEntity = m_coordinator->CreateEntity(
		TransformComponent({ 0.0f, 0.0f, 0.0f }, { 0,0,0 }, { 64.0f, 64.0f, 1.0f }),
		UIImageComponent("ICO_CURSOR", 200000.0f),
		UICursorComponent()
	);

	// 初期：一覧
	SwitchState(false);

	// フェードオーバーレイ
	const float fadeX = SCREEN_WIDTH * 0.5f;
	const float fadeY = SCREEN_HEIGHT * 0.5f;

	// 1. 黒背景層（m_transitionEntity）
	const float fadeBgW = SCREEN_WIDTH * 2.0f;
	const float fadeBgH = SCREEN_HEIGHT * 2.0f;

	m_transitionEntity = ScreenTransition::CreateOverlay(
		m_coordinator.get(), "BG_STAGE_SELECT", fadeX, fadeY, fadeBgW, fadeBgH
	);

	if (m_coordinator->HasComponent<UIImageComponent>(m_transitionEntity))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_transitionEntity);
		ui.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 茶色ボード（元テクスチャ色）をそのまま使う
		ui.depth = 200000.0f;
	}



	// 2. ゲーム遷移専用：全面黒フェード（m_blackTransitionEntity）
	// ※既存の茶色ボードフェード（m_transitionEntity）はそのまま維持し、
	//   「決定 → GameScene」だけこちらを使う
	m_blackTransitionEntity = ScreenTransition::CreateOverlay(
		m_coordinator.get(), "BG_STAGE_SELECT", fadeX, fadeY, fadeBgW, fadeBgH
	);

	if (m_coordinator->HasComponent<UIImageComponent>(m_blackTransitionEntity))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_blackTransitionEntity);
		ui.color = { 0.0f, 0.0f, 0.0f, 0.0f }; // 初期は透明（遷移時にαを上げる）
		ui.depth = 200001.0f;                 // 茶色ボードより手前
	}
	// 起動時フェードイン
	m_inputLocked = true;
	ScreenTransition::RequestFadeInEx(
		m_coordinator.get(), m_transitionEntity, 1.45f,
		[this]() { m_inputLocked = false; }, 0.0f, false
	);
}


// --------------------------------------------------------------
// 一覧↔詳細の切替や、詳細へ入り直すタイミングで
// 「演出が前回の続きになる」ことを防ぐための完全リセット
// --------------------------------------------------------------
void StageSelectScene::ResetSelectToDetailAnimState(bool unlockInput, bool keepFocusCard)
{
	// 遷移待機の完全停止
	m_isWaitingForTransition = false;
	m_transitionWaitTimer = 0.0f;
	m_transitionDelayTime = 1.0f;
	m_pendingStageID.clear();
	// ★スター点灯の遅延予約もリセット（前回の持ち越し防止）
	m_starRevealPending = false;
	m_starRevealStageId.clear();



	// カード集中演出の完全停止
	m_cardFocus.active = false;
	m_cardFocus.elapsed = 0.0f;
	m_cardFocus.entity = (ECS::EntityID)-1;

	// 集中カードは原則残さない（毎回新品を作る）
	// ただし、直後に新規作成するケースでは
	// Destroy → Create で EntityID が再利用され、AnimationSystem 側のキャッシュが残ると
	// 「前回の続きから再生」になることがある。
	// keepFocusCard=true のときは、ここでは破棄せず呼び出し元で
	// 「新規作成後に旧カードを破棄」して EntityID 再利用を回避する。
	if (!keepFocusCard)
	{
		DestroyFocusCard();
	}


	// 集中演出で隠した一覧カード(ヒットボックス)を復帰
	if (m_lastHiddenListCardEntity != (ECS::EntityID)-1)
	{
		SetUIVisible(m_lastHiddenListCardEntity, true);
		m_lastHiddenListCardEntity = (ECS::EntityID)-1;
	}

	// ★3Dカードモデルとステージ番号も復帰（残像対策）
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

	// 詳細UIの出現アニメを停止
	m_detailAppearActive = false;
	m_detailAppearTimer = 0.0f;

	if (unlockInput)
	{
		m_inputLocked = false;
	}
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
	// ------------------------------------------------------------
	// deltaTime の単位ゆらぎ対策：
	// - 秒(s) が前提
	// - 16.6 など大きい値なら ms とみなして秒へ
	// - 1.0 付近なら「フレーム数(=1)」とみなして 60fps 想定で秒へ
	// ------------------------------------------------------------
	float dtSec = deltaTime;
	if (dtSec >= 10.0f)
	{
		// ms -> s
		dtSec *= 0.001f;
	}
	else if (dtSec >= 0.9f && dtSec <= 1.1f)
	{
		// frame(1.0) -> s (60fps想定)
		dtSec = 1.0f / 60.0f;
	}

	// ★保険：毎フレーム UI座標カメラを上書き（描画直前にも効くように）
	if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
	{
		effectSystem->SetScreenSpaceCamera((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
	}

	// ECSの各システム更新（AnimationSystem等がここで座標を動かす）
	m_coordinator->UpdateSystems(dtSec);

	UpdateShootingStar(dtSec);
	UpdateActiveShootingStars(dtSec);
	UpdateEyeLight(dtSec);
	UpdateUISelectFx(dtSec);
	UpdateDetailAppear(dtSec);
	UpdateButtonHoverScale(dtSec);

	// ★一覧カードの番号ラベルをカードの可視/アルファに追従させる
	SyncStageNumberLabels();

	// ★解放カードの“遅延スタート”（既存カードは先に表示し、新規だけ後から浮かせる）
// - リザルト → セレクト復帰時に「フェードイン」と同時に出ないよう、フェードが終わるまで待つ
// - フェード完了後（必要なら少しだけ間を置いて）新規カードだけ Reveal を開始する
	if (m_scheduledRevealStage != -1)
	{
		const bool isFading =
			ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
			ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity);

		if (!isFading)
		{
			// 調整：フェードイン完了後に何秒待ってから出すか（0.0fで即開始）
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
			// フェード中はカウントしない（フェード完了後に0から開始）
			m_revealDelayTimer = 0.0f;
		}
	}




	UpdateStageReveal(dtSec);
	UpdateCardFocusAnim(dtSec);

	// ★★★ カード選択後の遷移待ち処理 ★★★
	if (m_isWaitingForTransition)
	{
		m_transitionWaitTimer += dtSec;

		// 指定時間（m_transitionDelayTime）経過したら詳細画面へ
		if (m_transitionWaitTimer >= m_transitionDelayTime)
		{
			m_isWaitingForTransition = false;
			m_inputLocked = false; // ロック解除して操作可能に

			// 詳細画面へ切り替え
			SwitchState(true);
		}
	}

	if (m_isWaitingForGameStart)
	{
		m_gameStartTimer += dtSec;

		// 指定時間（GAME_START_DELAY）経過したらフェードアウト開始
		if (m_gameStartTimer >= 1.0f)
		{
			m_isWaitingForGameStart = false; // 多重発火防止

			// ここでフェードアウトをリクエスト
			ScreenTransition::RequestFadeOutEx(
				m_coordinator.get(), m_blackTransitionEntity, 0.15f, 0.35f, 0.45f,
				[this]() {
					GameScene::SetStageNo(m_selectedStageID);
					SceneManager::ChangeScene<GameScene>();
				},
				false, nullptr, 0.0f, 0.35f, false, false
			);
		}
	}
}// StageSelectScene.cpp


void StageSelectScene::Draw()
{
	//auto ui = ECSInitializer::GetSystem<UIRenderSystem>();
	//auto rs = ECSInitializer::GetSystem<RenderSystem>();
	//auto fx = ECSInitializer::GetSystem<EffectSystem>();

	//// UIレンダラが無い場合は従来通り
	//if (!ui)
	//{
	//	if (rs) { rs->DrawSetup(); rs->DrawEntities(); }
	//	if (fx) fx->Render();
	//	return;
	//}

	//// -----------------------------------------
	//// 目的：
	//// - 「カードアニメーション（3D）」をUIより前に出す
	//// - ただし画面遷移フェード（黒塗りオーバーレイ）は最前面に残す
	//// 実装：
	////   1) UI背景(depth<=0)
	////   2) 通常UI(0<depth<=kOverlayDepthStart)  ※この上に3Dを描く
	////   3) 3D
	////   4) オーバーレイUI(depth>kOverlayDepthStart) ※フェードなど
	//// -----------------------------------------
	//const float kOverlayDepthStart = 100000.0f; // フェード(=200000)は確実にここより上

	//struct VisState { ECS::EntityID id; bool uiVis; bool btnVis; };
	//std::vector<VisState> savedStates;

	//std::vector<ECS::EntityID> allTargets = m_listUIEntities;
	//allTargets.insert(allTargets.end(), m_detailUIEntities.begin(), m_detailUIEntities.end());
	//if (m_cursorEntity != (ECS::EntityID)-1) allTargets.push_back(m_cursorEntity);

	//// 背景
	//if (m_listBgEntity != (ECS::EntityID)-1) allTargets.push_back(m_listBgEntity);

	//// フェード（オーバーレイ）も対象に入れてパス制御する
	//if (m_transitionEntity != (ECS::EntityID)-1) allTargets.push_back(m_transitionEntity);
	//if (m_blackTransitionEntity != (ECS::EntityID)-1) allTargets.push_back(m_blackTransitionEntity);

	//for (auto id : allTargets)
	//{
	//	if (id == (ECS::EntityID)-1) continue;

	//	VisState s{ id,false,false };
	//	if (m_coordinator->HasComponent<UIImageComponent>(id))
	//		s.uiVis = m_coordinator->GetComponent<UIImageComponent>(id).isVisible;
	//	if (m_coordinator->HasComponent<UIButtonComponent>(id))
	//		s.btnVis = m_coordinator->GetComponent<UIButtonComponent>(id).isVisible;

	//	savedStates.push_back(s);
	//}

	//auto SetVisible = [&](ECS::EntityID id, bool v)
	//	{
	//		if (id == (ECS::EntityID)-1) return;
	//		if (m_coordinator->HasComponent<UIImageComponent>(id))
	//			m_coordinator->GetComponent<UIImageComponent>(id).isVisible = v;
	//		if (m_coordinator->HasComponent<UIButtonComponent>(id))
	//			m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = v;
	//	};

	//auto GetDepth = [&](ECS::EntityID id) -> float
	//	{
	//		if (!m_coordinator->HasComponent<UIImageComponent>(id)) return 0.0f;
	//		return m_coordinator->GetComponent<UIImageComponent>(id).depth;
	//	};

	//auto DrawUIOnce = [&]()
	//	{
	//		ui->Render(true);
	//		ui->Render(false);
	//	};

	//// 1) UI背景パス（depth <= 0）
	//for (const auto& s : savedStates)
	//{
	//	if (s.id == (ECS::EntityID)-1) continue;

	//	bool draw = false;
	//	if (m_coordinator->HasComponent<UIImageComponent>(s.id))
	//	{
	//		const float d = GetDepth(s.id);
	//		draw = (d <= 0.0f) && s.uiVis;
	//	}
	//	SetVisible(s.id, draw);
	//}
	//DrawUIOnce();

	//// 2) 通常UI（0 < depth <= kOverlayDepthStart）
	//for (const auto& s : savedStates)
	//{
	//	if (s.id == (ECS::EntityID)-1) continue;

	//	bool draw = false;
	//	if (m_coordinator->HasComponent<UIImageComponent>(s.id))
	//	{
	//		const float d = GetDepth(s.id);
	//		draw = (d > 0.0f && d <= kOverlayDepthStart) && s.uiVis;
	//	}
	//	if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
	//	{
	//		const float d = GetDepth(s.id);
	//		draw = draw || (s.btnVis && (d > 0.0f && d <= kOverlayDepthStart));
	//	}
	//	SetVisible(s.id, draw);
	//}
	//DrawUIOnce();

	//// 3) 3D（カードアニメーションをUIの上に）
	//if (rs)
	//{
	//	rs->DrawSetup();
	//	rs->DrawEntities();
	//}

	//if (fx)
	//{
	//	fx->Render();
	//}

	//// 4) オーバーレイUI（depth > kOverlayDepthStart）※フェードなど
	//for (const auto& s : savedStates)
	//{
	//	if (s.id == (ECS::EntityID)-1) continue;

	//	bool draw = false;
	//	if (m_coordinator->HasComponent<UIImageComponent>(s.id))
	//	{
	//		const float d = GetDepth(s.id);
	//		draw = (d > kOverlayDepthStart) && s.uiVis;
	//	}
	//	if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
	//	{
	//		const float d = GetDepth(s.id);
	//		draw = draw || (s.btnVis && (d > kOverlayDepthStart));
	//	}
	//	SetVisible(s.id, draw);
	//}
	//DrawUIOnce();

	//// 5) 復元
	//for (const auto& s : savedStates)
	//{
	//	if (s.id == (ECS::EntityID)-1) continue;
	//	if (m_coordinator->HasComponent<UIImageComponent>(s.id))
	//		m_coordinator->GetComponent<UIImageComponent>(s.id).isVisible = s.uiVis;
	//	if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
	//		m_coordinator->GetComponent<UIButtonComponent>(s.id).isVisible = s.btnVis;
	//}

	auto ui = ECSInitializer::GetSystem<UIRenderSystem>();
	auto rs = ECSInitializer::GetSystem<RenderSystem>();
	auto fx = ECSInitializer::GetSystem<EffectSystem>();

	// UIレンダラが無い場合は従来通り
	if (!ui)
	{
		if (rs) { rs->DrawSetup(); rs->DrawEntities(); }
		if (fx) fx->Render();
		return;
	}

	// -----------------------------------------
	// 描画順序の制御 (レイヤー分け)
	// Layer 1: 背景UI (depth <= 0)
	// Layer 2: 通常UI (0 < depth <= 100,000)
	// Layer 3: 3Dモデル (カードなど)
	// Layer 4: 詳細UI (100,000 < depth < 200,000) ★ここを作る
	// Layer 5: エフェクト (詳細UIの上、フェードの下) ★ここで描画
	// Layer 6: フェード (depth >= 200,000)
	// -----------------------------------------

	const float kNormalUIEnd = 100000.0f;
	const float kFadeStart = 200000.0f; // フェード開始ライン

	struct VisState { ECS::EntityID id; bool uiVis; bool btnVis; };
	std::vector<VisState> savedStates;

	std::vector<ECS::EntityID> allTargets = m_listUIEntities;

	// ★ステージ番号ラベルも描画制御対象に含める
	for (auto& v : m_listStageLabelEntities)
	{
		allTargets.insert(allTargets.end(), v.begin(), v.end());
	}

	allTargets.insert(allTargets.end(), m_detailUIEntities.begin(), m_detailUIEntities.end());
	if (m_cursorEntity != (ECS::EntityID)-1) allTargets.push_back(m_cursorEntity);
	if (m_listBgEntity != (ECS::EntityID)-1) allTargets.push_back(m_listBgEntity);
	if (m_transitionEntity != (ECS::EntityID)-1) allTargets.push_back(m_transitionEntity);
	if (m_blackTransitionEntity != (ECS::EntityID)-1) allTargets.push_back(m_blackTransitionEntity);

	// 現在の表示状態を保存
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

	// ------------------------------------------------------
	// 1) 背景UI (depth <= 0)
	// ------------------------------------------------------
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

	// ------------------------------------------------------
	// 2) 通常UI (0 < depth <= 100,000)
	// ------------------------------------------------------
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

	// ------------------------------------------------------
	// 3) 3Dモデル (カードアニメーションなど)
	// ------------------------------------------------------
	if (rs)
	{
		rs->DrawSetup();
		rs->DrawEntities();
	}

	// ------------------------------------------------------
	// 4) 詳細UIなど (100,000 < depth < 200,000)
	// ★ここが重要：エフェクトより先に描画して「下敷き」にする
	// ------------------------------------------------------
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

	// ------------------------------------------------------
	// 5) エフェクト描画
	// ★詳細UIの上、かつフェード(200,000)の下に描画される
	// ------------------------------------------------------
	if (fx)
	{
		fx->Render();
	}

	// ------------------------------------------------------
	// 6) フェードなど (depth >= 200,000)
	// ------------------------------------------------------
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

	// ------------------------------------------------------
	// 復元
	// ------------------------------------------------------
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

	// フェード中は出さない（黒に隠れる）
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;

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

		// 既に消えている/コンポーネントが無い
		if (!m_coordinator->HasComponent<EffectComponent>(fx.entity))
		{
			m_uiSelectFx.erase(m_uiSelectFx.begin() + i);
			continue;
		}

		auto& ec = m_coordinator->GetComponent<EffectComponent>(fx.entity);

		// 寿命を過ぎて、再生も終わっているなら破棄
		if (fx.remaining <= 0.0f && ec.handle == -1)
		{
			m_coordinator->DestroyEntity(fx.entity);
			m_uiSelectFx.erase(m_uiSelectFx.begin() + i);
		}
	}
}

// Hover中だけボタンを少し拡大（UIButtonSystem側の演出が無い/効かない場合の保険）
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
	// 追従速度（大きいほどキビキビ）
	const float lerpK = 14.0f;
	const float a = Clamp01(lerpK * dt);

	// 拡大量（好みで調整）
	// - ボタン(UI) : 少しだけ
	// - カード(3Dモデル) : 少しだけ（こちらが見た目の本体）
	const float hoverMulBtn = 1.10f;
	const float hoverMulModel = 1.10f;

	auto updateOne = [&](ECS::EntityID id)
		{
			if (id == (ECS::EntityID)-1) return;
			if (!m_coordinator->HasComponent<UIButtonComponent>(id)) return;

			auto& btn = m_coordinator->GetComponent<UIButtonComponent>(id);
			if (!btn.isVisible) return; // 表示中だけ

			if (!m_coordinator->HasComponent<TransformComponent>(id)) return;
			auto& tr = m_coordinator->GetComponent<TransformComponent>(id);

			// 初回だけ「基準サイズ」を記録
			auto it = m_buttonBaseScale.find(id);
			if (it == m_buttonBaseScale.end())
			{
				it = m_buttonBaseScale.emplace(id, tr.scale).first;
			}
			const DirectX::XMFLOAT3 base = it->second;

			// Hover判定は基準サイズで行う（拡大中に判定が暴れないように）
			const float left = tr.position.x - base.x * 0.5f;
			const float right = tr.position.x + base.x * 0.5f;
			const float top = tr.position.y - base.y * 0.5f;
			const float bottom = tr.position.y + base.y * 0.5f;

			const bool hovered = allowHover && (cx >= left && cx <= right && cy >= top && cy <= bottom);

			const float targetMul = hovered ? hoverMulBtn : 1.0f;
			const float curMul = (base.x != 0.0f) ? (tr.scale.x / base.x) : 1.0f;
			const float newMul = curMul + (targetMul - curMul) * a;

			tr.scale.x = base.x * newMul;
			tr.scale.y = base.y * newMul;
			tr.scale.z = base.z;
		};

	// 一覧/詳細の両方に対して適用（表示中かどうかは btn.isVisible で弾く）
	// 詳細表示中は「一覧ボタン」に Hover 演出を当てない（見た目の誤誘導を避ける）
	// ★一覧のステージカードは「見た目＝3Dモデル」なので、ヒットボックス自体は拡大しない
	//   （当たり判定ズレ/番号の位置ズレ防止）。
	if (!m_isDetailMode)
	{
		// 1) Hover判定（UI座標）→ 2) 対応する3Dカード(M_SELECT1..6)だけ拡大
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

			// ヒットボックスの基準サイズ（Hover判定に使用）
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

			// 3Dモデルの基準スケール（初回だけ記録）
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
	}
	for (auto id : m_detailUIEntities) updateOne(id);
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

	// ★★★ 演出状態の完全リセット（切替のたびに必ず同じ状態に戻す） ★★★
	// 詳細へ行くとき(toDetail=true)は、拡大したカード(focus_card)を消さずに残す
	ResetSelectToDetailAnimState(false, toDetail);

	// ▼▼▼▼▼▼▼▼▼▼ 修正箇所 1 ▼▼▼▼▼▼▼▼▼▼
	// 修正前: 背景は常に表示（詳細は一覧の上に重ねる）
	// SetUIVisible(m_listBgEntity, true);

	// 修正後: 詳細画面(=toDetail)なら非表示、一覧なら表示
	SetUIVisible(m_listBgEntity, !toDetail);
	// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

	// 一覧（カード）の制御
	float startX = SCREEN_WIDTH * 0.2f;	float startY = SCREEN_HEIGHT * 0.3f;
	float gapX = 350.0f;
	float gapY = 250.0f;

	for (int i = 0; i < (int)m_listUIEntities.size(); ++i)
	{
		EntityID id = m_listUIEntities[i];

		// ▼▼▼▼▼▼▼▼▼▼ 修正箇所 2 ▼▼▼▼▼▼▼▼▼▼
		// 修正前: 一覧UIは常に表示（詳細は一覧の上に重ねる）
		// SetUIVisible(id, true);

		// 修正後: 詳細画面(=toDetail)なら非表示、一覧なら表示
		const int stageNo = (i < (int)m_listStageNos.size()) ? m_listStageNos[i] : (i + 1);
		const bool showList = (!toDetail) && IsStageUnlocked(stageNo);
		auto itReveal = m_stageReveal.find(stageNo);
		const bool revealActive = (itReveal != m_stageReveal.end() && itReveal->second.active);

		if (!showList)
		{
			SetUIVisible(id, false);
		}
		else if (revealActive)
		{
			// 演出中は「画像だけ表示、クリック不可」
			if (m_coordinator->HasComponent<UIImageComponent>(id))
				m_coordinator->GetComponent<UIImageComponent>(id).isVisible = true;
			if (m_coordinator->HasComponent<UIButtonComponent>(id))
				m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
		}
		else
		{
			SetUIVisible(id, true);
		}
		// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

		// 3Dモデルは一旦なし（これをやらないと裏で浮いて見える）
		if (m_coordinator->HasComponent<RenderComponent>(id))
		{
			m_coordinator->GetComponent<RenderComponent>(id).type = MESH_NONE;
		}

		// 一覧に戻るとき：回転・スケールだけ復元（座標は ReflowUnlockedCardsLayout で決める）
		if (!toDetail)
		{
			// 演出中は座標を触らない
			auto itReveal2 = m_stageReveal.find(stageNo);
			const bool revealActive2 = (itReveal2 != m_stageReveal.end() && itReveal2->second.active);
			if (!revealActive2 && m_coordinator->HasComponent<TransformComponent>(id))
			{
				auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
				tr.position.z = 5.0f;
				tr.rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
				tr.scale = DirectX::XMFLOAT3(250.0f, 150.0f, 1.0f);
			}
		}
	}

	// ★一覧の3Dカードモデルも一覧/詳細に合わせて確実にON/OFF（残像対策）
	for (int stageNo = 1; stageNo <= 6; ++stageNo)
	{
		const int idx = stageNo - 1;
		if (idx < 0 || idx >= (int)m_listCardModelEntities.size()) continue;

		const bool unlocked = IsStageUnlocked(stageNo);
		const bool showModel = (!toDetail) && unlocked;
		SetUIVisible(m_listCardModelEntities[idx], showModel);
	}

	if (!toDetail)
	{
		ReflowUnlockedCardsLayout();
	}

	// ★一覧/詳細切替でラベルも即同期（残像対策）
	SyncStageNumberLabels(true);

	// 詳細の制御
	for (auto id : m_detailUIEntities)
	{
		SetUIVisible(id, toDetail);
	}

	// ★注意: 上の一括表示でスターONも一瞬 visible=true になる。
// ★要求: 「カードを押した瞬間」に星が出ないよう、詳細UIの出現アニメが終わったタイミングで点灯させる。
	if (toDetail)
	{
		// まずは全部OFF（SwitchStateの一括表示でtrueにされても、ここで必ず上書き）
		UpdateStarIconsByStageId(std::string());

		// 反映予約（UpdateDetailAppearの完了時に点灯）
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
	else
	{
		UpdateStarIconsByStageId(std::string());
		m_starRevealPending = false;
		m_starRevealStageId.clear();
	}


	// カーソル
	if (m_cursorEntity != (ECS::EntityID)-1)
	{
		if (m_coordinator->HasComponent<UIImageComponent>(m_cursorEntity))
		{
			m_coordinator->GetComponent<UIImageComponent>(m_cursorEntity).isVisible = true;
		}
	}

	// 演出関連
	if (toDetail)
	{// ============================================================
		// ★修正: エフェクトシステム経由で「即時停止」してから削除
		// ============================================================
		auto effectSystem = ECSInitializer::GetSystem<EffectSystem>();

		for (auto& pair : m_activeEyeLights)
		{
			if (pair.first != (ECS::EntityID)-1)
			{
				// 1. まずエフェクトを止める (これで画面から消えます)
				if (effectSystem)
				{
					effectSystem->StopEffectImmediate(pair.first);
				}

				// 2. その後、Entityを削除
				m_coordinator->DestroyEntity(pair.first);
			}
		}
		m_activeEyeLights.clear();
		// ============================================================

		if (!wasDetail)
		{
			// 選択ステージに応じて「城(手前)」のテクスチャを差し替える
			ApplyStageMapTextureByStageId(m_selectedStageID);
			CreateStageInfoUI(m_selectedStageID);
			BeginDetailAppear();
			m_shootingStarTimer = 0.0f;
			std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
			m_nextShootingStarWait = dist(m_rng);
			m_spawnStarOnEnterDetail = true;

			m_eyeLightTimer = 0.0f;
			m_eyeLightNextInterval = 1.0f; // 最初は1秒後に光る
		}
	}
	else
	{
		if (wasDetail)
		{
			m_spawnStarOnEnterDetail = false;
			KillAllShootingStars();
			ClearStageInfoUI();
			if (m_debugStarEntity != (ECS::EntityID)-1)
			{
				m_coordinator->DestroyEntity(m_debugStarEntity);
				m_debugStarEntity = (ECS::EntityID)-1;
			}
			// ★詳細ウインドウを閉じたら未選択に戻す（開始時と同じ "--:--"）
			m_selectedStageID.clear();
			UpdateBestTimeDigitsByStageId(std::string());
			UpdateStarIconsByStageId(std::string());

		}
	}
}


//--------------------------------------------------------------
// Stage Map Texture (per-stage)
//  - TextureList.csv の AssetID: Stage_1 .. Stage_6 を使用
//--------------------------------------------------------------
int StageSelectScene::StageIdToStageNo(const std::string& stageId) const
{
	// 期待: "ST_001" .. "ST_006"
	if (stageId.size() >= 6 && stageId.rfind("ST_", 0) == 0)
	{
		// "001" を読む
		try
		{
			int n = std::stoi(stageId.substr(3));
			return n;
		}
		catch (...) {}
	}

	// 互換: "Stage_1" などが来た場合
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

	// 未選択/不正は "--:--" に固定（GetBestTimeMsを呼ばない）
	uint32_t bestMs = 0;
	if (stageNo >= 1 && stageNo <= 6)
	{
		bestMs = StageUnlockProgress::GetBestTimeMs(stageNo);
	}

	int digits[7] = { 10,10,11,10,10,12,10 }; // --:--.-
	bool showDecimal = false;

	if (bestMs != 0)
	{
		const int t10 = static_cast<int>((bestMs + 50) / 100); // 0.1s
		const int mm = (t10 / 600) % 100;
		const int ss = (t10 / 10) % 60;
		const int ds = (t10 % 10);

		digits[0] = (mm / 10) % 10;
		digits[1] = (mm % 10);
		digits[2] = 11; // ':'
		digits[3] = (ss / 10) % 10;
		digits[4] = (ss % 10);
		digits[5] = 12; // '.'
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
	// 未選択/不正IDのときは全てOFF
	const std::uint8_t mask =
		(stageNo >= 1 && stageNo <= 6) ? StageUnlockProgress::GetStageStarMask(stageNo) : 0;

	for (int i = 0; i < 3; ++i)
	{
		ECS::EntityID e = m_detailStarOnEntities[i];
		if (e == (ECS::EntityID)-1) continue;
		if (!m_coordinator->HasComponent<UIImageComponent>(e)) continue;

		auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);
		const bool on = ((mask >> i) & 0x1) != 0;

		// SwitchState() の一括表示で true にされるので、ここで最終状態を必ず上書き。
		// さらに isVisible を見ない描画実装でも確実に消えるよう alpha も同期。
		ui.isVisible = on;
		ui.color.w = on ? 1.0f : 0.0f;
	}
}

static int ExtractStageNoFromStageId(const std::string& stageId)
{
	// 1～6の最初の数字を拾う（"1-1", "STAGE2", "stage_3-1" など対応）
	for (char c : stageId)
	{
		if (c >= '1' && c <= '6') return (c - '0');
	}
	return 0;
}


std::string StageSelectScene::GetStageMapTextureAssetId(int stageNo) const
{
	if (stageNo >= 1 && stageNo <= 6)
	{
		return kStageMapUI[stageNo - 1];
	}
	// フォールバック（未選択/不正）
	return kStageMapUI[0];
}


/**
 * @brief ステージ詳細のマップ表示(UI_STAGE1～UI_STAGE6)を差し替える。
 * @details UI_STAGE_MAPBACK の上に重ねるオーバーレイ(m_stageMapOverlayEntity)を作り直して更新する。
 */
void StageSelectScene::ApplyStageMapTextureByStageId(const std::string& stageId)
{
	// オーバーレイ未生成なら何もしない
	if (m_stageMapOverlayEntity == (ECS::EntityID)-1) return;

	// ----------------------------
	// stageId からステージ番号(1～6)を推定
	// ----------------------------
	int stageNo = ExtractStageNoFromStageId(stageId);

	// 例: "STAGE1-1", "1-2", "Stage3", "STAGE_4" みたいなのでも拾えるように
	for (char c : stageId)
	{
		if (c >= '1' && c <= '6')
		{
			stageNo = (c - '0');
			break;
		}
	}

	// 有効か判定
	const bool valid = (stageNo >= 1 && stageNo <= 6);

	// valid の時だけ UI_STAGE1～UI_STAGE6 を選ぶ
	const std::string texId = valid ? GetStageMapTextureAssetId(stageNo) : std::string("UI_STAGE1");

	// ----------------------------
	// 現在のオーバーレイのTransform/Depthを取り出す
	// ----------------------------
	const ECS::EntityID oldId = m_stageMapOverlayEntity;

	if (!m_coordinator->HasComponent<TransformComponent>(oldId) ||
		!m_coordinator->HasComponent<UIImageComponent>(oldId))
	{
		return;
	}

	const auto oldTr = m_coordinator->GetComponent<TransformComponent>(oldId);
	const auto oldUi = m_coordinator->GetComponent<UIImageComponent>(oldId);

	// ----------------------------
	// 新しいオーバーレイEntityを生成（同じ位置/サイズ/深度）
	// ----------------------------
	ECS::EntityID newId = m_coordinator->CreateEntity(
		TransformComponent(oldTr.position, oldTr.rotation, oldTr.scale),
		UIImageComponent(texId, oldUi.depth)
	);

	// 表示/非表示（未選択状態なら消す）
	if (m_coordinator->HasComponent<UIImageComponent>(newId))
	{
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(newId);
		ui.isVisible = valid;
		ui.color.w = valid ? 1.0f : 0.0f; // アルファ
	}

	// 管理リストに追加
	m_detailUIEntities.push_back(newId);

	// ----------------------------
	// 古いEntityを管理リストから外して破棄
	// ----------------------------
	for (auto it = m_detailUIEntities.begin(); it != m_detailUIEntities.end(); ++it)
	{
		if (*it == oldId)
		{
			m_detailUIEntities.erase(it);
			break;
		}
	}

	m_coordinator->DestroyEntity(oldId);

	// ID更新
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
	m_inputLocked = true; // 演出中は操作不可
	// ★詳細出現中は星を点灯させない（完了タイミングで反映）
	UpdateStarIconsByStageId(std::string());



	// 初期状態（少し小さく/少しだけ下から）
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

	// モードが変わったら即停止
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

	// Smoothstep
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
		// 最終値へスナップ & 操作復帰
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

		// ★ここが「詳細UIの出現アニメ完了」
// 予約していたスター点灯を、このタイミングで反映する
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



//--------------------------------------------------------------
// Stage Unlock / Reveal（未解放は完全非表示、解放時に浮かび上がる）
//--------------------------------------------------------------
/**
 * @brief ステージ決定後の詳細演出（リビール）を開始する
 * * @param stageNo 決定されたステージ番号
 * * 【修正内容】
 * 詳細画面の右側に大きく表示されるモデルも、M_CARD 固定から
 * M_SELECT1 ～ M_SELECT6 の固有モデルに切り替わるように修正しました。
 */
 /**
  * @brief ステージ決定後の詳細演出（リビール）を開始する
  * * @param stageNo 決定されたステージ番号
  * * 【修正内容】
  * 詳細画面の右側に大きく表示されるモデルも、M_SELECT1 ～ M_SELECT6 に切り替えました。
  */
void StageSelectScene::BeginStageReveal(int stageNo)
{
	// 1. 古い演出用エンティティが残っていれば削除
	if (m_revealingCardEntity != (ECS::EntityID)-1) {
		m_coordinator->DestroyEntity(m_revealingCardEntity);
		m_revealingCardEntity = (ECS::EntityID)-1;
	}

	// --- 【BKB注釈】ここでも固有モデルIDを使用 ---
	std::string modelID = "M_SELECT" + std::to_string(stageNo);

	// 2. 演出開始時の初期状態を設定
	//    画面の右奥の方から、小さく、透明に近い状態で出現させる準備
	DirectX::XMFLOAT3 startPos = { 400.0f, 0.0f, -5.0f };
	DirectX::XMFLOAT3 startRot = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 startScale = { 0.1f, 0.1f, 0.1f };

	// 3. 演出用エンティティの作成
	m_revealingCardEntity = m_coordinator->CreateEntity(
		TransformComponent(startPos, startRot, startScale),
		RenderComponent(MESH_MODEL, { 1.0f, 1.0f, 1.0f, 1.0f }, CullMode::Front),
		ModelComponent(modelID, 1.5f, Model::None) // 他のUIより少し手前に表示
	);

	// 4. アニメーション制御用データの初期化
	//    m_stageReveal 構造体に、経過時間や期間をセット
	auto& anim = m_stageReveal[stageNo];
	anim.active = true;    // ★重要: アニメーションを有効化
	anim.entity = m_revealingCardEntity; // ★重要: 動かす対象のIDをセット

	anim.elapsed = 0.0f;
	anim.duration = 0.90f; // 0.9秒かけて演出

	// その他アニメーションに必要なパラメータのリセット
	anim.startY = 0.0f;
	anim.endY = 0.0f;
	anim.startAlpha = 0.0f;
	anim.endAlpha = 1.0f;
	anim.baseScale = { 1.0f, 1.0f, 1.0f };

	// 状態を「リスト表示」から「詳細表示」へ切り替え（Update関数側で処理される）
	// m_isDetailVisible ではなく、このプロジェクトではフラグ管理などが別にあるため
	// ここではアニメーションデータ(anim)のセットアップに集中します
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

		// 位置：下→定位置
		tr.position.y = anim.startY + (anim.endY - anim.startY) * e;

		// アルファ：0→1
		const float a = anim.startAlpha + (anim.endAlpha - anim.startAlpha) * e;
		rc.color.w = a;

		// 仕上げ：最後にクリック可能化
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

// 一覧（カード）表示の統一制御（未解放は完全非表示）
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

		// 新規解放カードは、遅延時間が経つまで強制的に隠す（全部同時に出るのを防ぐ）
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


// 解放済みステージ（1..m_maxUnlockedStage）を「見えている数」で中央寄せ配置する
// - 3列×2段を基本にし、各段は「その段にある数」で中央寄せ
// - 演出中のステージは配置を上書きしない（浮かび上がりを維持）
// 解放済みステージ（1..m_maxUnlockedStage）を「6枚表示時のスロット座標」に固定配置する
// - 3列×2段（最大6枚）のスロットを常に同じ位置に置く
// - 解放枚数が1枚でも「中央寄せ」しない（= 6枚ある時と同じ場所に出る）
// - 演出中のステージは配置を上書きしない（浮かび上がりを維持）
/**
 * @brief 解放されているステージカードを生成し、レイアウトを再計算する
 * * 【修正内容】
 * 以前は全てのカードが M_CARD 固定でしたが、CSVの定義（M_SELECT1 ～ M_SELECT6）に
 * 基づいて、ステージ番号に応じたモデルを生成するように変更しました。
 */
 /**
  * @brief 解放されているステージカードを生成し、レイアウトを再計算する
  * * 【修正内容】
  * M_CARD 固定ではなく、CSVで定義された M_SELECT1 ～ M_SELECT6 を使用して
  * ステージごとのユニークなモデルを表示します。
  */
  /**
   * @brief 解放されているステージカードを生成し、レイアウトを再計算する
   * * 【修正内容】
   * ・M_SELECT1 ～ M_SELECT6 のモデルを使用するように変更
   * ・存在しない m_currentSelectedStage の代わりに、m_selectedStageID を数値変換して使用するように修正
   */
   /**
	* @brief 一覧カード(ステージ選択)の3Dモデル（M_SELECT1～M_SELECT6）を再配置する
	* @details
	* - 一覧の「見た目」は 3Dモデル（list_card_model）
	* - クリック判定は UIボタン（list_card_hitbox）
	* - ロック状態に合わせて表示/非表示を同期する
	* - 何らかの理由でモデルが欠けていた場合はここで再生成する（安全復旧）
	*/
void StageSelectScene::ReflowUnlockedCardsLayout()
{
	if (!m_coordinator) return;

	constexpr int kTotalStages = 6;

	// 配列を常に 6 に揃える（不足していたら埋める）
	if ((int)m_listCardModelEntities.size() < kTotalStages)
	{
		m_listCardModelEntities.resize(kTotalStages, (ECS::EntityID)-1);
	}

	// UI→3D 変換用
	constexpr float kListCardZ = 4.8f;

	// ★一覧カードは UI用カードモデル想定なので、ワールドスケールは小さく固定（大きすぎる症状対策）
	constexpr float kListCardScale = 0.03f;

	for (int stageNo = 1; stageNo <= kTotalStages; ++stageNo)
	{
		const int idx = stageNo - 1;
		const bool unlocked = IsStageUnlocked(stageNo);

		// スロット中心（UI座標）→ 3Dワールドへ変換して配置
		const DirectX::XMFLOAT3 uiPos = GetListCardSlotCenterPos(stageNo);
		const DirectX::XMFLOAT3 wpos = UIToWorld(uiPos.x, uiPos.y, kListCardZ);

		// ----------------------------------------------------------
		// 1) 3Dカード（見た目）
		// ----------------------------------------------------------
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

		// 一覧表示中かつ解放済みなら表示、詳細中なら非表示
		const bool showModel = (!m_isDetailMode) && unlocked;
		SetUIVisible(cardModel, showModel);

		// ----------------------------------------------------------
		// 2) クリック判定（ヒットボックス）
		// ----------------------------------------------------------
		if (idx < (int)m_listUIEntities.size())
		{
			// ★重要：ヒットボックスの座標を「一覧カードのスロット中心」に必ず同期する
			// - これをやらないと、カード(3D)だけ移動して「クリック判定」と「番号ラベル(1-1など)」がズレる。
			// - SyncStageNumberLabel は GetUIRect(=ヒットボックス) を基準に中央揃え計算しているため。
			const ECS::EntityID hit = m_listUIEntities[idx];
			if (hit != (ECS::EntityID)-1 && m_coordinator->HasComponent<TransformComponent>(hit))
			{
				auto& tr = m_coordinator->GetComponent<TransformComponent>(hit);
				tr.position.x = uiPos.x;
				tr.position.y = uiPos.y;
				tr.position.z = 5.0f;
				// クリック判定のサイズは固定（Hoverで拡大しない前提）
				tr.scale = DirectX::XMFLOAT3(250.0f, 150.0f, 1.0f);
				tr.rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			}

			const bool showHitbox = (!m_isDetailMode) && unlocked;
			SetUIVisible(hit, showHitbox);
		}

		// ----------------------------------------------------------
		// 3) ステージ番号ラベル
		// ----------------------------------------------------------
		SetStageNumberLabelVisible(stageNo, (!m_isDetailMode) && unlocked);
	}

	// ★配置を変えたら、番号ラベルも必ず再同期（ここを外すと1フレームだけズレる環境がある）
	SyncStageNumberLabels(true);
}

void StageSelectScene::SetUIVisible(ECS::EntityID id, bool visible)
{
	if (!m_coordinator) return;

	// NOTE:
	//  既存のUI描画が isVisible を参照しない実装だった場合でも確実に消えるように、
	//  アルファも一緒に落とす（RenderComponent 側も同期）。
	//  ※visible=true のときは「今が完全透明(0)なら 1 に戻す」だけにして、
	//    Reveal演出中の半透明を上書きしない。
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
			// 0のときだけ復帰（演出中の途中値を壊さない）
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

		// ★3Dモデルは「type」でON/OFFされることがあるため、ModelComponent を持つものは type も同期する
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
			// 0のときだけ復帰（演出中の途中値を壊さない）
			if (rc.color.w <= 0.001f) rc.color.w = targetAlpha;
		}
	}
}


// ------------------------------------------------------------
// Stage number labels ("1-1" etc) : UI_STAGEMOJI* on list cards
// ------------------------------------------------------------
// 仕様：数字は UI_STAGEMOJI0～9、ハイフンは UI_STAGEMOJI_ をCSVで管理した切り出しで描画する。
// 注意：uvPos/uvScale をコード側で上書きするとアトラス切り出しが壊れて「点/細線」になるので上書きしない。
// 太さ：同じ文字を8方向に重ね描きして擬似ボールド化（画像自体は太くできないため）。
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

// ===== 調整用パラメータ（ここだけ触ればOK）=====
static constexpr float kStageNoScale = 1.75f;            // 全体サイズ倍率
static constexpr float kStageNoMarginBelow = 5.0f;      // カード下の余白
static constexpr float kStageNoDepth = 5.0f;             // UIのdepth
static constexpr float kStageNoBaseZ = 0.10f;            // Transform.z（擬似ボールドのZずらし基準）

// ★ステージ番号背景（UI_STAGE_NUMBER）の設定
static constexpr float kStageNoBgWidth = 180.0f;   // 背景の幅（元: 120.0f）
static constexpr float kStageNoBgHeight = 75.0f;   // 背景の高さ（元: 50.0f）
static constexpr float kStageNoBgDepth = 4.9f;     // 背景のdepth（文字より後ろ）
// ★背景の微調整オフセット（文字が背景の中心からずれている場合に調整）
static constexpr float kStageNoBgOffsetX = 10.0f;   // X方向のオフセット（正で右、負で左）
static constexpr float kStageNoBgOffsetY = -35.0f;   // Y方向のオフセット（正で下、負で上）


// ★★★ ステージ別の個別オフセット設定 ★★★
// 各ステージのラベル位置を個別に調整できます
// offsetX: 正の値で右に、負の値で左に移動
// offsetY: 正の値で下に、負の値で上に移動
struct StageNoOffset
{
	float textOffsetX;    // 文字のXオフセット
	float textOffsetY;    // 文字のYオフセット
	float bgOffsetX;      // 背景のXオフセット
	float bgOffsetY;      // 背景のYオフセット
};

static const StageNoOffset kStageNoOffsets[6] =
{
	// { 文字X, 文字Y, 背景X, 背景Y }
	{ 100.0f,  10.0f,  -90.0f,  -10.0f },  // ステージ1 (1-1)
	{ 20.0f,   10.0f,   3.0f,  -10.0f },  // ステージ2 (1-2)
	{ 110.0f,  10.0f,   92.0f,  -10.0f },  // ステージ3 (1-3)
	{ -85.0f,  75.0f,   -90.0f,  55.0f },  // ステージ4 (2-1)
	{ 20.0f,   75.0f,   3.0f,  55.0f },  // ステージ5 (2-2)
	{ 110.0f,  75.0f,   92.0f,  55.0f },  // ステージ6 (2-3)
};
// 使用例：
// ステージ1の文字を右に10px、下に5px、背景を左に5px移動したい場合:
// { 10.0f,  5.0f,  -5.0f,  0.0f },  // ステージ1 (1-1)
// ステージ3の文字はそのまま、背景だけを上に10px移動したい場合:
// { 110.0f,  10.0f,  0.0f,  -10.0f },  // ステージ3 (1-3)
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★

// ★★★ 文字ごとの個別オフセット（1-1 の「左数字」「-」「右数字」別に調整）★★★
// ここを触ると、「1」だけ右へ / 「-」だけ上へ、などができます。
// stageNo=1..6, charIndex=0..2  (例: "1-1" -> [0]='1', [1]='-', [2]='1')
struct StageNoCharOffset2
{
	float x;
	float y;
};

static const StageNoCharOffset2 kStageNoCharOffsets[6][3] =
{
	//  { 左数字(dx,dy),      '-'(dx,dy),        右数字(dx,dy) }
	{ { -90.0f, -17.0f }, { -220.0f, -10.0f }, { -190.0f, -17.0f } },
	// stage2 "1-2"
	{ { 80.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	// stage3 "1-3"
	{ { 80.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },
	// stage4 "2-1"
	{ { 75.0f, -17.0f }, { -40.0f, -10.0f }, { -10.0f, -17.0f } },
	// stage5 "2-2"
	{ { 65.0f, -17.0f }, { -50.0f, -10.0f }, { -30.0f, -17.0f } },
	// stage6 "2-3"
	{ { 65.0f, -17.0f }, { -50.0f, -10.0f }, { -50.0f, -17.0f } },
};

// 例）
// - ステージ1の「-」だけを 3px 上にしたい：kStageNoCharOffsets[0][1] = {0, -3}
// - ステージ4の右数字だけを 5px 右へ：     kStageNoCharOffsets[3][2] = {+5, 0}
// ★★★★★★★★★★★★★★★★★★★★★★★★★★★



// 文字サイズ（基準値×kStageNoScale）
// ★要望：UI_STAGEMOJI1～9 と UI_STAGEMOJI_ を UI_STAGEMOJI2 と同じサイズで表示する
// ＝ グリフ毎の個別メトリクスをやめて「共通サイズ」に統一する。
static constexpr float kStageNoGlyphWBase = 80.0f;        // UI_STAGEMOJI2 を基準
static constexpr float kStageNoGlyphHBase = 75.0f;        // UI_STAGEMOJI2 を基準
static constexpr float kStageNoGlyphAdvanceBase = 52.0f;  // UI_STAGEMOJI2 を基準（文字間隔）

// "-"（UI_STAGEMOJI_）も同一サイズにする（必要なら kStageNoHyphenYOffsetBase で位置だけ微調整）
static constexpr float kStageNoHyphenYOffsetBase = -5.0f; // '-'の上下位置（正で上、負で下）
static constexpr float kStageNoHyphenXOffsetBase = 0.0f;  // '-'の左右位置（正で右、負で左）

static inline void StageNo_GetGlyphMetrics(char c, float& outW, float& outH, float& outAdv)
{
	(void)c; // すべて同じメトリクス
	outW = kStageNoGlyphWBase * kStageNoScale;
	outH = kStageNoGlyphHBase * kStageNoScale;
	outAdv = kStageNoGlyphAdvanceBase * kStageNoScale;
}

// 擬似ボールド設定
static constexpr bool  kStageNoBoldEnabled = true;
static constexpr int   kStageNoBoldPasses = 9;           // 1(本体)+8(周囲) = 9
static constexpr float kStageNoBoldOffsetPxBase = 1.0f;  // 太さ（ピクセル）
// 太さ（ピクセル）※太さはちょうど良いので触らない想定

void StageSelectScene::BuildStageNumberLabels()
{
	if (!m_coordinator) return;

	// 既存があれば破棄（本体）
	for (auto& v : m_listStageLabelEntities)
	{
		for (auto id : v)
		{
			if (id != (ECS::EntityID)-1) m_coordinator->DestroyEntity(id);
		}
	}
	// 既存があれば破棄（擬似ボールド）
	for (auto& v : m_listStageLabelBoldEntities)
	{
		for (auto id : v)
		{
			if (id != (ECS::EntityID)-1) m_coordinator->DestroyEntity(id);
		}
	}

	// 既存があれば破棄（背景）
	for (auto id : m_listStageLabelBgEntities)
	{
		if (id != (ECS::EntityID)-1) m_coordinator->DestroyEntity(id);
	}

	m_listStageLabelEntities.clear();
	m_listStageLabelBoldEntities.clear();
	m_listStageLabelEntities.resize(6);
	m_listStageLabelBoldEntities.resize(6);
	m_listStageLabelBgEntities.clear();
	m_listStageLabelBgEntities.resize(6, (ECS::EntityID)-1);


	// 作成時の最大枠（後で文字ごとにscaleを上書きする）
	// ★数字2と3の大きなサイズも考慮してmaxW/maxHを計算
	const float maxW = kStageNoGlyphWBase * kStageNoScale;
	const float maxH = kStageNoGlyphHBase * kStageNoScale;

	for (int stageNo = 1; stageNo <= 6; ++stageNo)
	{
		const std::string text = StageNoToLabelText(stageNo);

		// ★UI_STAGE_NUMBER背景を作成
		const int idx = stageNo - 1;
		m_listStageLabelBgEntities[idx] = m_coordinator->CreateEntity(
			TransformComponent({ 0.0f, 0.0f, kStageNoBaseZ }, { 0,0,0 }, { kStageNoBgWidth, kStageNoBgHeight, 1.0f }),
			UIImageComponent("UI_STAGE_NUMBER", kStageNoBgDepth, true, { 1,1,1,1 })
		);


		auto& vec = m_listStageLabelEntities[stageNo - 1];
		auto& vecBold = m_listStageLabelBoldEntities[stageNo - 1];
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

			// uvPos/uvScale はCSV側の切り出しを使う（上書きしない）
			if (!assetId && m_coordinator->HasComponent<UIImageComponent>(e))
			{
				auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);
				ui.isVisible = false;
				ui.color.w = 0.0f;
			}

			vec.push_back(e);

			// 擬似ボールド（重ね描き：本体とは別Entity）
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
	}
}

void StageSelectScene::SetStageNumberLabelVisible(int stageNo, bool visible)
{
	if (stageNo < 1 || stageNo >(int)m_listStageLabelEntities.size()) return;

	for (auto id : m_listStageLabelEntities[stageNo - 1]) SetUIVisible(id, visible);

	if (stageNo >= 1 && stageNo <= (int)m_listStageLabelBoldEntities.size())
	{
		for (auto id : m_listStageLabelBoldEntities[stageNo - 1]) SetUIVisible(id, visible);

		// ★背景の表示/非表示も同期
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
	(void)force;
	for (int stageNo = 1; stageNo <= 6; ++stageNo) SyncStageNumberLabel(stageNo);
}

void StageSelectScene::SyncStageNumberLabel(int stageNo)
{
	if (!m_coordinator) return;
	if (stageNo < 1 || stageNo > 6) return;

	const int idx = stageNo - 1;
	if (idx < 0 || idx >= (int)m_listUIEntities.size()) return;
	if (idx >= (int)m_listStageLabelEntities.size()) return;

	const ECS::EntityID cardId = m_listUIEntities[idx];
	if (cardId == (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(cardId, l, t, r, b)) return;

	// カード可視/アルファに追従（ただし薄すぎると読めないので下限を持つ）
	bool cardVis = true;
	float cardAlpha = 1.0f;

	// 1) hitbox(UIButton) の可視を最優先
	if (m_coordinator->HasComponent<UIButtonComponent>(cardId))
	{
		const auto& btn = m_coordinator->GetComponent<UIButtonComponent>(cardId);
		cardVis = btn.isVisible;
	}

	// 2) 3Dモデル側の状態も反映（存在する場合）
	if (idx >= 0 && idx < (int)m_listCardModelEntities.size())
	{
		const ECS::EntityID modelId = m_listCardModelEntities[idx];
		if (modelId != (ECS::EntityID)-1 && m_coordinator->HasComponent<RenderComponent>(modelId))
		{
			const auto& rc = m_coordinator->GetComponent<RenderComponent>(modelId);
			cardAlpha = rc.color.w;
			// type が消されている(or 透明)ならラベルも消す
			if (rc.type == MESH_NONE || rc.color.w <= 0.001f)
			{
				cardVis = false;
			}
		}
	}

	// 3) それでも情報がない場合のみ、cardId 自体の UIImage/Render を参照
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

	// 文字列とメトリクスを復元（例："1-2"）
	const std::string text = StageNoToLabelText(stageNo);
	auto& vec = m_listStageLabelEntities[idx];

	// 合計幅（プロポーショナル）- 正確な計算
	float totalW = 0.0f;
	for (size_t i = 0; i < text.size(); ++i)
	{
		float w = 0.0f, h = 0.0f, adv = 0.0f;
		StageNo_GetGlyphMetrics(text[i], w, h, adv);

		if (i == text.size() - 1)
		{
			// 最後の文字：現在の位置 + 文字の幅
			totalW += w;
		}
		else
		{
			// 途中の文字：advance を加算
			totalW += adv;
		}
	}

	const float centerX = (l + r) * 0.5f;
	const float baseX = centerX - totalW * 0.5f;

	// ★ステージ別の個別オフセットを適用（文字用）
	const float stageTextOffsetX = (idx >= 0 && idx < 6) ? kStageNoOffsets[idx].textOffsetX : 0.0f;
	const float stageTextOffsetY = (idx >= 0 && idx < 6) ? kStageNoOffsets[idx].textOffsetY : 0.0f;
	// ★ステージ別の個別オフセットを適用（背景用）
	const float stageBgOffsetX = (idx >= 0 && idx < 6) ? kStageNoOffsets[idx].bgOffsetX : 0.0f;
	const float stageBgOffsetY = (idx >= 0 && idx < 6) ? kStageNoOffsets[idx].bgOffsetY : 0.0f;

	// 太さ（重ね描きのオフセット）
	const float boldOffset = kStageNoBoldOffsetPxBase;

	// 8方向オフセット
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

	// 配置：cursorで横並び、ハイフンは専用advance＆yOffsetで数字の真ん中へ
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

		// ★ステージ別オフセットを適用
		float perCharOffsetX = 0.0f;
		float perCharOffsetY = 0.0f;
		if (idx >= 0 && idx < 6 && i >= 0 && i < 3)
		{
			perCharOffsetX = kStageNoCharOffsets[idx][i].x;
			perCharOffsetY = kStageNoCharOffsets[idx][i].y;
		}

		const float cx = baseX + cursor + w * 0.5f + xOff + stageTextOffsetX + perCharOffsetX;
		const float cy = (baseY + digitH * 0.5f) + yOff + stageTextOffsetY + perCharOffsetY;

		// Transform
		if (m_coordinator->HasComponent<TransformComponent>(e))
		{
			auto& tr = m_coordinator->GetComponent<TransformComponent>(e);
			tr.position.x = cx;
			tr.position.y = cy;
			tr.position.z = kStageNoBaseZ; // 本体
			tr.scale = DirectX::XMFLOAT3(w, h, 1.0f);
		}

		// Visible/Alpha
		if (m_coordinator->HasComponent<UIImageComponent>(e))
		{
			auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);
			ui.isVisible = cardVis;
			ui.color.w = cardVis ? std::max(0.95f, cardAlpha) : 0.0f;
		}

		// 擬似ボールド：Zもずらす
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

		// ★UI_STAGE_NUMBER背景の位置とvisibilityを同期
		if (idx >= 0 && idx < (int)m_listStageLabelBgEntities.size())
		{
			const ECS::EntityID bgId = m_listStageLabelBgEntities[idx];
			if (bgId != (ECS::EntityID)-1)
			{
				// 背景の中心位置を計算（文字列全体の中心）
				const float bgCenterX = centerX + stageBgOffsetX + kStageNoBgOffsetX;
				// ★背景を文字と同じY座標に配置（baseYは文字の下端なので、文字の高さの半分を足す）
				const float bgCenterY = (baseY + digitH * 0.5f) + stageBgOffsetY + kStageNoBgOffsetY;


				if (m_coordinator->HasComponent<TransformComponent>(bgId))
				{
					auto& tr = m_coordinator->GetComponent<TransformComponent>(bgId);
					tr.position.x = bgCenterX;
					tr.position.y = bgCenterY;
					tr.position.z = kStageNoBaseZ - 0.01f; // 文字より少し後ろ
					tr.scale = DirectX::XMFLOAT3(kStageNoBgWidth, kStageNoBgHeight, 1.0f);
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



/**
 * [void - PlayUISelectEffect]
 * @brief UIエンティティの中心にワンショットエフェクトを出す
 */
void StageSelectScene::PlayUISelectEffect(ECS::EntityID uiEntity, const std::string& effectId, float scale)
{
	if (!m_coordinator) return;

	// ★カード選択中/遷移中はエフェクトを出さない
	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;


	float l, t, r, b;
	if (!GetUIRect(uiEntity, l, t, r, b)) return;

	const float cx = (l + r) * 0.5f;
	const float cy = (t + b) * 0.5f;

	const float uiH = (b - t);

	// UI depth: keep the effect just above the button so it visually sits on top.
	float uiZ = 0.0f;
	if (m_coordinator->HasComponent<TransformComponent>(uiEntity))
	{
		uiZ = m_coordinator->GetComponent<TransformComponent>(uiEntity).position.z;
	}
	// ★UIの真上（上端より少し上）にオフセット
	// 0.6f を増やすほど、より上に行きます（例：0.7f, 0.8f）
	const float effectX = cx;
	const float effectY = cy - uiH * 0.30f; // slightly above center for natural overlap

	// UIスクリーン空間（EffectSystemは SetScreenSpaceCamera 済み）
	const float z = uiZ + 0.01f;

	// ★流れ星と同じ「小さいスケール帯」を使う（まず見えることを優先）
	const float finalScale = scale;   // ← 200倍をやめる

	ECS::EntityID fx = m_coordinator->CreateEntity(
		TagComponent("ui_select_fx"),
		TransformComponent({ effectX, effectY, z }, { 0,0,0 }, { 1,1,1 }),
		EffectComponent(effectId, false, true, { 0,0,0 }, finalScale)
	);

	// 手動で少し後に掃除（LifeTimeSystemに依存しない）
	m_uiSelectFx.push_back({ fx, 1.0f }); // 1秒もあれば十分
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

void StageSelectScene::UpdateEyeLight(float dt)
{
	// --------------------------------------------------------
	// 1. 既存のエフェクトの寿命管理（自動消滅）
	// --------------------------------------------------------
	for (int i = (int)m_activeEyeLights.size() - 1; i >= 0; --i)
	{
		auto& pair = m_activeEyeLights[i];
		pair.second -= dt; // 寿命を減らす

		// 寿命切れ、またはEntityが既に無効ならリストから削除
		if (pair.second <= 0.0f || pair.first == (ECS::EntityID)-1)
		{
			if (pair.first != (ECS::EntityID)-1)
			{
				m_coordinator->DestroyEntity(pair.first);
			}
			m_activeEyeLights.erase(m_activeEyeLights.begin() + i);
		}
	}
	// 詳細モード以外や、遷移中は再生しない
	if (m_isDetailMode) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return;

	// タイマー更新

	// ★カード選択中/遷移中は新規エフェクトを出さない
	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;

	m_eyeLightTimer += dt;

	// 待ち時間を経過したら生成
	if (m_eyeLightTimer >= m_eyeLightNextInterval)
	{
		// 次回の設定（例: 0.5秒 〜 1.5秒 の間でランダム）
		m_eyeLightTimer = 0.0f;
		std::uniform_real_distribution<float> dist(0.5f, 1.5f);
		m_eyeLightNextInterval = dist(m_rng);

		// ============================================================
		// ★解放済みステージカードの位置リストを作成
		// ============================================================
		// ============================================================
		// ★解放済みステージカードの位置リストを作成
		// ============================================================
		struct EyePoint { int stageNo; float x; float y; };
		std::vector<EyePoint> points;

		// ------------------------------------------------------------
		// ★目エフェクトの座標調整（ここだけ触ればOK）
		//  - Global: 全体一括オフセット
		//  - Stage : ステージ(カード)ごとの微調整
		//  - Jitter: 少しだけランダムに揺らして自然に
		// ------------------------------------------------------------
		static constexpr float kEyeGlobalOffsetX = 0.0f;
		static constexpr float kEyeGlobalOffsetY = 0.0f;
		static constexpr DirectX::XMFLOAT2 kEyeStageOffset[6] =
		{
			{ -120.0f, -30.0f }, // stage1
			{ -30.0f, -30.0f }, // stage2
			{ 70.0f, -30.0f }, // stage3
			{ -120.0f, 38.0f }, // stage4
			{ -30.0f, 38.0f }, // stage5
			{ 70.0f, 38.0f }, // stage6
		};
		static constexpr float kEyeJitterX = 0.0f; // 例: 6.0f で左右に少し揺れる
		static constexpr float kEyeJitterY = 0.0f; // 例: 3.0f で上下に少し揺れる
		static constexpr float kEyeZ = 0.0f;       // 目エフェクトのZ（前に出したいなら +）
		static constexpr float kEyeScale = 5.0f;   // 大きすぎるなら 1.0f〜3.0f へ
		static constexpr bool  kEyeDebugLog = false; // trueにすると座標がログに出る

		// 解放済みステージのみを対象に
		for (int stageNo = 1; stageNo <= m_maxUnlockedStage && stageNo <= 6; ++stageNo)
		{
			DirectX::XMFLOAT3 pos = GetListCardSlotCenterPos(stageNo);
			points.push_back({ stageNo, pos.x, pos.y });
		}

		// 解放済みステージがない場合は何もしない
		if (points.empty()) return;

		// リストの中からランダムに1つ選ぶ
		std::uniform_int_distribution<int> idxDist(0, (int)points.size() - 1);
		const int pick = idxDist(m_rng);

		const int stageNo = points[pick].stageNo;
		float cx = points[pick].x;
		float cy = points[pick].y;

		// ------------------------------------------------------------
		// ★座標調整を適用（ここが“全ての目エフェクト”の基準）
		// ------------------------------------------------------------
		if (stageNo >= 1 && stageNo <= 6)
		{
			cx += kEyeGlobalOffsetX + kEyeStageOffset[stageNo - 1].x;
			cy += kEyeGlobalOffsetY + kEyeStageOffset[stageNo - 1].y;
		}
		if (kEyeJitterX != 0.0f || kEyeJitterY != 0.0f)
		{
			std::uniform_real_distribution<float> jx(-kEyeJitterX, kEyeJitterX);
			std::uniform_real_distribution<float> jy(-kEyeJitterY, kEyeJitterY);
			cx += jx(m_rng);
			cy += jy(m_rng);
		}

		if (kEyeDebugLog)
		{
			std::cout
				<< "[StageSelect] EyeLight spawn stage=" << stageNo
				<< " pos=(" << cx << "," << cy << ")"
				<< " global=(" << kEyeGlobalOffsetX << "," << kEyeGlobalOffsetY << ")"
				<< " stageOff=(";
			if (stageNo >= 1 && stageNo <= 6)
			{
				std::cout << kEyeStageOffset[stageNo - 1].x << "," << kEyeStageOffset[stageNo - 1].y;
			}
			else
			{
				std::cout << "N/A";
			}
			std::cout << ")\n";
		}

		// --- エフェクト生成 ---
		ECS::EntityID fxEntity = m_coordinator->CreateEntity(
			TagComponent("effect_eyeslight"),
			TransformComponent({ cx, cy, kEyeZ }, { 0,0,0 }, { 1,1,1 }),
			EffectComponent(
				"EFK_EYESLIGHT", // ★アセットID (要登録)
				false,           // ループしない（1回再生）
				true,            // 生成時に即再生
				{ 0,0,0 },       // オフセット
				kEyeScale        // ★スケール (大きすぎる場合は 1.0f 等に調整)
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

	// ★カード選択中/遷移中はエフェクトを出さない
	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return; // 遷移中は見えないので作らない
	if (m_stageMapEntity == (ECS::EntityID)-1) return;
	if (m_debugStarEntity != (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(m_stageMapEntity, l, t, r, b)) return;

	const float cx = (l + r) * 0.5f;
	const float cy = (t + b) * 0.5f;

	// ★まず「エフェクト自体が描けるか」を確実に確認するため、動作実績のある 
	// を常駐表示
	m_debugStarEntity = m_coordinator->CreateEntity(
		TagComponent("effect_debug"),
		TransformComponent({ cx, cy, 9.5f }, { 0,0,0 }, { 1,1,1 }),
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

	// ★カード選択中/遷移中はエフェクトを出さない
	if (m_inputLocked || m_isWaitingForTransition || m_cardFocus.active) return;
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
	const float z = 0.0f; // MAPBACK(10.0) と  の間

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

// ステージ固有のUIを作成する関数
void StageSelectScene::CreateStageInfoUI(const std::string& stageID)
{
	// 文字列ID("ST_001") を 整数(1) に変換
	int stageNo = StageIdToStageNo(stageID);

	// UIの深さ基準（詳細画面と同じ設定）
	const float baseDepth = 110000.0f;

	// switch文で分岐（整数ならOK！）
	switch (stageNo)
	{
	case 1: // 1-1
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE3", baseDepth + 3.2f)
		));
		break;

	}
	case 2: // 1-2
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE3", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE1", baseDepth + 3.2f)
		));
		break;
	}
	case 3: // 1-3
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE3", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE1", baseDepth + 3.2f)
		));
		break;
	}
	case 4: // 2-1
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE7", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE8", baseDepth + 3.2f)
		));
		break;
	}
	case 5: // 2-2
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE7", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE8", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE9", baseDepth + 3.2f)
		));
		break;
	}
	case 6: // 2-3
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.63f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE7", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE8", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.83f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE9", baseDepth + 3.2f)
		));
		break;
	}
	case 7: //4個出すとき用
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.58f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.88f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 200, 200, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.58f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE7", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.68f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE8", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.78f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE9", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.88f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 85, 85, 1 }),
			UIImageComponent("ICO_TREASURE10", baseDepth + 3.2f)
		));
		break;
	}
	case 8: //5個出すとき用
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.560f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 170, 170, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.645f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 170, 170, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.730f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 170, 170, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.815f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 170, 170, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.900f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 170, 170, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.560f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 70, 70, 1 }),
			UIImageComponent("ICO_TREASURE7", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.645f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 70, 70, 1 }),
			UIImageComponent("ICO_TREASURE8", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.730f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 70, 70, 1 }),
			UIImageComponent("ICO_TREASURE9", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.815f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 70, 70, 1 }),
			UIImageComponent("ICO_TREASURE10", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.900f, SCREEN_HEIGHT * 0.21f, 0.0f }, { 0,0,0 }, { 70, 70, 1 }),
			UIImageComponent("ICO_TREASURE11", baseDepth + 3.2f)
		));
		break;
	}
	case 9: // 6個出すとき用
	{
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.555f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 140, 140, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.625f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 140, 140, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.695f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 140, 140, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.765f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 140, 140, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.835f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 140, 140, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーケース
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.905f, SCREEN_HEIGHT * 0.25f, 0.0f }, { 0,0,0 }, { 140, 140, 1 }),
			UIImageComponent("UI_TRESURE_CASE", baseDepth + 3.1f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.555f, SCREEN_HEIGHT * 0.22f, 0.0f }, { 0,0,0 }, { 55, 55, 1 }),
			UIImageComponent("ICO_TREASURE7", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.625f, SCREEN_HEIGHT * 0.22f, 0.0f }, { 0,0,0 }, { 55, 55, 1 }),
			UIImageComponent("ICO_TREASURE8", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.695f, SCREEN_HEIGHT * 0.22f, 0.0f }, { 0,0,0 }, { 55, 55, 1 }),
			UIImageComponent("ICO_TREASURE9", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.765f, SCREEN_HEIGHT * 0.22f, 0.0f }, { 0,0,0 }, { 55, 55, 1 }),
			UIImageComponent("ICO_TREASURE10", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.835f, SCREEN_HEIGHT * 0.22f, 0.0f }, { 0,0,0 }, { 55, 55, 1 }),
			UIImageComponent("ICO_TREASURE11", baseDepth + 3.2f)
		));
		// トレジャーアイコン
		m_stageSpecificEntities.push_back(m_coordinator->CreateEntity(
			TransformComponent({ SCREEN_WIDTH * 0.905f, SCREEN_HEIGHT * 0.22f, 0.0f }, { 0,0,0 }, { 55, 55, 1 }),
			UIImageComponent("ICO_TREASURE12", baseDepth + 3.2f)
		));
		break;
	}
	}

	// 作成した固有UIにも「詳細出現アニメーション（ふわっと出るやつ）」を適用するために登録
	for (auto id : m_stageSpecificEntities) {
		CacheDetailBaseTransform(id); // これを忘れるとアニメーションしません
		m_detailUIEntities.push_back(id); // 共通管理リストにも入れておくと、一括操作に便利
	}
}

// 固有UIを削除する関数
void StageSelectScene::ClearStageInfoUI()
{
	// 作成したエンティティをECSから削除
	for (auto id : m_stageSpecificEntities) {
		if (id != (ECS::EntityID)-1) {
			m_coordinator->DestroyEntity(id);
		}
	}
	m_stageSpecificEntities.clear();

	// m_detailUIEntities の中からも無効になったIDを消しておくと安全ですが、
	// 現在の実装では SwitchState で再構築されるわけではないので、
	// ここで明示的に remove するか、m_detailUIEntities の再構築ロジックを見直す必要があります。
	// 今回は簡易的に「固有UIは毎回作り直す」方針をとります。
}


// StageSelectScene.cpp 例
// StageSelectScene.cpp
