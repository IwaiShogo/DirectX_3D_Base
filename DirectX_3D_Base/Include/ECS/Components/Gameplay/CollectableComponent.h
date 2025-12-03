/*****************************************************************//**
 * @file	CollectableComponent.h
 * @brief	回収可能なお宝エンティティを定義するComponent
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：回収アイテムを識別するCollectableComponentを作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLECTABLE_COMPONENT_H___
#define ___COLLECTABLE_COMPONENT_H___

// ===== インクルード =====
#include <cstdint>

/**
 * @struct	CollectableComponent
 * @brief	回収可能なお宝エンティティに付与される
 */
struct CollectableComponent
{
	bool isCollected = false;
	float collectionRadius = 0.5f;
	int orderIndex = 0;//0なら順序無し1以上なら順序あり

	CollectableComponent(float radius, int order = 0)
		: collectionRadius(radius), orderIndex(order){}
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(CollectableComponent)

#endif // !___COLLECTABLE_COMPONENT_H___