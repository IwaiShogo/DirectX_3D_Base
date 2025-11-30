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
#include "ECS/Systems/UI/CursorSystem.h"
#include "ECS/Systems/Core/AudioSystem.h"
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Components/Core/SoundComponent.h>

#include "ECS/Components/UI/UIAnimationComponent.h"


#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
#include <string>
#include <iomanip> // 桁数合わせ用

// マップの初期X座標（画面左外）と目標X座標
const float MAP_START_X = -2.5f;
const float MAP_END_X = -0.4f;

// ボタンの初期Y座標（画面下外）と目標Y座標
const float BTN_START_Y = -2.0f;
const float OK_END_Y = -0.55f;
const float BACK_END_Y = -0.85f;

// ===== 静的メンバー変数の定義 =====
ECS::Coordinator* StageinformationScene::s_coordinator = nullptr;
using namespace DirectX;

void StageinformationScene::Init()
{
	// 1. ECS初期化
	m_coordinator = std::make_unique<ECS::Coordinator>();
	s_coordinator = m_coordinator.get();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// 変数初期化
	m_isSceneChanging = false;
	m_nextScene = NextScene::NONE;

	// 2. システム登録
	{
		auto system = m_coordinator->RegisterSystem<CursorSystem>();
		ECS::Signature signature;
		signature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
		signature.set(m_coordinator->GetComponentTypeID<TagComponent>());
		m_coordinator->SetSystemSignature<CursorSystem>(signature);
		system->Init(m_coordinator.get());
	}

	// 3. UI作成
	// 背景 (UI_BG)
	m_bg = m_coordinator->CreateEntity(
		TagComponent("InformationBG"),
		TransformComponent(XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 2.0f, 1.0f)),
		UIImageComponent("UI_BG")
	);

	m_selectcork = m_coordinator->CreateEntity(
		TagComponent("SelectSceneUICORK"),
		TransformComponent(XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.9f, 1.9f, 1.0f)),
		UIImageComponent("UI_CORK")
	);

	// マップ画像 (移動アニメーションを追加)
	m_MapImage = m_coordinator->CreateEntity(
		TagComponent("SelectSceneUIMapImage"),
		TransformComponent(XMFLOAT3(MAP_END_X, 0.25f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.9f, 1.2f, 1.0f)),
		UIImageComponent("UI_STAGEMAP"),
		UIAnimationComponent{
			UIAnimationComponent::AnimType::Scale,
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.9f, 1.2f, 1.0f),
			0.0f,
			0.5f
		}
	);
	m_coordinator->GetComponent<TransformComponent>(m_MapImage).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
	// トレジャーアイコン (拡大アニメーション)
	m_Treasure = m_coordinator->CreateEntity(
		TagComponent("SelectSceneUITREASURE"),
		TransformComponent(XMFLOAT3(0.4f, 0.65f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)), // Scale 0
		UIImageComponent("UI_PAPER_1"), // 仮画像
		UIAnimationComponent{
		UIAnimationComponent::AnimType::Scale,
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.6f, 0.55f, 1.0f),
			0.4f, // 0.4秒待つ
			0.4f
		}
	);
	m_coordinator->GetComponent<TransformComponent>(m_MapImage).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

	// 警備員アイコン (拡大アニメーション)
	m_Security = m_coordinator->CreateEntity(
		TagComponent("SelectSceneUISECURITY"),
		TransformComponent(XMFLOAT3(0.4f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)), // Scale 0
		UIImageComponent("UI_PAPER_2"), // 仮画像
		UIAnimationComponent{
			UIAnimationComponent::AnimType::Scale,
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.6f, 0.55f, 1.0f),
			0.4f,
			0.4f
		}
	);
	m_coordinator->GetComponent<TransformComponent>(m_Treasure).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

	// OKボタン (移動アニメーション)
	{
		float sx = 0.6f, sy = 0.2f;
		m_OK = m_coordinator->CreateEntity(
			TagComponent("SelectSceneUIOK"),
			TransformComponent(XMFLOAT3(0.4f, OK_END_Y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
			UIInteractableComponent(-1.0f, -1.0f, true),
			UIImageComponent("UI_PAPER_1"),
			UIAnimationComponent{
				UIAnimationComponent::AnimType::Scale,
				XMFLOAT3(0.0f, 0.0f, 0.0f),    // 開始サイズ
				XMFLOAT3(sx, sy, 1.0f),        // 終了サイズ
				0.8f,
				0.5f
			}
		);
		auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_OK);
		comp.baseScaleX = sx; comp.baseScaleY = sy;
	}
	m_coordinator->GetComponent<TransformComponent>(m_Security).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

	// BACKボタン (移動アニメーション)
	{
		float sx = 0.6f, sy = 0.2f;
		m_BACK = m_coordinator->CreateEntity(
			TagComponent("SelectSceneUIBACK"),
			TransformComponent(XMFLOAT3(0.4f, BACK_END_Y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
			UIInteractableComponent(-1.0f, -1.0f, true),
			UIImageComponent("UI_PAPER_2"),
			UIAnimationComponent{
				UIAnimationComponent::AnimType::Scale,
				XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT3(sx, sy, 1.0f),
				0.8f,
				0.5f
			}
		);;
		auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_BACK);
		comp.baseScaleX = sx; comp.baseScaleY = sy;
	}
	m_coordinator->GetComponent<TransformComponent>(m_Security).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_cursorEntity = m_coordinator->CreateEntity(
		TagComponent("Cursor"),
		TransformComponent(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.1f, 1.0f)),
		UIImageComponent("UI_MUSIMEGANE")
	);

	// 数字 (拡大アニメーション)
	float startX = -0.7f;
	float startY = -0.7f;
	float stepX = 0.1f;

	for (int i = 0; i < 5; ++i)
	{
		std::string initAsset = (i == 2) ? "UI_COLON" : "UI_NUM_0";
		m_timeDigits[i] = m_coordinator->CreateEntity(
			TagComponent("TimeDigit" + std::to_string(i)),
			TransformComponent(XMFLOAT3(startX + (i * stepX), startY, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)), // Scale 0
			UIImageComponent(initAsset),
			UIAnimationComponent{
				UIAnimationComponent::AnimType::Scale,
				XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT3(0.1f, 0.2f, 1.0f),
				0.4f,
				0.4f
			}
		);
	}

	float bestTime = ScoreManager::GetBestTime(GameScene::s_StageNo);
	UpdateTimeDisplay(bestTime);

	// BGM再生
	ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST2", 0.3f);
}

