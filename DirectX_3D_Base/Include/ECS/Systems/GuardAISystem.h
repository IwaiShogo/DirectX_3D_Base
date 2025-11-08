/*****************************************************************//**
 * @file	GuardAISystem.h
 * @brief	プレイヤーの位置を追跡する警備員AIのコンポーネント定義
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Fukudome Hiroaki
 * ------------------------------------------------------------
 * 
 * @date	2025/11/09	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GUARD_AI_SYSTEM_H___
#define ___GUARD_AI_SYSTEM_H___

#include "ECS/ECS.h"

class GuardAISystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    void Update();
};

#endif // !___GUARD_AI_SYSTEM_H___