/*****************************************************************//**
 * @file	TitleScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/30	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "Scene/TitleScene.h"
#include "Scene/StageSelectScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIButtonComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Core/TitleControlSystem.h"


using namespace DirectX;

//仮の入力チェック関数
static bool IsInputStart() {
	return false;
}

void TitleScene::Init()
{
	m_coordinator = std::make_shared<ECS::Coordinator>();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// コントローラー
	TitleControllerComponent titleCtrl;
	titleCtrl.camStartPos = XMFLOAT3(0.0f, 2.0f, -14.3f);
	titleCtrl.camEndPos = XMFLOAT3(0.0f, 1.6f, -5.0f);

	// 固定カメラ
	ECS::EntityID cam = ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), titleCtrl.camStartPos);
	titleCtrl.cameraEntityID = cam;

	// 背景モデル
	ECS::EntityID museum = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_MODEL, // MESH_BOXで仮描画
			/* Color	*/	XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
		),
		ModelComponent(
			/* Path		*/	"M_TITLE_MUSEUM",
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::ZFlip
		)
	);

	// UI作成
	{
		ECS::EntityID logo = m_coordinator->CreateEntity(
			TransformComponent(
				/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.3f, 0.0f),
				/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(550, 410, 1)
			),
			UIImageComponent(
				/* AssetID		*/	"UI_TITLE_LOGO",
				/* Depth		*/	0.0f,
				/* IsVisible	*/	true
			)
		);
		titleCtrl.pressStartUIEntities.push_back(logo);

		ECS::EntityID ent = m_coordinator->CreateEntity(
			TransformComponent(
				/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.8f, 0.0f),
				/* Rotation	*/	XMFLOAT3(1.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(400, 50, 1)
			),
			UIImageComponent(
				/* AssetID		*/	"UI_PRESS_START",
				/* Depth		*/	0.0f,
				/* IsVisible	*/	true
			)
		);
		titleCtrl.pressStartUIEntities.push_back(ent);
	}

	// メニューUI
	{
		// New Game
		ECS::EntityID newGame = m_coordinator->CreateEntity(
			TransformComponent(
				/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.6f, 0.0f),
				/* Rotation	*/	XMFLOAT3(3.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(300, 80, 1)
			),
			UIImageComponent(
				/* AssetID		*/	"BTN_NEW_GAME",
				/* Depth		*/	0.0f,
				/* IsVisible	*/	false
			),
			UIButtonComponent(
				/* State		*/	ButtonState::Normal,
				/* IsVisible	*/	false,
				/* OnClick		*/	[]() { SceneManager::ChangeScene<StageSelectScene>(); }
			)
		);

		// Continue
		ECS::EntityID cont = m_coordinator->CreateEntity(
			TransformComponent(
				/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.75f, 0.0f),
				/* Rotation	*/	XMFLOAT3(3.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(300, 80, 1)
			),
			UIImageComponent(
				/* AssetID	*/	"BTN_CONTINUE",
				/* Depth		*/	0.0f,
				/* IsVisible	*/	false
			),
			UIButtonComponent(
				/* State		*/	ButtonState::Normal,
				/* IsVisible	*/	false,
				/* OnClick		*/	[]() { SceneManager::ChangeScene<StageSelectScene>(); }
			)
		);

		// 登録
		titleCtrl.menuUIEntities.push_back(newGame);
		titleCtrl.menuUIEntities.push_back(cont);
	}

	// カーソル
	{
		m_coordinator->CreateEntity(
			TransformComponent(
				/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
				/* Scale	*/	XMFLOAT3(64.0f, 64.0f, 1.0f)
			),
			UIImageComponent(
				/* AssetID	*/	"ICO_CURSOR",
				/* Depth	*/	1.0f
			),
			UICursorComponent()
		);
	}

	// --- 4. デモ用Entityの作成 ---	
	//ECS::EntityFactory::CreateTitleSceneEntity(m_coordinator.get());
	ECS::EntityID controller = m_coordinator->CreateEntity(
		TitleControllerComponent(titleCtrl)
	);

	std::cout << "TitleScene::Init() - TitleUiSystem Ready." << std::endl;
}

void TitleScene::Uninit()
{
	auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>();
	if (effectSystem)
	{
		effectSystem->Uninit();
	}

	ECS::ECSInitializer::UninitECS();

	m_coordinator.reset();
}

void TitleScene::Update(float deltaTime)
{
	// 1. システムの一括更新
	// (ここで UIInputSystem も自動的に動くので、手動呼び出しは不要です！)
	m_coordinator->UpdateSystems(deltaTime);

#ifdef _DEBUG
	if (IsKeyTrigger('E'))
	{
		ECS::EntityFactory::CreateOneShotEffect(m_coordinator.get(), "EFK_TITLE_SHINE", { 0, 0, 0 }, 6.0f);
	}
#endif // _DEBUG
}
void TitleScene::Draw()
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