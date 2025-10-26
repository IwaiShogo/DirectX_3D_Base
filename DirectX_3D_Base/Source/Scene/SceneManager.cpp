/*****************************************************************//**
 * @file	SceneManager.cpp
 * @brief	ゲームシーンの管理、切り替え、ライフサイクル制御を行う静的クラスの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：`SceneManager`の静的メンバーを定義し、シーン切り替えロジックを実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/SceneManager.h"
#include <iostream>

// ===== 静的メンバー変数の定義 =====
std::unique_ptr<Scene> SceneManager::s_currentScene = nullptr;
std::function<Scene* ()> SceneManager::s_nextSceneFactory = nullptr;
std::unordered_map<std::type_index, std::function<Scene* ()>> SceneManager::s_sceneFactories;


/**
 * @brief 現在のシーンを破棄し、次のシーンを生成・初期化する内部処理
 */
void SceneManager::ProcessSceneChange()
{
	// 次のシーンのファクトリ関数が設定されていなければ何もしない
	if (!s_nextSceneFactory)
	{
		return;
	}

	// 1. 現在のシーンの終了処理と破棄
	if (s_currentScene)
	{
		s_currentScene->Uninit();
		// unique_ptr::reset()により、デストラクタが呼ばれ、メモリが解放される
		s_currentScene.reset();
		std::cout << "SceneManager: Previous scene uninitialized and destroyed." << std::endl;
	}

	// 2. 新しいシーンの生成と初期化
	// ファクトリ関数から新しいScene*を取得し、unique_ptrで所有権を管理
	Scene* newScene = s_nextSceneFactory();
	s_currentScene = std::unique_ptr<Scene>(newScene);
	s_currentScene->Init();

	// 3. 切り替え要求をクリア
	s_nextSceneFactory = nullptr;
	std::cout << "SceneManager: New scene initialized and started." << std::endl;
}

void SceneManager::Init()
{
	std::cout << "SceneManager: Initialized." << std::endl;
	// ここで全てのシーン（GameScene, TitleSceneなど）をRegisterScene<T>()で登録することが推奨される
}

void SceneManager::Uninit()
{
	// 現在のシーンの終了処理と破棄
	if (s_currentScene)
	{
		s_currentScene->Uninit();
		s_currentScene.reset();
		std::cout << "SceneManager: All scenes uninitialized and destroyed." << std::endl;
	}

	// ファクトリのクリア
	s_sceneFactories.clear();
}

void SceneManager::Update(float deltaTime)
{
	// 毎フレームの最初にシーン切り替えをチェック・実行する
	ProcessSceneChange();

	// 現在のシーンの更新処理
	if (s_currentScene)
	{
		s_currentScene->Update(deltaTime);
	}
}

void SceneManager::Draw()
{
	// 現在のシーンの描画処理
	if (s_currentScene)
	{
		s_currentScene->Draw();
	}
}