void StageinformationScene::Uninit()
{
	ECS::ECSInitializer::UninitECS();
	m_coordinator.reset();
	s_coordinator = nullptr;
}

void StageinformationScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);

	if (m_isSceneChanging)
	{
		// ここがないと、ボタンを押してもフラグが立つだけで画面が切り替わりません
		if (m_nextScene == NextScene::GAME) {
			SceneManager::ChangeScene<GameScene>();
		}
		else if (m_nextScene == NextScene::SELECT) {
			SceneManager::ChangeScene<StageSelectScene>();
		}
		return;
	}

	// アニメーションが終わるまで入力をブロックする簡易タイマー
	// ※Systemが勝手に動かしてくれるので、ここでは「待つ」判定だけで良い
	static float sceneTimer = 0.0f;
	// 【注意】シーン再訪時にリセットされないので、本来はメンバ変数を使うべきですが、
	// アニメーション自体はSystemのタイマーで動くので、ここは「操作不能時間」の管理だけです。
	sceneTimer += deltaTime;

	// アニメーション時間(最大1.3秒)が終わるまでクリック無効
	// if (sceneTimer < 1.3f) return; // これを入れるとアニメーション中操作できなくなります

	if (m_OK != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_OK);
		if (comp.isClicked)
		{
			ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST2", 0.5f);
			m_isSceneChanging = true;
			m_nextScene = NextScene::GAME;
		}
	}

	if (m_BACK != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_BACK);
		if (comp.isClicked)
		{
			ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST3", 0.5f);
			m_isSceneChanging = true;
			m_nextScene = NextScene::SELECT;
		}
	}
}
void StageinformationScene::Draw()
{
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>()) {
		system->DrawSetup();
		system->DrawEntities();
	}
	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>()) {
		system->Render();
	}
}

void StageinformationScene::UpdateTimeDisplay(float time)
{
	int minutes = 0;
	int seconds = 0;

	if (time > 0.0f)
	{
		minutes = static_cast<int>(time) / 60;
		seconds = static_cast<int>(time) % 60;
	}

	int m10 = minutes / 10;
	int m1 = minutes % 10;
	int s10 = seconds / 10;
	int s1 = seconds % 10;

	if (m_timeDigits[0] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[0]).assetID = "UI_NUM_" + std::to_string(m10);
	if (m_timeDigits[1] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[1]).assetID = "UI_NUM_" + std::to_string(m1);
	if (m_timeDigits[3] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[3]).assetID = "UI_NUM_" + std::to_string(s10);
	if (m_timeDigits[4] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_timeDigits[4]).assetID = "UI_NUM_" + std::to_string(s1);
}