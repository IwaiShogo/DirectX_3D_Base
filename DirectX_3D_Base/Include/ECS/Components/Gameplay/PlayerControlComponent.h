/*****************************************************************//**
 * @file	PlayerControlComponent.h
 * @brief	Entityがプレイヤーによって操作されることを示すタグ・データコンポーネント。
 * 
 * @details	
 * プレイヤー操作に必要な移動速度、ジャンプ力などのパラメータを保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：プレイヤーの移動速度、ジャンプ力などを保持する `PlayerControlComponent` を作成。
 * 
 * @update	2025/11/03	最終更新日
 * 			作業内容：	- 追加：三人称視点操作に必要なパラメータと状態を追加。
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___PLAYER_CONTROL_COMPONENT_H___
#define ___PLAYER_CONTROL_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include "ECS/Types.h"

/**
 * @struct PlayerControlComponent
 * @brief プレイヤー操作可能なEntityを示すタグとパラメータ
 */
struct PlayerControlComponent
{
	float moveSpeed = 5.0f;			///< プレイヤーの移動速度
	//float rotationSpeed = 5.0f;	///< プレイヤーの回転速度

	// カメラ関連
	ECS::EntityID attachedCameraID = 0;	///< プレイヤーに追従するカメラEntityID

	// アクション関連（Bボタン）
	bool isItemStealTriggered = false;	///< アイテム窃盗アクションがトリガーされたか

	/**
	 * @brief コンストラクタ
	 */
	PlayerControlComponent(
		float speed = 5.0f
	) 
		: moveSpeed(speed)
	{}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(PlayerControlComponent)

#endif // !___PLAYER_CONTROL_COMPONENT_H___