/*****************************************************************//**
 * @file	AllSystems.h
 * @brief	システム用集約ヘッダーファイル
 * 
 * @details	
 * 各シーンやシステムで個別のシステムをインクルードする代わりに、
 * このファイルをインクルードすることで記述を簡略化出来ます。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：ECSシステムのヘッダー集約ファイルを作成
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ALL_SYSTEM_H___
#define ___ALL_SYSTEM_H___

// ===== インクルード =====
// @system	カメラ操作
#include "ECS/Systems/CameraControlSystem.h"
// @system	プレイヤー入力制御
#include "ECS/Systems/PlayerControlSystem.h"
// @system	衝突判定
#include "ECS/Systems/CollisionSystem.h"
// @system	物理演算
#include "ECS/Systems/PhysicsSystem.h"
// @system	描画
#include "ECS/Systems/RenderSystem.h"
// @system	UI
#include "ECS/Systems/UISystem.h"
// @system	カメラ切り替え
#include "ECS/Systems/StateSwitchSystem.h"
// @system	ゲーム終了時の処理
#include "ECS/Systems/GameFlowSystem.h"
// @system	システム
#include "ECS/Systems/CollectionSystem.h"
// @system	警備員AI
#include "ECS/Systems/GuardAISystem.h"

#endif // !___ALL_SYSTEM_H___