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
 * @date	2025/10/22	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/GameScene.h"

// Component

// System


/* リソースマネージャー */
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"



// --------------------------------------------------
// 内部関数: ECSの初期設定
// --------------------------------------------------

/**
 * @brief ECSコア、Component、Systemの初期化と登録を行う
 */
void GameScene::SetupECS()
{
    
}

/**
 * @brief ECS Entityの初期配置（テスト用Entityの生成など）を行う
 */
void GameScene::SetupEntities()
{
    
}

// --------------------------------------------------
// Sceneインターフェースの実装
// --------------------------------------------------

void GameScene::Initialize()
{
    // ECSのセットアップ
    SetupECS();

    // Entityの初期配置
    SetupEntities();

    // ... カメラやDirectXリソースの初期化など
}

void GameScene::Update(float deltaTime)
{
    
}

void GameScene::Draw()
{
    

}

void GameScene::Finalize()
{
    // ... シーン固有のリソース解放など
}