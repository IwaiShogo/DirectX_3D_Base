/*****************************************************************//**
 * @file	StageSelectScene.cpp
 *********************************************************************/

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

static DirectX::XMFLOAT3 UIToWorld(float sx, float sy, float zWorld)
{
	// 1ワールド単位 = 100px くらいから調整開始（必要なら後で詰める）
	constexpr float PIXELS_PER_UNIT = 100.0f;

	const float wx = (sx - SCREEN_WIDTH * 0.5f) / PIXELS_PER_UNIT;
	const float wy = -(sy - SCREEN_HEIGHT * 0.5f) / PIXELS_PER_UNIT; // Y反転（UI↓ / 3D↑）
	return DirectX::XMFLOAT3(wx, wy, zWorld);
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

//--------------------------------------------------------------
// Card Focus Animation（ボタン押下 → カードが中央に来る）
//--------------------------------------------------------------
void StageSelectScene::StartCardFocusAnim(ECS::EntityID cardEntity, const DirectX::XMFLOAT3& uiPos)
{
	constexpr float kCardZ = 5.0f;        // 6.0f → 5.0f（少し手前）
	constexpr float kEndScale = 0.15f;    // 0.22f → 0.70f（画面いっぱい狙い）
	constexpr float kStartScaleMul = 0.10f;
	constexpr float kDuration = 1.5f;    // 0.45f → 0.70f（“だんだん”感を強める）

	constexpr float kExtraRollRad = DirectX::XMConvertToRadians(50.0f); // 追加で少しだけZ回転（演出）
	// 画面「中央」補正（+で右 / -で左、-で上 / +で下）
// 画面「中央」補正（+で右 / -で左、-で上 / +で下）
	const float kCenterOffsetPxX = SCREEN_WIDTH * 0.0f;
	const float kCenterOffsetPxY = -SCREEN_HEIGHT * 0.0f;



	m_cardFocus.active = true;
	m_cardFocus.entity = cardEntity;
	m_cardFocus.elapsed = 0.0f;
	m_cardFocus.duration = kDuration;

	m_cardFocus.startPos = UIToWorld(uiPos.x, uiPos.y, kCardZ);
	m_cardFocus.endPos = UIToWorld(
		SCREEN_WIDTH * 0.5f + kCenterOffsetPxX,
		SCREEN_HEIGHT * 0.5f + kCenterOffsetPxY,
		kCardZ
	);

	m_cardFocus.endScale = DirectX::XMFLOAT3(kEndScale, kEndScale, kEndScale);
	m_cardFocus.startScale = DirectX::XMFLOAT3(kEndScale * kStartScaleMul, kEndScale * kStartScaleMul, kEndScale * kStartScaleMul);

	m_cardFocus.baseRot = DirectX::XMFLOAT3(0.0f, DirectX::XM_PI, 0.0f); // カード表向きの向きに合わせる
	m_cardFocus.extraRollRad = kExtraRollRad;

	{
		auto& tr = m_coordinator->GetComponent<TransformComponent>(cardEntity);
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

		EntityID btn = m_coordinator->CreateEntity(
			TransformComponent({ x, y, 5.0f }, { 0,0,0 }, { 250, 150, 1 }),
			UIImageComponent("BTN_STAGE_SELECT", 1.0f),
			RenderComponent(MESH_NONE, { 1.0f, 1.0f, 1.0f, 1.0f }),

			// ★★★ 修正箇所：ここをコメントアウト ★★★
			// これらが有効だと、裏で6個分の時間が進んでしまい、メインの演出が一瞬で終わってしまいます。
			// ボタン自体は UIImageComponent で表示されているため、これを消しても見た目は変わりません。
			// ModelComponent("M_CARD", 5.0f, Model::None),
			// AnimationComponent({ "A_CARD_COMEON" }),
			// ★★★★★★★★★★★★★★★★★★★★★★★

			UIButtonComponent(
				ButtonState::Normal,
				true,
				[this, id, i]() {
					if (m_inputLocked || m_isWaitingForTransition) return;
					if (m_isDetailMode) return; // 詳細表示中は一覧からの再選択を無効化

					// ★一覧→詳細の演出は「毎回必ず同じ初期状態」から開始する
					// Destroy→Create で EntityID が再利用されると、AnimationSystem 側のキャッシュにより
					// 「前回の続きから再生」になることがあるため、ここでは旧フォーカスカードは破棄しない。
					ResetSelectToDetailAnimState(false, true);

					m_selectedStageID = id;
					m_inputLocked = true;

					if (i < m_listUIEntities.size())
					{
						EntityID currentBtnID = m_listUIEntities[i];

						// 1. 選択したカード（ボタン）を消す（集中カード演出のため）
						SetUIVisible(currentBtnID, false);
						m_lastHiddenListCardEntity = currentBtnID;


						// 2. クリック位置から出す「集中カード」を新規作成
						DirectX::XMFLOAT3 uiPos = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f };
						if (m_coordinator->HasComponent<TransformComponent>(currentBtnID))
						{
							uiPos = m_coordinator->GetComponent<TransformComponent>(currentBtnID).position;
						}

						// 旧フォーカスカードは「新規作成後」に破棄して EntityID 再利用を回避する
						ECS::EntityID oldFocus = m_focusCardEntity;

						m_focusCardEntity = m_coordinator->CreateEntity(
							TagComponent("focus_card"),
							TransformComponent({ 0.0f, 0.0f, 0.0f }, { 0,0,0 }, { 1.0f, 1.0f, 1.0f }),
							RenderComponent(MESH_MODEL, { 1.0f, 1.0f, 1.0f, 1.0f }),
							ModelComponent("M_CARD", 5.0f, Model::None),
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

						// 3. ボタン位置 → 画面中央へ「補間」で移動/拡大（段階演出）
						StartCardFocusAnim(m_focusCardEntity, uiPos);

						// ★再生速度（StageSelectScene.h の LIST_TO_DETAIL_ANIM_SPEED で調整）
						const float animSpeed = LIST_TO_DETAIL_ANIM_SPEED;

						// アニメーション再生（これが唯一の再生者になるので正常速度になる）
						if (m_coordinator->HasComponent<AnimationComponent>(m_focusCardEntity))
						{
							auto& anim = m_coordinator->GetComponent<AnimationComponent>(m_focusCardEntity);
							anim.Play("A_CARD_COMEON", false, animSpeed);
						}

						// 遷移待ち開始
						m_isWaitingForTransition = true;
						m_transitionWaitTimer = 0.0f;

						// 遷移待ち時間（StageSelectScene.h の LIST_TO_DETAIL_DELAY で調整）
						// ※以前は m_cardFocus.duration や animSpeed から自動算出していたため、
						//   LIST_TO_DETAIL_DELAY を変更しても反映されませんでした。
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
		}

	}

	// 解放済みステージの並びを「見えている数」に合わせて中央寄せ
	ReflowUnlockedCardsLayout();

	// 復帰時に「今回解放されたステージ」を浮かび上がり演出
	if (m_pendingRevealStage >= 2 && m_pendingRevealStage <= 6 && IsStageUnlocked(m_pendingRevealStage))
	{
		BeginStageReveal(m_pendingRevealStage);
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

	// UI_STAGE_MAP (BACK=10.0, SIRO=9.0)
	EntityID mapBack = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 10.0f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE_MAPBACK", baseDepth + 1.0f) // 0.90f -> baseDepth + 1.0f
	);
	m_detailUIEntities.push_back(mapBack);
	m_stageMapEntity = mapBack;

	EntityID mapSiro = m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.25f, SCREEN_HEIGHT * 0.38f, 9.0f }, { 0,0,0 }, { 500, 480, 1 }),
		UIImageComponent("UI_STAGE_MAPSIRO", baseDepth + 2.0f) // 0.95f -> baseDepth + 2.0f
	);
	m_detailUIEntities.push_back(mapSiro);

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

	// トレジャー枠
	m_detailUIEntities.push_back(m_coordinator->CreateEntity(
		TransformComponent({ SCREEN_WIDTH * 0.73f, SCREEN_HEIGHT * 0.2f, 0.0f }, { 0,0,0 }, { 550, 220, 1 }),
		UIImageComponent("UI_TRESURE", baseDepth + 3.0f) // 1.0f -> baseDepth + 3.0f
	));

	// ★スター表示
	{
		const ResultData& rd = ResultScene::GetResultData();
		const bool hasValidResult = (rd.isCleared && (rd.stageID == m_selectedStageID));
		bool stars[3] = { hasValidResult && !rd.wasSpotted, hasValidResult && rd.collectedAllOrdered, hasValidResult && rd.clearedInTime };
		const char* conditionTex[3] = { "STAR_TEXT1","STAR_TEXT2","STAR_TEXT3" };

		float baseY = SCREEN_HEIGHT * 0.50f;
		float gapY = 55.0f;
		float starX = SCREEN_WIDTH * 0.6f;
		float captionX = SCREEN_WIDTH * 0.75f;

		for (int i = 0; i < 3; ++i)
		{
			float y = baseY + i * gapY;
			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 320.0f, 60.0f, 1.0f }),
				UIImageComponent(conditionTex[i], baseDepth + 5.0f, true, { 1,1,1,1 }) // 1.0f -> ...
			));

			float offSize = (i == 0) ? 50.0f : 34.0f;
			float onSize = (i == 0) ? 50.0f : 34.0f;

			m_detailUIEntities.push_back(m_coordinator->CreateEntity(
				TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { offSize, offSize, 1.0f }),
				UIImageComponent("ICO_STAR_OFF", baseDepth + 5.0f, true, { 1,1,1,1 }) // 1.0f -> ...
			));

			if (stars[i])
			{
				m_detailUIEntities.push_back(m_coordinator->CreateEntity(
					TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
					UIImageComponent("ICO_STAR_ON", baseDepth + 6.0f, true, { 1,1,1,1 }) // 2.0f -> ...
				));
			}
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
						m_coordinator.get(), m_transitionEntity, 0.15f, 0.35f, 0.45f,
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

	// 集中演出で隠した一覧カードを復帰（一覧を薄く残す仕様のため）
	if (m_lastHiddenListCardEntity != (ECS::EntityID)-1)
	{
		SetUIVisible(m_lastHiddenListCardEntity, true);
		m_lastHiddenListCardEntity = (ECS::EntityID)-1;
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
	UpdateUISelectFx(dtSec);
	UpdateDetailAppear(dtSec);
	UpdateButtonHoverScale(dtSec);
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
	// 目的：
	// - 「カードアニメーション（3D）」をUIより前に出す
	// - ただし画面遷移フェード（黒塗りオーバーレイ）は最前面に残す
	// 実装：
	//   1) UI背景(depth<=0)
	//   2) 通常UI(0<depth<=kOverlayDepthStart)  ※この上に3Dを描く
	//   3) 3D
	//   4) オーバーレイUI(depth>kOverlayDepthStart) ※フェードなど
	// -----------------------------------------
	const float kOverlayDepthStart = 100000.0f; // フェード(=200000)は確実にここより上

	struct VisState { ECS::EntityID id; bool uiVis; bool btnVis; };
	std::vector<VisState> savedStates;

	std::vector<ECS::EntityID> allTargets = m_listUIEntities;
	allTargets.insert(allTargets.end(), m_detailUIEntities.begin(), m_detailUIEntities.end());
	if (m_cursorEntity != (ECS::EntityID)-1) allTargets.push_back(m_cursorEntity);

	// 背景
	if (m_listBgEntity != (ECS::EntityID)-1) allTargets.push_back(m_listBgEntity);

	// フェード（オーバーレイ）も対象に入れてパス制御する
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

	// 1) UI背景パス（depth <= 0）
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

	// 2) 通常UI（0 < depth <= kOverlayDepthStart）
	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;

		bool draw = false;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = (d > 0.0f && d <= kOverlayDepthStart) && s.uiVis;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = draw || (s.btnVis && (d > 0.0f && d <= kOverlayDepthStart));
		}
		SetVisible(s.id, draw);
	}
	DrawUIOnce();

	// 3) 3D（カードアニメーションをUIの上に）
	if (rs)
	{
		rs->DrawSetup();
		rs->DrawEntities();
	}

	// 4) オーバーレイUI（depth > kOverlayDepthStart）※フェードなど
	for (const auto& s : savedStates)
	{
		if (s.id == (ECS::EntityID)-1) continue;

		bool draw = false;
		if (m_coordinator->HasComponent<UIImageComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = (d > kOverlayDepthStart) && s.uiVis;
		}
		if (m_coordinator->HasComponent<UIButtonComponent>(s.id))
		{
			const float d = GetDepth(s.id);
			draw = draw || (s.btnVis && (d > kOverlayDepthStart));
		}
		SetVisible(s.id, draw);
	}
	DrawUIOnce();

	// 5) 復元
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

	// 拡大量（好みで 1.10f → 1.15f などに）
	const float hoverMul = 1.10f;

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

			const float targetMul = hovered ? hoverMul : 1.0f;
			const float curMul = (base.x != 0.0f) ? (tr.scale.x / base.x) : 1.0f;
			const float newMul = curMul + (targetMul - curMul) * a;

			tr.scale.x = base.x * newMul;
			tr.scale.y = base.y * newMul;
			tr.scale.z = base.z;
		};

	// 一覧/詳細の両方に対して適用（表示中かどうかは btn.isVisible で弾く）
	// 詳細表示中は「一覧ボタン」に Hover 演出を当てない（見た目の誤誘導を避ける）
	if (!m_isDetailMode)
	{
		for (auto id : m_listUIEntities) updateOne(id);
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

	if (!toDetail)
	{
		ReflowUnlockedCardsLayout();
	}

	// 詳細の制御
	for (auto id : m_detailUIEntities)
	{
		SetUIVisible(id, toDetail);
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
	{
		if (!wasDetail)
		{
			BeginDetailAppear();
			m_shootingStarTimer = 0.0f;
			std::uniform_real_distribution<float> dist(m_shootingStarIntervalMin, m_shootingStarIntervalMax);
			m_nextShootingStarWait = dist(m_rng);
			m_spawnStarOnEnterDetail = true;
		}
	}
	else
	{
		if (wasDetail)
		{
			m_spawnStarOnEnterDetail = false;
			KillAllShootingStars();
			if (m_debugStarEntity != (ECS::EntityID)-1)
			{
				m_coordinator->DestroyEntity(m_debugStarEntity);
				m_debugStarEntity = (ECS::EntityID)-1;
			}
		}
	}
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

		m_detailAppearActive = false;
		m_detailAppearTimer = 0.0f;
		m_inputLocked = false;
	}
}



