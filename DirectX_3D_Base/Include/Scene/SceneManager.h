/*****************************************************************//**
 * @file	SceneManager.h
 * @brief	ゲームシーンの管理、切り替え、ライフサイクル制御を行う静的クラス。
 * 
 * @details	
 * Title, Game, Resultなど、全てのSceneを抽象基底クラス`Scene`として扱い、
 * 遷移要求を安全に処理する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：シーンの管理・遷移機能を持つ `SceneManager` 静的クラスを定義。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SCENE_MANAGER_H___
#define ___SCENE_MANAGER_H___

#include <memory> // std::unique_ptr用
#include <typeindex> // シーンの型を識別するため
#include <functional> // シーンのファクトリ関数用
#include <unordered_map>

#include "Scene.h"
#include "StageSelectScene.h"
#include "StageinformationScene.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"
 /**
  * @class SceneManager
  * @brief シーンの登録、切り替え、実行を統括する静的マネージャクラス
  */
class SceneManager
{
private:
	// 現在実行中のシーンのインスタンス
	static std::unique_ptr<Scene> s_currentScene;

	// 次に切り替えるシーンのファクトリ関数（nullptrの場合は切り替えなし）
	static std::function<Scene* ()> s_nextSceneFactory;

	// シーンを生成するためのファクトリ関数のマップ（型ID -> ファクトリ関数）
	static std::unordered_map<std::type_index, std::function<Scene* ()>> s_sceneFactories;

	// @brief 現在のシーンを破棄し、次のシーンを生成・初期化する内部処理
	static void ProcessSceneChange();

public:
	SceneManager() = delete; // 静的クラスのためインスタンス化を禁止

	/// @brief シーンマネージャの初期化処理
	static void Init();

	/// @brief シーンマネージャの終了処理（現在のシーンを破棄）
	static void Uninit();

	/// @brief 毎フレームの更新処理。シーン切り替え処理を含む。
	static void Update(float deltaTime);

	/// @brief 毎フレームの描画処理
	static void Draw();

	/**
	 * @brief 新しいシーン型をSceneManagerに登録する
	 * @tparam T - Scene基底クラスを継承したシーンクラス
	 */
	template<typename T>
	static void RegisterScene()
	{
		std::type_index type = std::type_index(typeid(T));
		s_sceneFactories[type] = []() -> Scene* { return new T(); };
	}

	/**
	 * @brief シーンの切り替えをリクエストする（次のUpdate()で実行される）
	 * @tparam T - 切り替えたいSceneの具象型
	 */
	template<typename T>
	static void ChangeScene()
	{
		std::type_index type = std::type_index(typeid(T));
		if (s_sceneFactories.count(type))
		{
			s_nextSceneFactory = s_sceneFactories[type];
		}
		else
		{
			// TODO: エラー処理（未登録のシーン型）
			// 例外を投げるか、ログに出力
		}
	}
};

#endif // !___SCENE_MANAGER_H___