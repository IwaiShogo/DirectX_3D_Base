/*****************************************************************//**
 * @file	PlayerControlSystem.cpp
 * @brief	プレイヤーのキー入力に基づいてEntityの運動状態を走査するSystemの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：キー入力（左右移動、ジャンプ）に基づいてRigidBodyの速度を更新するロジックを実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/PlayerControlSystem.h"
#include <iostream>

using namespace DirectX;

/**
 * @brief 入力に応じてRigidBodyの速度を更新する
 */
void PlayerControlSystem::Update()
{
	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// Componentを高速に取得
		RigidBodyComponent& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
		PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entity);

		// 質量が0（静的オブジェクト）の場合は操作をスキップ
		if (rigidBody.mass <= 0.0f)
		{
			continue;
		}

		// --- 1. 水平移動の処理 (X軸) ---

		float targetX = 0.0f;
		float targetZ = 0.0f;

		// 左移動 (Aキー)
		if (IsKeyPress('A'))
		{
			targetX -= playerControl.moveSpeed;
		}
		// 右移動 (Dキー)
		if (IsKeyPress('D'))
		{
			targetX += playerControl.moveSpeed;
		}
		// 上移動 (Wキー)
		if (IsKeyPress('W'))
		{
			targetZ += playerControl.moveSpeed;
		}
		// 下移動 (Sキー)
		if (IsKeyPress('S'))
		{
			targetZ -= playerControl.moveSpeed;
		}

		// PhysicsSystemでの摩擦減速と競合しないよう、水平速度を直接設定
		rigidBody.velocity.x = targetX;
		rigidBody.velocity.z = targetZ;


		// --- 2. ジャンプの処理 (Y軸) ---

		// スペースキーがトリガーされ、かつ地面にいる場合（※現在は未実装のため一旦常に許可）
		// 正確な実装にはCollisionSystemでIsGroundedフラグを設定する必要がある
		if (IsKeyTrigger(VK_SPACE)) // || IsKeyTrigger(VK_UP)
		{
			// if (playerControl.IsGrounded) // 本来のロジック
			{
				rigidBody.velocity.y = playerControl.jumpPower; // Y軸上向きに初速を与える
			}
		}
	}
}