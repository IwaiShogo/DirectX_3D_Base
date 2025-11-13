/*****************************************************************//**
 * @file	TitleScene.cpp
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
#include "Scene/TitleScene.h"
#include "ECS/ECSInitializer.h"
#include <iostream>

//仮の入力チェック関数
bool IsInputStart() {
	//ここに実際の入力チェックロジックが入る
	//今回は遷移テストのため、デバッグで一時的にtrueを返すなどしてもいい
	return false;
}

// ===== TitleScene メンバー関数の実装  =====
void TitleScene::Init()
{
	// TitleSceneに必要なエンティティの作成 (例：ロゴ、ボタン)
	//ECS::EntityFactory::CreateTitleUiEntities(ECS::ECSInitializer::GetCoordinator()):
	std::cout << "TitleScene::Init() - TitleUiSystem Ready." << std::endl;

}

void TitleScene::Uninit()
{
	//このシーンで作成したエンティティを破棄
	//ECS::ECSInitializer::GetCoordinator()->DestoryEntities(m_sceneEntities);
	std::cout << "TitleScene::Uninit() - Title  Systems Destroyed." << std::endl;
}

void TitleScene::Update(float deltaTime)
{
	if (IsKeyTrigger('N'))
	{
		SceneManager::ChangeScene<GameScene>();//N:ゲームシーンに切り替え
	}
}

void TitleScene::Draw()
{
	//RenderSystemは常に存在すると仮定し、Draw処理は共有する
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();	//UIエンティティの描画
	}
}