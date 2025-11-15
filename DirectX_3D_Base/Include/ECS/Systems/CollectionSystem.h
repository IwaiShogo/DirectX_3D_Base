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
	ECS::EntityID m_itemGetUI_ID = ECS::INVALID_ENTITY_ID;

	//  UI表示タイマー
	float m_uiTimer = 0.0f;
	//  UI表示時間（3秒）
	static constexpr float UI_DISPLAY_DURATION = 3.0f;
public:
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
		m_itemGetUI_ID = ECS::INVALID_ENTITY_ID;
		m_uiTimer = 0.0f; // 初期化
	}

	void Update(float deltaTime);

	/**
	 * @brief 表示対象のUIエンティティIDを設定する
	 * @param id
	 */
	void SetItemGetUI_ID(ECS::EntityID id) { m_itemGetUI_ID = id; }
};

#endif // !___COLLECTION_SYSTEM_H___