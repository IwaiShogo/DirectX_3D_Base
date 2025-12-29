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
 * @date	2025/11/30	쐬
 * 			ƓeF	- ǉF
 *
 * @update	2025/xx/xx	ŏIXV
 * 			ƓeF	- XXF
 *
 * @note	iȗj
 *********************************************************************/

#include "Scene/GameScene.h"

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "Systems/Input.h"
#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerRenderSystem擾Ɏgp
#include <sstream> 

using namespace DirectX;

// ===== ÓIo[ϐ̒` =====u
// ̃VXeECSɃANZX邽߂̐ÓI|C^
ECS::Coordinator* GameScene::s_coordinator = nullptr;
std::string GameScene::s_StageNo = "";

void GameScene::Init()
{
	// ECS
	m_coordinator = std::make_shared<ECS::Coordinator>();
	s_coordinator = m_coordinator.get();
	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 3. JSONRtBOgĈꌂI ---
	// Ȃuꌂ֐vɁAIDCoordinatorn܂
	// ֐͎ۂ̃R[hɍ킹ďĂ
	ECS::EntityFactory::GenerateStageFromConfig(m_coordinator.get(), s_StageNo);


	// --- 4. ̑̋Entity̍쐬 ---
	//ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());

	ECS::EntityID gameController = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator.get());
	auto& gameState = m_coordinator->GetComponent<GameStateComponent>(gameController);

	// 1. gbvr[p
	ECS::EntityID scoutingBGM = ECS::EntityFactory::CreateLoopSoundEntity(
		m_coordinator.get(),
		"BGM_TEST",
		0.5f
	);
	// ^O "BGM_SCOUTING" ɕύX
	if (m_coordinator->HasComponent<TagComponent>(scoutingBGM)) {
		m_coordinator->GetComponent<TagComponent>(scoutingBGM).tag = "BGM_SCOUTING";
	}

	// 2. ANVp
	ECS::EntityID actionBGM = ECS::EntityFactory::CreateLoopSoundEntity(
		m_coordinator.get(),
		"BGM_TEST2",
		0.5f
	);
	// ^O "BGM_ACTION" ɕύX
	if (m_coordinator->HasComponent<TagComponent>(actionBGM)) {
		m_coordinator->GetComponent<TagComponent>(actionBGM).tag = "BGM_ACTION";
	}

	// ANVp͎~߂Ă
	if (m_coordinator->HasComponent<SoundComponent>(actionBGM)) {
		m_coordinator->GetComponent<SoundComponent>(actionBGM).RequestStop();
	}

	ECS::EntityID topviewBG = m_coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 200000.0f),
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
			/* IsVisible	*/	false,
			/* Color		*/	XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)
		)
	);
	gameState.tpsBgID = tpsBG;

	// ʕς̍ג_
	float lineWidth = (float)SCREEN_WIDTH;
	float lineHeight = 5.0f; // ̑

	// XLC
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

	// fW^Obh
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
	// gbvr[JñtF[hCi\j
	// ------------------------------
	m_isFadeIn = true;
	m_fadeTimer = 0.0f;

	// tXN[iUI_WHITE ŏZĎgzj
	m_fadeEntity = m_coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 200000.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
		),
		UIImageComponent(
			"UI_WHITE",
			200000.0f,
			true,
			XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) // ESsX^[g
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
	float alpha = 1.0f - eased; // 10

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