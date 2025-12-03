/*****************************************************************//**
 * @file	UIInputSystem.h
 * @brief	UIの入力判定を行うシステム
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author Oda Kaito
 * ------------------------------------------------------------
 *
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 *
 *
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef __UI_INPUT_SYSTEM_H__
#define __UI_INPUT_SYSTEM_H__

#include "ECS/ECS.h"
#include "ECS/SystemManager.h"

class UIInputSystem : public ECS::System
{

private:
	// Coordinatorのポインタを保持するメンバ変数
	ECS::Coordinator* m_coordinator = nullptr;

public:
	UIInputSystem() = default;

	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}


	void Update(float deltTime)override;

private:
	/**
	 * @brief 2つの矩形のオーバーラップ判定 (AABB)
	 * @param posA Aの位置
	 * @param scaleA Aのサイズ
	 * @param posB Bの位置
	 * @param scaleB Bのサイズ
	 * @return true: 重なっている
	 */
	bool IsOverlapping(
		const DirectX::XMFLOAT3& posA, const DirectX::XMFLOAT3& scaleA,
		const DirectX::XMFLOAT3& posB, const DirectX::XMFLOAT3& scaleB);
};
#endif // __UI_INPUT_SYSTEM_H_