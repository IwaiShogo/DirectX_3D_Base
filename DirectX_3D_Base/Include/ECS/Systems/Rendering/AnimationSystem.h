/*****************************************************************//**
 * @file	AnimationSystem.h
 * @brief	AnimationComponentを持つEnitityのアニメーションを更新するSystem。
 * 
 * @details	
 * Model::Stepを呼び出し、アニメーション時間の進行とボーン行列の計算を行う。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：AnimationSystemのヘッダーファイルを作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ANIMATION_SYSTEM_H___
#define ___ANIMATION_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include <DirectXMath.h>

/**
 * @class	AnimationSystem
 * @brief	アニメーションを更新するシステム
 */
class AnimationSystem
	: public ECS::System
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

#endif // !___ANIMATION_SYSTEM_H___