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

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用

// ===== 静的メンバー変数の定義 =====
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

using namespace DirectX;

void StageSelectScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_unique<ECS::Coordinator>();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 4. デモ用Entityの作成 ---
	//ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());

	std::cout << "StageSelectScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;
}

void StageSelectScene::Uninit()
{
	// 1. ECS Systemの静的リソースを解放
	ECS::ECSInitializer::UninitECS();

	// Coordinatorの破棄（unique_ptrが自動的にdeleteを実行）
	m_coordinator.reset();

	// 静的ポインタをクリア
	s_coordinator = nullptr;

	std::cout << "StageSelectScene::Uninit() - ECS Destroyed." << std::endl;
}

void StageSelectScene::Update(float deltaTime)
{
}

void StageSelectScene::Draw()
{
}