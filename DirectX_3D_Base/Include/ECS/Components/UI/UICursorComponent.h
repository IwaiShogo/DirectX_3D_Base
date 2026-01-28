/*****************************************************************//**
 * @file	UICursorComponent.h
 * @brief	UIカーソル用識別用コンポーネント
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

#ifndef ___UI_CURSOR_COMPONENT_H___
#define ___UI_CURSOR_COMPONENT_H___

/**
 * @struct	UICursorComponent
 * @brief	カーソルとしてふるまうエンティティに付与
 */
struct UICursorComponent
{
	// 状態
	bool isTriggered;	// このフレームで決定操作が行われたか

	// 設定
	float moveSpeed;	// スティック操作時の移動速度（px / frame）

	UICursorComponent(float speed = 10.0f)
		: isTriggered(false)
		, moveSpeed(speed)
	{}
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(UICursorComponent)

#endif // !___UI_CURSOR_COMPONENT_H___