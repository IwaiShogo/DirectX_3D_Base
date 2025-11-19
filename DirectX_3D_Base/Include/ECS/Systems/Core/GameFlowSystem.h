/*****************************************************************//**
 * @file	GameFlowSystem.h
 * @brief	ゲームが終了した後の具体的なシーン処理
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GAME_FLOW_SYSTEM_H___
#define ___GAME_FLOW_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Scene/SceneManager.h" // シーン遷移のため（仮定）
#include "Scene/GameScene.h"     // リトライ時のシーン初期化のため（仮定）

 /**
  * @class GameFlowSystem
  * @brief ゲームオーバー/クリア判定を受け取り、シーン遷移（リトライ/次ステージ）を実行する
  * 処理対象: GameStateComponent を持つ Entity
  */
class GameFlowSystem
	: public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;

public:
	void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }
	void Update(float deltaTime) override;
};

#endif // !___GAME_FLOW_SYSTEM_H___