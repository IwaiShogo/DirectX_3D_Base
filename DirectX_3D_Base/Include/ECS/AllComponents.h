/*****************************************************************//**
 * @file	AllComponents.h
 * @brief	コンポーネント用集約ヘッダーファイル
 * 
 * @details	
 * 各シーンやシステムで個別のコンポーネントをインクルードする代わりに、
 * このファイルをインクルードすることで記述を簡略化出来ます。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：ECSコンポーネントのヘッダー集約ファイルを作成
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ALL_COMPONENTS_H___
#define ___ALL_COMPONENTS_H___

// ===== インクルード =====
// @brief	必要なECSの共通型定義
#include "ECS/Types.h"

// --- 全てのコンポーネントヘッダーを集約 ---
// @folder		Core
// @component	カメラ
#include "ECS/Components/Core/CameraComponent.h"
// @component	位置・回転・スケール
#include "ECS/Components/Core/TransformComponent.h"
// @component	カメラ切り替え
#include "ECS/Components/Core/GameStateComponent.h"
// @component	タグ
#include "ECS/Components/Core/TagComponent.h"
// @component	サウンド
#include "ECS/Components/Core/SoundComponent.h"

// @folder		Rendering
// @component	描画
#include "ECS/Components/Rendering/RenderComponent.h"
// @component	デバッグ
#include "ECS/Components/Rendering/DebugComponent.h"
// @component	モデル描画
#include "ECS/Components/Rendering/ModelComponent.h"

// @folder		Physics
// @component	衝突判定
#include "ECS/Components/Physics/CollisionComponent.h"
// @component	剛体 / 物理演算
#include "ECS/Components/Physics/RigidBodyComponent.h"

// @folder		UI
// @component	UI要素
#include "ECS/Components/UI/UIComponent.h"
// @component	一時的なUI
#include "ECS/Components/UI/TemporaryUIComponent.h"

// @folder		Gameplay
// @component	プレイヤー入力制御
#include "ECS/Components/Gameplay/PlayerControlComponent.h"
// @component	回収可能なお宝
#include "ECS/Components/Gameplay/CollectableComponent.h"
// @component	ステージ内のアイテム管理
#include "ECS/Components/Gameplay/ItemTrackerComponent.h"
// @component	マップ管理
#include "ECS/Components/Gameplay/MapComponent.h"

// @folder		Gimmick
// @component	警備員AI
#include "ECS/Components/Gimmick/GuardComponent.h"

#endif // !___ALL_COMPONENTS_H___