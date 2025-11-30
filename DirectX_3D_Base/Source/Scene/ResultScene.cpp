/*****************************************************************//**
 * @file	ResultScene.cpp
 * @brief
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author
 * ------------------------------------------------------------
 *
 * @date	2025/11/08	初回作成日
 * 			作業内容：	-
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード  =====
#include "Scene/ResultScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIInteractableComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include <ECS/Systems/Core/ResultSceneSystem.h>

using namespace DirectX;
using namespace std;

//仮の入力チェック関数
bool IsInputTitle() { return false; }
bool ResultScene::isClear = false;
int ResultScene::finalItenCount = 0;


//===== ResultScene メンバー関数の実装 =====

void ResultScene::Init()
{

	m_coordinator = std::make_shared<ECS::Coordinator>();

	ECS::ECSInitializer::InitECS(m_coordinator);



	

	if (ResultScene::isClear)
	{
		// リザルト成功ロゴ
		ECS::EntityID ResultClearLogo = m_coordinator->CreateEntity(
			TagComponent(
				/* Tag	*/	"ResultClearLogo"
			),
			TransformComponent(
				/* Position	*/	XMFLOAT3(0.0f, 0.8f, 0.0f),
				/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(0.8f, 0.2f, 1.0f)
			),

			UIImageComponent(

				"UI_GAMEOVER"

			)
		);
	}
	
	if (ResultScene::isClear == false)
	{
		// リザルト失敗ロゴ
		ECS::EntityID ResultOutLogo = m_coordinator->CreateEntity(
			TagComponent(
				/* Tag	*/	"ResultOutLogo"
			),
			TransformComponent(
				/* Position	*/	XMFLOAT3(0.0f, 0.8f, 0.0f),
				/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(0.8f, 0.2f, 1.0f)
			),

			UIImageComponent(

				"UI_GAMEOVER"

			)
		);
	}

	const std::vector<std::string> onIDs = { "UI_TEST1", "UI_TEST2","UI_TEST3" };
	const std::vector<std::string> offIDs = { "UI_TEST1_OFF", "UI_TEST2_OFF", "UI_TEST3_OFF" };

	// 3つの宝箱スロットについてループ処理
	for (int i = 0; i < 3; ++i)
	{
		std::string assetID;

		// 現在のインデックス(i)が獲得数未満なら「獲得済み(ON)」、それ以外は「未獲得(OFF)」


	}

	// クリア内容詳細
	ECS::EntityID ResultTime = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultTime"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, 0.3f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.1f, 0.6f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST3"

		)
	);

	

	// 獲得した宝1
	ECS::EntityID ResultTakara1 = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultTakara1"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(-0.35f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.3f, 0.3f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST1"

		)
	);

	// 獲得した宝2
	ECS::EntityID ResultTakara2 = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultTakara2"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.3f, 0.3f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST2"

		)
	);

	// 獲得した宝3
	ECS::EntityID ResultTakara3 = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultTakara3"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.35f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.3f, 0.3f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST3"

		)
	);

	// 未獲得の宝1
	ECS::EntityID ResultNotTakara1 = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultNotTakara1"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(-0.35f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.3f, 0.3f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST1_OFF"

		)
	);

	// 未獲得の宝2
	ECS::EntityID ResultNotTakara2 = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultNotTakara2"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.3f, 0.3f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST2_OFF"

		)
	);

	// 未獲得の宝3
	ECS::EntityID ResultNotTakara3 = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultNotTakara3"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.35f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.3f, 0.3f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST3_OFF"

		)
	);

	// 獲得した宝背景
	ECS::EntityID ResultTakara = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultTakara"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, -0.4f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.1f, 0.6f, 1.0f)
		),

		UIImageComponent(

			"UI_TEST5"

		)
	);


	


	// ステージセレクトボタン
	ECS::EntityID ResultSelect = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultSelect"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.3f, -0.85f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.2f, 0.2f, 1.0f)
		),

		UIInteractableComponent(-1.0f, -1.0f),

		UIImageComponent(

			"UI_RESULT_SELECT"

		)

		
	);


	

	// リトライボタン
	ECS::EntityID ResultRetry = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultRetry"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.55f, -0.85f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.2f, 0.2f, 1.0f)
		),

		UIInteractableComponent(-1.0f, -1.0f),

		UIImageComponent(

			"UI_RESULT_RETORY"

		)
	);
	

	// タイトルボタン
	ECS::EntityID ResultTitle = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultTitle"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.8f, -0.85f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(0.2f, 0.2f, 1.0f)
		),

		UIInteractableComponent(-1.0f, -1.0f),

		UIImageComponent(

			"UI_RESULT_TITLE"

		)
	);

	// シーン切り替えボタン背景
	ECS::EntityID btonresultnormal = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"btnresultnormal"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.5f, -0.85f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 0.3f, 1.0f)
		),

		UIInteractableComponent(-1.0f, -1.0f),

		UIImageComponent(

			"UI_BTN_RESULT_NORMAL"

		)


	);
	

	// リザルトシーン背景
	ECS::EntityID ResultSceneBuckground = m_coordinator->CreateEntity(
		TagComponent(
			/* Tag	*/	"ResultSceneBuckground"
		),
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(2.0f, 2.0f, 1.0f)
		),

		UIImageComponent(

			"UI_RESULTSCENE_BACKGROUND"

		)
	);

	

	
	
	//m_coordinator = std::make_shared<ECS::Coordinator>();

	//ECS::ECSInitializer::InitECS(m_coordinator);


	//ECS::EntityFactory::CreateResultUIEntities(ECS::ECSInitializer::GetCoordinator());
	std::cout << "ResultScene::Init() - ResultUISystem Ready." << std::endl;
}

void ResultScene::Uninit()
{
	//このシーンで作成したエンティティを破棄
	//ECS::ECSInitializer::GetCoordinator()->DestoryEntities(m_sceneEntities);
	//m_buttons.clear();
	std::cout << "ResultScene::Uninit() - Result Scene Systems Destroyed." << std::endl;
}

void ResultScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);

	/*ECS::EntityID interactableEntity = ECS::FindFirstEntityWithComponent<UIInteractableComponent>(m_coordinator.get());*/


	/*auto uiInput = ECS::ECSInitializer::GetSystem<UIInputSystem>();
	if (uiInput)
	{
		uiInput->Update(deltaTime);
	}*/

	// ボタンのクリック判定
	



	
	
}

void ResultScene::Draw()
{
	//RenderSystemは常に存在すると仮定し、Draw処理は共有する
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();	//UIエンティティの描画
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
	
}