//--------------------------------------------------------------
// Stage Unlock / Reveal（未解放は完全非表示、解放時に浮かび上がる）
//--------------------------------------------------------------
void StageSelectScene::BeginStageReveal(int stageNo)
{
	if (!m_coordinator) return;
	if (stageNo < 1 || stageNo > 6) return;
	const int idx = stageNo - 1;
	if (idx < 0 || idx >= (int)m_listUIEntities.size()) return;

	ECS::EntityID id = m_listUIEntities[idx];
	if (id == (ECS::EntityID)-1) return;

	if (!m_coordinator->HasComponent<TransformComponent>(id) ||
		!m_coordinator->HasComponent<UIImageComponent>(id))
	{
		return;
	}

	auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
	auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);

	StageRevealAnim anim;
	anim.active = true;
	anim.entity = id;
	anim.elapsed = 0.0f;
	anim.duration = 0.90f;
	anim.endY = tr.position.y;
	anim.startY = tr.position.y + 60.0f; // 少し下から
	anim.startAlpha = 0.0f;
	anim.endAlpha = 1.0f;
	anim.baseScale = tr.scale;

	// 初期状態
	tr.position.y = anim.startY;
	ui.isVisible = true;
	ui.color.w = anim.startAlpha;

	// 演出中はクリック不可にする（見えない/半透明で誤クリック防止）
	if (m_coordinator->HasComponent<UIButtonComponent>(id))
	{
		m_coordinator->GetComponent<UIButtonComponent>(id).isVisible = false;
	}

	// RenderComponent 側も念のため同期（UI描画がこちらを参照する実装もあるため）
	if (m_coordinator->HasComponent<RenderComponent>(id))
	{
		m_coordinator->GetComponent<RenderComponent>(id).color.w = anim.startAlpha;
	}

	m_stageReveal[stageNo] = anim;
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
			!m_coordinator->HasComponent<UIImageComponent>(anim.entity))
		{
			anim.active = false;
			continue;
		}

		anim.elapsed += dt;
		float t = (anim.duration > 0.0f) ? (anim.elapsed / anim.duration) : 1.0f;
		if (t > 1.0f) t = 1.0f;
		const float e = SmoothStep01(t);

		auto& tr = m_coordinator->GetComponent<TransformComponent>(anim.entity);
		auto& ui = m_coordinator->GetComponent<UIImageComponent>(anim.entity);

		// 位置：下→定位置
		tr.position.y = anim.startY + (anim.endY - anim.startY) * e;

		// アルファ：0→1
		const float a = anim.startAlpha + (anim.endAlpha - anim.startAlpha) * e;
		ui.color.w = a;

		if (m_coordinator->HasComponent<RenderComponent>(anim.entity))
		{
			m_coordinator->GetComponent<RenderComponent>(anim.entity).color.w = a;
		}

		// 仕上げ：最後にクリック可能化
		if (t >= 1.0f)
		{
			tr.position.y = anim.endY;
			ui.color.w = anim.endAlpha;

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
void StageSelectScene::ReflowUnlockedCardsLayout()
{
	if (!m_coordinator) return;

	const int n = std::max(1, std::min(6, m_maxUnlockedStage));
	const float gapX = 350.0f;
	const float gapY = 250.0f;

	const float centerX = SCREEN_WIDTH * 0.5f;
	const float startY = SCREEN_HEIGHT * 0.32f;

	for (int stageNo = 1; stageNo <= n; ++stageNo)
	{
		const int listIdx = stageNo - 1;
		if (listIdx < 0 || listIdx >= (int)m_listUIEntities.size()) continue;

		// 演出中は位置を上書きしない
		auto itReveal = m_stageReveal.find(stageNo);
		if (itReveal != m_stageReveal.end() && itReveal->second.active)
			continue;

		ECS::EntityID id = m_listUIEntities[listIdx];
		if (id == (ECS::EntityID)-1) continue;
		if (!m_coordinator->HasComponent<TransformComponent>(id)) continue;

		const int visibleIndex = stageNo - 1; // 解放が連番(1..n)である前提
		const int row = visibleIndex / 3;
		const int col = visibleIndex % 3;

		const int rowCount = std::min(3, n - row * 3);
		const float rowWidth = (rowCount <= 1) ? 0.0f : (rowCount - 1) * gapX;
		const float rowStartX = centerX - rowWidth * 0.5f;

		const float x = rowStartX + col * gapX;
		const float y = startY + row * gapY;

		auto& tr = m_coordinator->GetComponent<TransformComponent>(id);
		tr.position.x = x;
		tr.position.y = y;

		// 一覧の基準サイズ（既存の見た目に合わせる）
		tr.position.z = 5.0f;
		tr.rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		tr.scale = DirectX::XMFLOAT3(250.0f, 150.0f, 1.0f);
	}
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

/**
 * [void - PlayUISelectEffect]
 * @brief UIエンティティの中心にワンショットエフェクトを出す
 */
void StageSelectScene::PlayUISelectEffect(ECS::EntityID uiEntity, const std::string& effectId, float scale)
{
	if (!m_coordinator) return;

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

void StageSelectScene::EnsureDebugEffectOnMap()
{
	if (!m_debugShowGlowOnMap) return;
	if (!m_coordinator) return;
	if (!m_isDetailMode) return;
	if (ScreenTransition::IsBusy(m_coordinator.get(), m_transitionEntity) ||
		ScreenTransition::IsBusy(m_coordinator.get(), m_blackTransitionEntity)) return; // 遷移中は見えないので作らない
	if (m_stageMapEntity == (ECS::EntityID)-1) return;
	if (m_debugStarEntity != (ECS::EntityID)-1) return;

	float l, t, r, b;
	if (!GetUIRect(m_stageMapEntity, l, t, r, b)) return;

	const float cx = (l + r) * 0.5f;
	const float cy = (t + b) * 0.5f;

	// ★まず「エフェクト自体が描けるか」を確実に確認するため、動作実績のある EFK_TREASURE_GLOW を常駐表示
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
	const float z = 9.5f; // MAPBACK(10.0) と MAPSIRO(9.0) の間

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

// StageSelectScene.cpp 例
// StageSelectScene.cpp