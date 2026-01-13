/*****************************************************************//**
 * @file	EnemySpawnSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/12/11	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ENEMY_SPAWN_SYSTEM_H___
#define ___ENEMY_SPAWN_SYSTEM_H___

#include "ECS/ECS.h"

class EnemySpawnSystem
	: public ECS::System
{
public:
	void Init(ECS::Coordinator* coordinator)
	{
		m_coordinator = coordinator;
	}

	void Update(float deltaTime) override;

private:
	ECS::Coordinator* m_coordinator;
};

#endif // !___ENEMY_SPAWN_SYSTEM_H___