/*****************************************************************//**
 * @file	UIButtonComponent.h
 * @brief
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Oda kaito
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

#ifndef ___UI_BUTTON_COMPONENT_H___
#define ___UI_BUTTON_COMPONENT_H___

#include <functional>

 /**
  * @enum ButtonState
  * @brief ボタンの状態
  */
enum class ButtonState
{
	Normal,		// 通常
	Hover,		// カーソルが重なっている
	Pressed,	// 決定ボタン
};

/**
 * @struct UIButtonComponent
 * @brief ボタン機能を持つコンポーネント
 */
struct UIButtonComponent
{
	ButtonState state;				///< 現在の状態
	bool isVisible;					///< 有効か（非表示なら判定もしない想定）
	std::function<void()> onClick;	///< クリック時のコールバック関数
	DirectX::XMFLOAT3 originalScale; //元のサイズ
	// 追加
	ButtonState prevState = ButtonState::Normal;


	UIButtonComponent()
		: state(ButtonState::Normal)
		, isVisible(true)
		, onClick(nullptr)
		, originalScale{ 1.0f,1.0f,1.0f }
	{
	}

	UIButtonComponent(
		ButtonState initialState,
		bool initialVisible,
		std::function<void()> callback,
		const DirectX::XMFLOAT3& scale = { 1.0f, 1.0f, 1.0f }
	)
		: state(initialState)
		,prevState(initialState)
		, isVisible(initialVisible)
		, onClick(callback)
		, originalScale(scale)
	{
		if (!this->onClick)
		{
			this->onClick = []() {};
		}
	}
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(UIButtonComponent)

#endif // !___UI_BUTTON_COMPONENT_H___