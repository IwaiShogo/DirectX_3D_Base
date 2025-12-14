/*****************************************************************//**
 * @file	FlickerSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/13	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___FLICKER_SYSTEM_H___
#define ___FLICKER_SYSTEM_H___

#include "ECS/ECS.h"

class FlickerSystem : public ECS::System
{
public:
    void Init(ECS::Coordinator* coordinator) { m_coordinator = coordinator; }
    void Update(float deltaTime) override;
private:
    ECS::Coordinator* m_coordinator;
};

#endif // !___FLICKER_SYSTEM_H___