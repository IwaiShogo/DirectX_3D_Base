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
#include "Scene/StageSelectScene.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include "Scene/StageinformationScene.h"
#include "Systems/Input.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/UI/CursorSystem.h"
#include "ECS/Systems/Core/AudioSystem.h"
#include "ECS/Components/UI/ZoomTransitionComponent.h"
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Components/Core/SoundComponent.h>
#include "ECS/Components/UI/UIAnimationComponent.h"

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

	// 変数初期化（不要なメンバ変数は削除した前提ですが、もし残っていれば初期化してください）
	// m_isTransitioning = false;

	// 2. System登録
	{
		auto system = m_coordinator->RegisterSystem<CursorSystem>();
		Signature signature;
		signature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
		signature.set(m_coordinator->GetComponentTypeID<TagComponent>());
		m_coordinator->SetSystemSignature<CursorSystem>(signature);
		system->Init(m_coordinator.get());
	}

	// 3. 背景
	m_selectbg = m_coordinator->CreateEntity(
		TagComponent("SelectSceneUIBG"),
		TransformComponent(XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 2.0f, 1.0f)),
		UIImageComponent("UI_BG")
	);
	m_selectcork = m_coordinator->CreateEntity(
		TagComponent("SelectSceneUICORK"),
		TransformComponent(XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.9f, 1.9f, 1.0f)),
		UIImageComponent("UI_CORK")
	);

	// 4. ボタン作成
	// ★修正箇所：DirectX:: を削除し、シンプルな記述に戻しました
	auto CreateButton = [&](EntityID& entity, std::string tag, std::string img, float x, float y, float sx, float sy, float delay) {
		entity = m_coordinator->CreateEntity(
			TagComponent(tag),
			TransformComponent(XMFLOAT3(x, y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
			UIInteractableComponent(-1.0f, -1.0f, true),
			UIImageComponent(img),
			UIAnimationComponent{
				UIAnimationComponent::AnimType::Scale,
				XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT3(sx, sy, 1.0f),
				delay,
				0.5f
			},
			ZoomTransitionComponent{
				false,   // isActive
				150.0f,  // speed
				20.0f,   // threshold
				TransitionTarget::ToInfo, // 行き先
				0,       // ステージNo (後で設定)
				false    // isFinished
			}
		);
		// 強制上書き
		m_coordinator->GetComponent<TransformComponent>(entity).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

		auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(entity);
		comp.baseScaleX = sx; comp.baseScaleY = sy;
		};

	CreateButton(m_selectEntity1, "SelectSceneUI1", "UI_PAPER_1", -0.6f, 0.45f, 0.7f, 0.7f, 0.0f);
	CreateButton(m_selectEntity2, "SelectSceneUI2", "UI_PAPER_2", 0.0f, 0.6f, 0.4f, 0.3f, 0.1f);
	CreateButton(m_selectEntity3, "SelectSceneUI3", "UI_PAPER_3", 0.0f, 0.0f, 0.5f, 0.35f, 0.2f);
	CreateButton(m_selectEntity4, "SelectSceneUI4", "UI_PAPER_1", 0.6f, 0.4f, 0.55f, 0.7f, 0.3f);
	CreateButton(m_selectEntity5, "SelectSceneUI5", "UI_PAPER_2", -0.55f, -0.45f, 0.6f, 0.6f, 0.4f);
	CreateButton(m_selectEntity6, "SelectSceneUI6", "UI_PAPER_3", 0.5f, -0.5f, 0.8f, 0.8f, 0.5f);

	// 5. カーソル
	m_cursorEntity = m_coordinator->CreateEntity(
		TagComponent("Cursor"),
		TransformComponent(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.1f, 1.0f)),
		UIImageComponent("UI_MUSIMEGANE")
	);

	// 6. A/Bボタン
	{
		m_selectA = m_coordinator->CreateEntity(
			TagComponent("SelectSceneUIA"),
			TransformComponent(XMFLOAT3(0.4f, -0.9f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
			UIImageComponent("UI_TEST2"),
			UIAnimationComponent{
				UIAnimationComponent::AnimType::Scale,
				XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT3(0.3f, 0.1f, 1.0f),
				0.8f,
				0.5f
			}
			// ※A/Bボタンにはズーム遷移(ZoomTransitionComponent)は不要なので付けません
		);
		m_coordinator->GetComponent<TransformComponent>(m_selectA).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);

		m_selectB = m_coordinator->CreateEntity(
			TagComponent("SelectSceneUIB"),
			TransformComponent(XMFLOAT3(0.8f, -0.9f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
			UIImageComponent("UI_TEST2"),
			UIAnimationComponent{
				UIAnimationComponent::AnimType::Scale,
				XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT3(0.3f, 0.1f, 1.0f),
				0.9f,
				0.5f
			}
		);
		m_coordinator->GetComponent<TransformComponent>(m_selectB).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	// BGM再生
	EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST", 0.3f);

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

<<<<<<< HEAD
	if (m_selectEntity1 != ECS::INVALID_ENTITY_ID)
	{
		const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(m_selectEntity1);
		if (comp.isClicked)
=======
	if (IsKeyTrigger('Q'))
	{
		// 音を鳴らす（必要であれば）
		// ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_CANCEL", 0.5f);

		SceneManager::ChangeScene<TitleScene>();
		return; // シーンが変わるので、これ以降の処理はスキップ
	}

	// 2. クリック判定
	auto CheckClickAndStartTransition = [&](EntityID entity, int stageNo)
>>>>>>> 86ca950fec7521f1906ef5f5fc2c83a833b2ea35
		{
			if (entity != INVALID_ENTITY_ID)
			{
				if (m_coordinator->HasComponent<UIInteractableComponent>(entity))
				{
					const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(entity);
					if (comp.isClicked)
					{
						EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST", 0.5f);

						auto& zoom = m_coordinator->GetComponent<ZoomTransitionComponent>(entity);
						zoom.isActive = true;
						zoom.targetStageNo = stageNo;
						zoom.nextScene = TransitionTarget::ToInfo;

						// UIInputSystemの干渉を防ぐためコンポーネント削除
						m_coordinator->RemoveComponent<UIInteractableComponent>(entity);
					}
				}
			}
		};

	CheckClickAndStartTransition(m_selectEntity1, 1);
	CheckClickAndStartTransition(m_selectEntity2, 2);
	CheckClickAndStartTransition(m_selectEntity3, 3);
	CheckClickAndStartTransition(m_selectEntity4, 4);
	CheckClickAndStartTransition(m_selectEntity5, 5);
	CheckClickAndStartTransition(m_selectEntity6, 6);
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