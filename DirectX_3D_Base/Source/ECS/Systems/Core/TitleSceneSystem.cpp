#include "ECS/Systems/Core/TitleSceneSystem.h"
#include "ECS/Components/Core/TitleSceneComponent.h"
#include "ECS/Components/Core/SoundComponent.h"
#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "Scene/TitleScene.h"
#include "Systems/Input.h" // キー入力用
#include <iostream>
#include <vector>

using namespace DirectX;
using namespace ECS;

void TitleSceneSystem::Update(float deltaTime)
{
	// 管理用エンティティ（TitleSceneComponentを持つやつ）を処理
	for (auto const& entity : m_entities)
	{
		auto& sceneComp = m_coordinator->GetComponent<TitleSceneComponent>(entity);

		// ====================================================
		// 1. 生成ロジック (IDが無効なら作る)
		// ====================================================
		if (sceneComp.TitleBackGroundEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.TitleBackGroundEntity = m_coordinator->CreateEntity(

				TagComponent(
					/* Tag	*/	"BackGround"
				),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
				),
				RenderComponent(
					/* MeshType	*/	MESH_MODEL,
					/* Color	*/	XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
				),
				ModelComponent(
					/* Path		*/	"M_TITLE_BACKGROUND",
					/* Scale	*/	0.1f,
					/* Flip		*/	Model::None
				)
			);
			std::cout << "Title BackGround Entity Created! ID:" << sceneComp.ButtonTitleStartEntity << std::endl;

		}

		// ロゴ
		if (sceneComp.TitleLogoEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.TitleLogoEntity = m_coordinator->CreateEntity(
				TagComponent("TitleLogo"),
				TransformComponent(XMFLOAT3(0.0f, 0.4f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.8f, 0.4f, 1.0f)),
				UIImageComponent("UI_TITLE_LOGO")
			);
		}

		// スタートボタンエンティティの生成
			if (sceneComp.ButtonTitleStartEntity == ECS::INVALID_ENTITY_ID)
			{
				float startBtnScaleX = 0.5f; // 設定したい幅
				float startBtnScaleY = 0.2f; // 設定したい高さ

				sceneComp.ButtonTitleStartEntity = m_coordinator->CreateEntity(
					TagComponent("StartButton"),
					TransformComponent(
						XMFLOAT3(0.0f, -0.2f, 0.0f),
						XMFLOAT3(0.0f, 0.0f, 0.0f),
						XMFLOAT3(startBtnScaleX, startBtnScaleY, 1.0f)
					),
					UIInteractableComponent(-1.0f, -1.0f),
					UIImageComponent("UI_TEST1")
				);

				// ★★★ 追加：baseScaleの設定 ★★★
				// これを忘れると、マウスホバー時にサイズが 1.0 に戻って巨大化します
				auto& interact = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonTitleStartEntity);
				interact.baseScaleX = startBtnScaleX;
				interact.baseScaleY = startBtnScaleY;

				std::cout << "Title StartButton Entity Created! ID:" << sceneComp.ButtonTitleStartEntity << std::endl;
			}

		// 終了ボタンエンティティの生成
		if (sceneComp.ButtonTitleExitEntity == ECS::INVALID_ENTITY_ID)
		{
			// スケール値を変数にしておくとミスが減ります
			float exitScaleX = 0.8f;
			float exitScaleY = 0.2f;

			sceneComp.ButtonTitleExitEntity = m_coordinator->CreateEntity(
				TagComponent("exitbutton"),
				TransformComponent(
					XMFLOAT3(0.0f, -0.65f, 0.0f),
					XMFLOAT3(0.0f, 0.0f, 0.0f),
					XMFLOAT3(exitScaleX, exitScaleY, 1.0f)
				),
				UIInteractableComponent(exitScaleX, exitScaleY),
				UIImageComponent("UI_TEST2")
			);

			// ★★★ 追加：baseScaleの設定 ★★★
			{
				auto& interact = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonTitleExitEntity);
				interact.baseScaleX = exitScaleX;
				interact.baseScaleY = exitScaleY;
			}

			std::cout << "Title ExitButton Entity Created! ID:" << sceneComp.ButtonTitleExitEntity << std::endl;
		}

		// ====================================================
		// 2. 入力・遷移ロジック (生成済みなら判定)
		// ====================================================

		// スタートボタンクリック判定
		if (sceneComp.ButtonTitleStartEntity != ECS::INVALID_ENTITY_ID)
		{
			const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonTitleStartEntity);
			if (comp.isClicked)
			{
				SceneManager::ChangeScene<StageSelectScene>();
				return; // シーン遷移したら抜ける
			}
		}

		// 終了ボタンクリック判定
		if (sceneComp.ButtonTitleExitEntity != ECS::INVALID_ENTITY_ID)
		{
			const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonTitleExitEntity);
			if (comp.isClicked)
			{
				PostQuitMessage(0);
				return;
			}
		}

	}
}