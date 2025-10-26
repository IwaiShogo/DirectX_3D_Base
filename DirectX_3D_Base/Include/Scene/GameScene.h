/*****************************************************************//**
 * @file	GameScene.h
 * @brief	ゲームのメインロジックを含むシーンクラス。
 * 
 * @details	
 * ECSの初期化、管理、Systemの実行をこのシーンクラス内で完結させる。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/21	初回作成日
 * 			作業内容：	- 追加：ECS::CoordinatorのインスタンスとRenderSystemへのポインタをメンバーに追加。
 *						- 追加：他のシステムからCoordinatorにアクセスするための静的アクセサ関数を定義。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GAME_SCENE_H___
#define ___GAME_SCENE_H___

#include "Scene.h"
#include "ECS/Coordinator.h"

#include <memory>

// ===== 前方宣言 =====
class RenderSystem;

 /**
  * @class GameScene
  * @brief 実際のゲームロジックとECSを管理するシーン
  */
class GameScene
	: public Scene
{
private:
	// ECSの中心となるコーディネーター (シーンがECSのライフサイクルを管理)
	std::unique_ptr<ECS::Coordinator> m_coordinator;

	// 常に利用するSystemへの参照を保持 (Update/Drawの呼び出しを容易にする)
	std::shared_ptr<RenderSystem> m_renderSystem;

	// ECSのグローバルアクセス用 (SystemなどがECS操作を行うための窓口)
	static ECS::Coordinator* s_coordinator;

public:
	// コンストラクタとデストラクタ（Sceneを継承しているため仮想デストラクタはScene側で定義済みと仮定）
	GameScene();
	~GameScene() override; // 仮想デストラクタを実装

	// Sceneインターフェースの実装
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	/**
	 * @brief Coordinatorインスタンスへのポインタを取得する静的アクセサ
	 * @return ECS::Coordinator* - 現在アクティブなシーンのCoordinator
	 */
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }
};

#endif // !___GAME_SCENE_H___