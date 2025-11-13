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
	bool isDebugModeActive = false;			// デバッグモードが有効か (F1キーでトグル)

	// --- 描画制御 ---
	bool isDrawLinesEnabled = true;			// BSP/MSTライン描画（F2でトグル）
	bool isCollisionDrawEnabled = false;	// 当たり判定の可視化（F3でトグル）
	bool isAIShown = false;					// AIステートの可視化（F4でトグル）

	// --- 時間制御 ---
	float timeScale = 1.0f;					// 1.0f: 通常速度、0.5f: スローモーション（NumPad+ / NumPad-）
	bool isTimeStopped = false;				// 時間停止（VK_PAUSEでトグル）
	bool isFrameStep = false;				// １フレーム送りモード（VK_f8でトグル）

	// --- テレポート/位置操作 ---
	DirectX::XMFLOAT3 teleportTarget = { 0.0f, 0.0f, 0.0f };	// テレポート先座標（F5で実行）

	// --- 強制制御 ---
	int forceGuardState = -1;	// 仮での実装

	DebugComponent() = default;
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(DebugComponent)

#endif // !___DEBUG_COMPONENT_H___