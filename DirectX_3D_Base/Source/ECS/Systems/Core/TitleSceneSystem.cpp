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
#include "Scene/SceneManager.h"
#include <iostream>
#include <vector>

using namespace DirectX;
using namespace ECS;

void TitleSceneSystem::Update(float deltaTime)
{
	//TitleSceneComponentを持つエンティティを探す
	for (auto const& entity : m_entities)
	{
		//SceneComponentの取得
		auto& sceneComp = m_coordinator->GetComponent<TitleSceneComponent>(entity);

		// ---初回エンティティ生成ロジック
		//IDが未初期化の場合のみ作成
		
		// -------------------------- //
		// --- エンティティの生成 --- //
		// -------------------------- //

		//タイトルバックグラウンドエンティティの生成	//???
		if (sceneComp.TitleBackGroundEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.TitleBackGroundEntity = m_coordinator->CreateEntity(
				TagComponent(
					/* Tag	*/	"TitleBackGround"
				),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
				),
				RenderComponent(
					/* MeshType	*/	MESH_MODEL,
					/* Color	*/	XMFLOAT4(0.3f, 0.3f, 1.0f, 1.0f)
				),
				ModelComponent(
					/* Path		*/	"Titlekari",
					/* Scale	*/	0.1f,
					/* Flip		*/	Model::None
				)
			);
			std::cout << "Title BackGround Entity Created! ID:" << sceneComp.TitleBackGroundEntity << std::endl;
		}


		//	スタートボタンエンティティの生成
		if (sceneComp.ButtonTitleStartEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.ButtonTitleStartEntity = m_coordinator->CreateEntity(
				TagComponent(/* Tag	*/	"startbutton"),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, -0.4f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(0.8f, 0.2f, 1.0f)
				),
				UIInteractableComponent(0.8f, 0.2f),
				UIImageComponent(
					"UI_TEST1"
				)
			);
		std::cout << "Title StartButton Entity Created! ID:" <<sceneComp.ButtonTitleStartEntity << std::endl;
		}

		//	終了ボタンエンティティの生成
		if (sceneComp.ButtonTitleExitEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.ButtonTitleExitEntity = m_coordinator->CreateEntity(
				TagComponent(/* Tag	*/	"exitbutton"),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, -0.65f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(0.8f, 0.2f, 1.0f)
				),
				UIInteractableComponent(0.8f, 0.2f),
				UIImageComponent(
					"UI_TEST2"
				)
			);
			std::cout << "Title ExitButton Entity Created! ID:" << sceneComp.ButtonTitleExitEntity << std::endl;
		}

		//タイトルロゴエンティティの作成
		if (sceneComp.TitleLogoEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.TitleLogoEntity = m_coordinator->CreateEntity(
				TagComponent(/* Tag	*/	"logo"),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, 0.6f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(1.0f, 0.7f, 1.0f)
				),
				UIImageComponent("UI_TITLE_LOGO")
			);
			std::cout << "Title Logo Entity Created! ID:" << sceneComp.TitleLogoEntity << std::endl;
		}
		
		//はじめからボタンエンティティの作成
		if (sceneComp.Button_NewGameEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.Button_NewGameEntity = m_coordinator->CreateEntity(
				TagComponent(/* Tag	*/	"newgame"),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, -0.65f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(0.8f, 0.2f, 1.0f)
				),
				UIInteractableComponent(0.8f, 0.2f),
				UIImageComponent(
					"UI_TITLE_HAZIMEKARA"
				)
			);
			std::cout << "Title NewGameButton Entity Created! ID:" << sceneComp.ButtonTitleExitEntity << std::endl;
		}

		//続きからボタンエンティティの生成
		ECS::EntityID Button_ContinueEntity = ECS::INVALID_ENTITY_ID;	//　続きから	//未実装


		if (sceneComp.Button_ContinueEntity == ECS::INVALID_ENTITY_ID)
		{
			sceneComp.Button_ContinueEntity = m_coordinator->CreateEntity(
				TagComponent(/* Tag	*/	"continue"),
				TransformComponent(
					/* Position	*/	XMFLOAT3(0.0f, -0.65f, 0.0f),
					/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
					/* Scale	*/	XMFLOAT3(0.8f, 0.2f, 1.0f)
				),
				UIInteractableComponent(0.8f, 0.2f),
				UIImageComponent(
					"UI_TEST4"
				)
			);
			std::cout << "Title ContinueButton Entity Created! ID:" << sceneComp.ButtonTitleExitEntity << std::endl;
		}

		// --------------------------------------- //
		// --- ボタンの表示/非表示制御ロジック --- //
		// --------------------------------------- //
		if (sceneComp.ButtonTitleStartEntity != ECS::INVALID_ENTITY_ID)
		{
			// TransformComponentを非const参照で取得し、位置を操作します
			auto& startTransform = m_coordinator->GetComponent<TransformComponent>(sceneComp.ButtonTitleStartEntity);
			auto& exitTransform = m_coordinator->GetComponent<TransformComponent>(sceneComp.ButtonTitleExitEntity);
			auto& newGameTransform = m_coordinator->GetComponent<TransformComponent>(sceneComp.Button_NewGameEntity);
			auto& continueTransform = m_coordinator->GetComponent<TransformComponent>(sceneComp.Button_ContinueEntity);

			// 画面外座標 (非表示用)
			const XMFLOAT3 HIDDEN_POS = XMFLOAT3(100.0f, 100.0f, 0.0f);
			// 画面内座標 (表示用)
			const XMFLOAT3 START_POS = XMFLOAT3(0.0f, -0.4f, 0.0f);
			const XMFLOAT3 EXIT_POS = XMFLOAT3(0.0f, -0.65f, 0.0f);
			const XMFLOAT3 NEWGAME_POS = XMFLOAT3(0.0f, -0.4f, 0.0f);  // NewGameはStartと同じ位置
			const XMFLOAT3 CONTINUE_POS = XMFLOAT3(0.0f, -0.65f, 0.0f); // ContinueはExitと同じ位置

			if (sceneComp.showGameStartOptions)
			{
				// showGameStartOptions が true の場合: New Game / Continue を表示
				newGameTransform.position = NEWGAME_POS;
				continueTransform.position = CONTINUE_POS;

				// Start / Exit を非表示
				startTransform.position = HIDDEN_POS;
				exitTransform.position = HIDDEN_POS;
			}
			else
			{
				// showGameStartOptions が false の場合: Start / Exit を表示
				startTransform.position = START_POS;
				exitTransform.position = EXIT_POS;

				// New Game / Continue を非表示
				newGameTransform.position = HIDDEN_POS;
				continueTransform.position = HIDDEN_POS;
			}
		}



		//スタートボタン判定ロジック
		if (sceneComp.ButtonTitleStartEntity != ECS::INVALID_ENTITY_ID)
		{
			//該当エンティティがUIInteratbaleComponentを持つか確認(安全のため)
			if (m_coordinator->m_entityManager->GetSignature(sceneComp.ButtonTitleStartEntity).test(
				m_coordinator->GetComponentTypeID<UIInteractableComponent>()))
			{
				const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonTitleStartEntity);

				if (comp.isClicked)
				{
					std::cout << "Button Clicked! -> GameScene" << std::endl;
					//SceneManager::ChangeScene<GameScene>();

					//ボタンの表示を切り替え
					sceneComp.showGameStartOptions = true;
				}
			}
		}

		//終了ボタン判定ロジック
		if (sceneComp.ButtonTitleExitEntity != ECS::INVALID_ENTITY_ID)
		{
			//該当エンティティがUIInteratbaleComponentを持つか確認(安全のため)
			if (m_coordinator->m_entityManager->GetSignature(sceneComp.ButtonTitleExitEntity).test(
				m_coordinator->GetComponentTypeID<UIInteractableComponent>()))
			{
				const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonTitleExitEntity);

				if (comp.isClicked)
				{//終了処理

				}
			}
		}
		
		//はじめからボタン判定ロジック
		if (sceneComp.Button_NewGameEntity != ECS::INVALID_ENTITY_ID)
		{
			//該当エンティティがUIInteratbaleComponentを持つか確認(安全のため)
			if (m_coordinator->m_entityManager->GetSignature(sceneComp.Button_NewGameEntity).test(
				m_coordinator->GetComponentTypeID<UIInteractableComponent>()))
			{
				const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.Button_NewGameEntity);

				if (comp.isClicked)
				{
					std::cout << "Button Clicked! -> GameScene" << std::endl;
					SceneManager::ChangeScene<StageSelectScene>();
				}
			}
		}

	}

	//EnterまたはコントローラーのAを押したらシーンを切り替え
	if (IsKeyTrigger(VK_RETURN) || IsButtonTriggered(BUTTON_A))
	{
		SceneManager::ChangeScene<StageSelectScene>();
	}
	
	
	if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_B))
	{//しゅうりょうしょり

	}

}
