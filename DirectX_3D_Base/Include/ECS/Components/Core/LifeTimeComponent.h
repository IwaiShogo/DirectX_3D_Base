/*****************************************************************//**
 * @file	LifeTimeComponent.h
 * @brief	生存時間の管理
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

#ifndef ___LIFE_TIME_COMPONENT_H___
#define ___LIFE_TIME_COMPONENT_H___

struct LifeTimeComponent
{
    float lifeTime; // 生存時間（秒）

    LifeTimeComponent(float time = 1.0f)
        : lifeTime(time)
    {
    }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(LifeTimeComponent)

#endif // !___LIFE_TIME_COMPONENT_H___