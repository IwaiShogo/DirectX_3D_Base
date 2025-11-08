/*****************************************************************//**
 * @file	DebugComponent.h
 * @brief	デバッグモードの状態を保持するComponent
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：デバッグモードのON/OFF状態を保持するDebugComponentを作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___DEBUG_COMPONENT_H___
#define ___DEBUG_COMPONENT_H___

 /**
  * @struct DebugComponent
  * @brief ゲームのデバッグモード状態を保持する
  */
struct DebugComponent
{
	bool isDebugModeActive = false;		///< デバッグモードが有効か (F1キーでトグル)
	bool isDrawLinesEnabled = true;		///< お手本プログラムのライン描画が有効か

	DebugComponent() = default;
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(DebugComponent)

#endif // !___DEBUG_COMPONENT_H___