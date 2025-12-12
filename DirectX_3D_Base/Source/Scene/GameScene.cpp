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

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
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


	// 1. トップビュー用
	ECS::EntityID scoutingBGM = ECS::EntityFactory::CreateLoopSoundEntity(
		m_coordinator.get(),
		"BGM_TEST",
		0.5f
	);
	// タグを "BGM_SCOUTING" に変更
	if (m_coordinator->HasComponent<TagComponent>(scoutingBGM)) {
		m_coordinator->GetComponent<TagComponent>(scoutingBGM).tag = "BGM_SCOUTING";
	}

	// 2. アクション用
	ECS::EntityID actionBGM = ECS::EntityFactory::CreateLoopSoundEntity(
		m_coordinator.get(),
		"BGM_TEST2",
		0.5f
	);
	// タグを "BGM_ACTION" に変更
	if (m_coordinator->HasComponent<TagComponent>(actionBGM)) {
		m_coordinator->GetComponent<TagComponent>(actionBGM).tag = "BGM_ACTION";
	}

	// アクション用は止めておく
	if (m_coordinator->HasComponent<SoundComponent>(actionBGM)) {
		m_coordinator->GetComponent<SoundComponent>(actionBGM).RequestStop();
	}	ECS::EntityID gameController = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator.get());
	auto& gameState = m_coordinator->GetComponent<GameStateComponent>(gameController);

	ECS::EntityID topviewBG = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
		),
		UIImageComponent(
			/* AssetID		*/	"BG_TOPVIEW",
			/* Depth		*/	-1.0f,
			/* IsVisible	*/	true
		)
	);
	gameState.topviewBgID = topviewBG;

	ECS::EntityID tpsBG = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
		),
		UIImageComponent(
			/* AssetID		*/	"UI_SCAN_LINE",
			/* Depth		*/	-1.0f,
			/* IsVisible	*/	false
		)
	);
	gameState.tpsBgID = tpsBG;

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

	// ------------------------------
	// トップビュー開始時のフェードイン（黒→表示）
	// ------------------------------
	m_isFadeIn = true;
	m_fadeTimer = 0.0f;

	// フルスクリーン黒板（UI_WHITE を黒で乗算して使う想定）
	m_fadeEntity = m_coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
		),
		UIImageComponent(
			"UI_WHITE",
			1000.0f,
			true,
			XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) // 黒・完全不透明からスタート
		)
	);
}


void GameScene::Uninit()
{
	auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>();
	if (effectSystem)
	{
		effectSystem->Uninit();
	}

	ECS::ECSInitializer::UninitECS();

	m_coordinator.reset();
	s_coordinator = nullptr;
}

void GameScene::Update(float deltaTime)
{
	m_coordinator->UpdateSystems(deltaTime);

	
	UpdateFadeIn(deltaTime);
}

void GameScene::UpdateFadeIn(float deltaTime)
{
	if (!m_isFadeIn) return;
	if (m_fadeEntity == ECS::INVALID_ENTITY_ID) { m_isFadeIn = false; return; }
	if (!m_coordinator || !m_coordinator->HasComponent<UIImageComponent>(m_fadeEntity)) { m_isFadeIn = false; return; }

	m_fadeTimer += deltaTime;

	float t = (m_fadeInDuration <= 0.0f) ? 1.0f : (m_fadeTimer / m_fadeInDuration);
	if (t > 1.0f) t = 1.0f;
	if (t < 0.0f) t = 0.0f;

	// SmoothStep: t^2(3-2t)
	float eased = t * t * (3.0f - 2.0f * t);
	float alpha = 1.0f - eased; // 1→0

	auto& img = m_coordinator->GetComponent<UIImageComponent>(m_fadeEntity);
	img.isVisible = true;
	img.color = XMFLOAT4(0.0f, 0.0f, 0.0f, alpha);

	if (t >= 1.0f)
	{
		m_isFadeIn = false;
		img.isVisible = false;
	}
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

	if (auto system = ECS::ECSInitializer::GetSystem<EffectSystem>())
	{
		system->Render();
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render(false);
	}
}