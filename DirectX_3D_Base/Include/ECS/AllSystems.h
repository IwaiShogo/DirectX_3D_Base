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
// @system	固定カメラ
#include "ECS/Systems/Core/BasicCameraSystem.h"
// @system	ゲーム終了時の処理
#include "ECS/Systems/Core/GameControlSystem.h"
// @system	オーディオ
#include "ECS/Systems/Core/AudioSystem.h"
// @system	タイトルコントローラー
#include "ECS/Systems/Core/TitleControlSystem.h"
// @system	生存時間
#include "ECS/Systems/Core/LifeTimeSystem.h"


// @folder	Rendering
// @system	描画
#include "ECS/Systems/Rendering/RenderSystem.h"
// @system	デバッグ
#include "ECS/Systems/Rendering/DebugDrawSystem.h"
// @system	アニメーション
#include "ECS/Systems/Rendering/AnimationSystem.h"
// @system	エフェクト
#include "ECS/Systems/Rendering/EffectSystem.h"

// @folder	Physics
// @system	衝突判定
#include "ECS/Systems/Physics/CollisionSystem.h"
// @system	物理演算
#include "ECS/Systems/Physics/PhysicsSystem.h"

// @folder	UI
// @system	画像UI
#include "ECS/Systems/UI/UIRenderSystem.h"
// @system	UI入力
#include "ECS/Systems/UI/UIInputSystem.h"
// @system	カーソルUI
#include "ECS/Systems/UI/CursorSystem.h"

#include "ECS/Systems/UI/FloatingSystem.h"
// @folder	Gameplay
// @system	プレイヤー入力制御
#include "ECS/Systems/Gameplay/PlayerControlSystem.h"
// @system	システム
#include "ECS/Systems/Gameplay/CollectionSystem.h"
// @system	マップ生成
#include "ECS/Systems/Gameplay/MapGenerationSystem.h"
// @system	生成を遅らせる
#include "ECS/Systems/Gameplay/EnemySpawnSystem.h"

// @system	警備員AI
#include "ECS/Systems/Gimmick/GuardAISystem.h"

#endif // !___ALL_SYSTEM_H___