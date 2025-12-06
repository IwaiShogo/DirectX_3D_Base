/*****************************************************************//**
 * @file	GameScene.cpp
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

#include "Scene/GameScene.h"

#include "Ecs/Components/ScoreManager.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Gameplay/CollectionSystem.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "Systems/Input.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
#include <sstream> 

using namespace DirectX;

// ===== 静的メンバー変数の定義 =====u
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* GameScene::s_coordinator = nullptr;
std::string GameScene::s_StageNo = "";

void GameScene::Init()
{
	// ECS初期化
	m_coordinator = std::make_shared<ECS::Coordinator>();
	s_coordinator = m_coordinator.get();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 3. JSONコンフィグを使って一撃生成！ ---
	// あなたが作った「一撃関数」に、IDとCoordinatorを渡します
	// ※関数名は実際のコードに合わせて書き換えてください
	ECS::EntityFactory::GenerateStageFromConfig(m_coordinator.get(), s_StageNo);

	// --- 4. その他の共通Entityの作成 ---
	ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());

	ECS::EntityID cont = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
		),
		UIImageComponent(
			/* AssetID	*/	"BG_TOPVIEW",
			/* Depth		*/	-1.0f,
			/* IsVisible	*/	true
		)
	);

	// 画面幅いっぱいの細長い棒を作る
	float lineWidth = (float)SCREEN_WIDTH;
	float lineHeight = 5.0f; // 線の太さ

	// スキャンライン
	ECS::EntityID scanLine = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(lineWidth, lineHeight, 1)
		),
		UIImageComponent(
			/* AssetID		*/	"UI_SCAN_LINE",
			/* Depth		*/	2.0f,
			/* IsVisible	*/	true,
			/* Color		*/	XMFLOAT4(0.0f, 1.0f, 0.0f, 0.5f)
		),
		ScanLineComponent(
			/* Speed	*/	300.0f,
			/* Start	*/	0.0f,
			/* End		*/	(float)SCREEN_HEIGHT
		)
	);

	// デジタルグリッド
	ECS::EntityID grid = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
		),
		UIImageComponent(
			/* AssetID		*/	"UI_SCAN_LINE",
			/* Depth		*/	5.0f,
			/* IsVisible	*/	true,
			/* Color		*/	XMFLOAT4(0.0f, 1.0f, 1.0f, 0.1f)
		)
	);
}

void GameScene::Uninit()
{

	ECS::ECSInitializer::UninitECS();

	m_coordinator.reset();

	s_coordinator = nullptr;
}

void GameScene::Update(float deltaTime)
{

	m_elapsedTime += deltaTime;

	if (IsKeyTrigger('Q') || IsButtonTriggered(BUTTON_A))
	{
		SceneManager::ChangeScene<GameScene>();
	}
	
	m_coordinator->UpdateSystems(deltaTime);

}

void GameScene::Draw()
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

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render(false);
	}
}