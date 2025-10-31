/*****************************************************************//**
 * @file	PhysicsSystem.h
 * @brief	TransformComponentとRigidBodyComponentを持つEntityの運動を計算するSystem。
 * 
 * @details	
 * 重力と運動積分を適用し、Entityの位置を更新する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：ECS::Systemを継承した `PhysicsSystem` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___PHYSICS_SYSTEM_H___
#define ___PHYSICS_SYSTEM_H___

// ===== インクルード =====
#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
// Components
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RigidBodyComponent.h"
#include "ECS/Components/CollisionComponent.h" // 将来的な衝突処理に備えてインクルード

// Scene
#include "Scene/GameScene.h" 
#include "Main.h" // GRAVITYとfFPSの定数にアクセス

#include <memory>

 /**
  * @class PhysicsSystem
  * @brief Entityの物理運動をシミュレーションするSystem
  * * 処理対象: TransformComponent と RigidBodyComponent を持つ全てのEntity
  */
class PhysicsSystem : public ECS::System
{
private:
	// ComponentManagerへのアクセス簡略化のため、Coordinatorを保持
	ECS::Coordinator* m_coordinator;

public:
	// Systemの初期化
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

	/// @brief 運動積分と基本的な衝突前処理を行う
	void Update();
};

#endif // !___PHYSICS_SYSTEM_H___