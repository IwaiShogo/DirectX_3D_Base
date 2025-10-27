/*****************************************************************//**
 * @file	PlayerControlSystem.h
 * @brief	プレイヤーのキー入力に基づいてEntityの運動状態を操作するSystem
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：プレイヤーのキー入力処理を実装する'PlayerControlSystem'を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___PLAYER_CONTROL_SYSTEM_H___
#define ___PLAYER_CONTROL_SYSTEM_H___

// ===== インクルード =====
// ECS
#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
// Components
#include "ECS/Components/RigidBodyComponent.h"
#include "ECS/Components/PlayerControlComponent.h"
#include "Scene/GameScene.h" 
#include "Systems/Input.h" // IsKeyHold, IsKeyTrigger を使用

 /**
  * @class PlayerControlSystem
  * @brief プレイヤーのキーボード入力を処理し、RigidBodyに力を与えるSystem
  * * 処理対象: RigidBodyComponent と PlayerControlComponent を持つ全てのEntity
  */
class PlayerControlSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator;

public:
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief 入力に応じてRigidBodyの速度を更新する
	void Update();
};

#endif // !___PLAYER_CONTROL_SYSTEM_H___