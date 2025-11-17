/*****************************************************************//**
 * @file	UITimerSystem.h
 * @brief	一時的に表示されるUIのタイマーを管理するシステム。
 * * @details
 * TemporaryUIComponent と UIComponent を持つエンティティを監視し、
 * タイマーが0になったら UIComponent::IsVisible を false にし、
 * TemporaryUIComponent を削除する。
 * * ------------------------------------------------------------
 * @author	Iwai Shogo (Assisted by Machiko DX)
 * ------------------------------------------------------------
 * * @date	2025/11/16	初回作成日
 * 作業内容：	- 追加：
 * * @note	（省略可）
 *********************************************************************/

#ifndef ___UI_TIMER_SYSTEM_H___
#define ___UI_TIMER_SYSTEM_H___

 // ===== インクルード =====
#include "ECS/ECS.h"
#include <vector>

class UITimerSystem
	: public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;

public:
	/**
	 * @brief 初期化処理
	 * @param coordinator
	 */
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

	/**
	 * @brief 更新処理 (毎フレーム)
	 * @param deltaTime - 前フレームからの経過時間
	 */
	void Update(float deltaTime);
};

#endif // !___UI_TIMER_SYSTEM_H___