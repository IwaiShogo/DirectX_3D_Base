/*****************************************************************//**
 * @file	TemporaryUIComponent.h
 * @brief	エンティティが一時的に表示されることを示し、その残り時間を管理するComponent。
 * * @details
 * このコンポーネントを持つUIは、UITimerSystemによって監視され、
 * displayTimerが0になると非表示にされる。
 * * ------------------------------------------------------------
 * @author	Iwai Shogo (Assisted by Machiko DX)
 * ------------------------------------------------------------
 * * @date	2025/11/16	初回作成日
 * 作業内容：	- 追加：UIの表示時間を管理する `TemporaryUIComponent` を作成。
 *********************************************************************/

#ifndef ___TEMPORARY_UI_COMPONENT_H___
#define ___TEMPORARY_UI_COMPONENT_H___

 /**
  * @struct TemporaryUIComponent
  * @brief 一時的な表示時間（タイマー）を管理する
  */
struct TemporaryUIComponent
{
	float displayTimer; ///< 表示残り時間（秒）

	/**
	 * @brief コンストラクタ
	 * @param duration - 表示する時間（秒）
	 */
	TemporaryUIComponent(float duration = 3.0f)
		: displayTimer(duration)
	{}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TemporaryUIComponent)

#endif // !___TEMPORARY_UI_COMPONENT_H___