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
// @component	カメラ
#include "ECS/Components/CameraComponent.h"
// @component	衝突判定
#include "ECS/Components/CollisionComponent.h"
// @component	モデル描画
#include "ECS/Components/ModelComponent.h"
// @component	プレイヤー入力制御
#include "ECS/Components/PlayerControlComponent.h"
// @component	描画
#include "ECS/Components/RenderComponent.h"
// @component	剛体 / 物理演算
#include "ECS/Components/RigidBodyComponent.h"
// @component	位置・回転・スケール
#include "ECS/Components/TransformComponent.h"
// @component	UI要素
#include "ECS/Components/UIComponent.h"
// @component	カメラ切り替え
#include "ECS/Components/GameStateComponent.h"
// @component	タグ
#include "ECS/Components/TagComponent.h"
// @component	回収可能なお宝
#include "ECS/Components/CollectableComponent.h"
// @component	ステージ内のアイテム管理
#include "ECS/Components/ItemTrackerComponent.h"
// @component	マップ管理
#include "ECS/Components/MapComponent.h"


// @component	デバッグ
#include "ECS/Components/DebugComponent.h"
// @component	警備員AI
#include "ECS/Components/GuardComponent.h"

#endif // !___ALL_COMPONENTS_H___