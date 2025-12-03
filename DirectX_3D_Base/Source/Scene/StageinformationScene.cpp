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
#include <ECS/Components/UI/UIButtonComponent.h>
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
}

void StageinformationScene::Draw()
{
}