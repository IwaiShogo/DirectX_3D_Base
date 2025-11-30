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
// @folder	Core
// @system	カメラ操作
#include "ECS/Systems/Core/CameraControlSystem.h"
// @system	カメラ切り替え
#include "ECS/Systems/Core/StateSwitchSystem.h"
// @system	ゲーム終了時の処理
#include "ECS/Systems/Core/GameFlowSystem.h"
// @system	オーディオ
#include "ECS/Systems/Core/AudioSystem.h"

// @folder	Rendering
// @system	描画
#include "ECS/Systems/Rendering/RenderSystem.h"
// @system	デバッグ
#include "ECS/Systems/Rendering/DebugDrawSystem.h"

// @folder	Physics
// @system	衝突判定
#include "ECS/Systems/Physics/CollisionSystem.h"
// @system	物理演算
#include "ECS/Systems/Physics/PhysicsSystem.h"

// @folder	UI
// @system	画像UI
#include "ECS/Systems/UI/UIRenderSystem.h"
// @system  アニメーションUI
#include "ECS/Systems/UI/UIAnimationSystem.h"

// @system  ズームアニメーション
#include "ECS/Systems/UI/ZoomTransitionSystem.h"

// @folder	Gameplay
// @system	プレイヤー入力制御
#include "ECS/Systems/Gameplay/PlayerControlSystem.h"
// @system	システム
#include "ECS/Systems/Gameplay/CollectionSystem.h"
// @system	マップ生成
#include "ECS/Systems/Gameplay/MapGenerationSystem.h"

// @system	警備員AI
#include "ECS/Systems/Gimmick/GuardAISystem.h"

#endif // !___ALL_SYSTEM_H___