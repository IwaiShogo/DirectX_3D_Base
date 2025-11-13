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

 // ===== インクルード =====
#include "Scene/ResultScene.h"
#include "ECS/ECSInitializer.h" // RenderSystem取得用
#include <iostream>


// 仮の入力チェック関数
bool IsInputTitle() { return false; }


// ===== ResultScene メンバー関数の実装 =====

void ResultScene::Init()
{
	
	// 2. スコア表示用のエンティティを作成
	// ECS::EntityFactory::CreateResultUIEntities(ECS::ECSInitializer::GetCoordinator());

	std::cout << "ResultScene::Init() - ResultUISystem Ready." << std::endl;
}

void ResultScene::Uninit()
{
	
	// 2. このシーンで作成したエンティティを破棄
	// ECS::ECSInitializer::GetCoordinator()->DestroyEntities(m_sceneEntities);

	std::cout << "ResultScene::Uninit() - Result Scene Systems Destroyed." << std::endl;
}

void ResultScene::Update(float deltaTime)
{
	if (IsKeyTrigger('N'))
	{
		SceneManager::ChangeScene<TitleScene>();
	}
}

void ResultScene::Draw()
{
	// Draw処理は共有する
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}
}