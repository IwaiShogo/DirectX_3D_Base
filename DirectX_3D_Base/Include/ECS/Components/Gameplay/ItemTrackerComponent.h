/*****************************************************************//**
 * @file	ItemTrackerComponent.h
 * @brief	ステージ内のアイテム回収状況を追跡するComponent
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：総アイテム数と回収数を保持するItemTrackerComponentを作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ITEM_TRACKER_COMPONENT_H___
#define ___ITEM_TRACKER_COMPONENT_H___

// ===== インクルード =====
#include <cstdint>

/**
 * @struct	ItemTrackerComponent
 * @brief	ゲーム内のアイテムの総数と回収数を追跡する
 */
struct ItemTrackerComponent
{
	uint32_t totalItems = 0;		// アイテムの総数
	uint32_t collectedItems = 0;	// 集めたアイテム

	bool useOrderedCollection = false;//このステージで順序モードが有効かどうか
	int currentTargetOrder = 1;//次に取るアイテム番号


	// コンストラクタ
	ItemTrackerComponent(uint32_t total = 0, bool ordered = false)
		: totalItems(total), useOrderedCollection(ordered), currentTargetOrder(1) {}
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(ItemTrackerComponent)

#endif // !___ITEM_TRACKER_COMPONENT_H___