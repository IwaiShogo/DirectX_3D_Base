/*****************************************************************//**
 * @file	LifeTimeSystem.h
 * @brief	生存時間
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/08	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___LIFE_TIME_SYSTEM_H___
#define ___LIFE_TIME_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"

class LifeTimeSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

public:
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

    void Update(float deltaTime) override;
};

#endif // !___LIFE_TIME_SYSTEM_H___