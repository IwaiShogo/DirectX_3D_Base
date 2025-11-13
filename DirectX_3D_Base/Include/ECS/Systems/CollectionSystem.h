/*****************************************************************//**
 * @file	CollectionSystem.h
 * @brief	アイテムとプレイヤーの距離をチェックし、回収処理を実行します。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLECTION_SYSTEM_H___
#define ___COLLECTION_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"

class CollectionSystem
	: public ECS::System
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

#endif // !___COLLECTION_SYSTEM_H___