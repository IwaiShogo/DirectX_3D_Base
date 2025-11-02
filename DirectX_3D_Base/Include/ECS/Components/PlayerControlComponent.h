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
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___PLAYER_CONTROL_COMPONENT_H___
#define ___PLAYER_CONTROL_COMPONENT_H___

// ===== インクルード =====
#include "Main.h"

/**
 * @struct PlayerControlComponent
 * @brief プレイヤー操作可能なEntityを示すタグとパラメータ
 */
struct PlayerControlComponent
{
	float moveSpeed;	///< 水平方向の移動速度 (METER/秒)
	float jumpPower;	///< Y軸方向のジャンプ初速 (METER/秒)
	bool isGrounded;	///< 地面に接触しているかどうかのフラグ (将来の衝突システムで使用)

	/**
	 * @brief コンストラクタ
	 */
	PlayerControlComponent(
		float moveSpeed = METER(4.0f),
		float jumpPower = METER(7.0f)
	) : moveSpeed(moveSpeed), jumpPower(jumpPower), isGrounded(false)
	{}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(PlayerControlComponent)

#endif // !___PLAYER_CONTROL_COMPONENT_H___