/*****************************************************************//**
 * @file	BasicCameraSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/03	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___BASIC_CAMERA_SYSTEM_H___
#define ___BASIC_CAMERA_SYSTEM_H___

#include "ECS/ECS.h"

 /**
  * @class BasicCameraSystem
  * @brief TransformComponentとBasicCameraComponentに基づいて行列を更新するシステム
  */
class BasicCameraSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;

public:
	/**
	 * @brief 初期化処理（署名の設定など）
	 */
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

	/**
	 * @brief 更新処理
	 * @param deltaTime デルタタイム
	 */
	void Update(float deltaTime) override;
};

#endif // !___BASIC_CAMERA_SYSTEM_H___