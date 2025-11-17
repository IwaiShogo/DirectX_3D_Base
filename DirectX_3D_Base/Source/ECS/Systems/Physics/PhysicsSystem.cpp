/*****************************************************************//**
 * @file	PhysicsSystem.cpp
 * @brief	TransformComponentとRigidBodyComponentを持つEntityの運動を計算するSystemの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：PhysicsSystemの運動積分ロジックを実装。重力、速度、位置の更新を行う。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/Physics/PhysicsSystem.h"
#include <iostream>

using namespace DirectX;

/**
 * @brief 運動積分と基本的な衝突前処理を行う
 */
void PhysicsSystem::Update()
{
	// 運動に必要な時間差分 (dt = 1/FPS)
	// Main.hで定義されているfFPSを使用
	const float deltaTime = 1.0f / fFPS;

	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// Componentを高速に取得
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RigidBodyComponent& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);

		// 質量が0の場合は静的なオブジェクトとして扱い、運動計算をスキップ
		if (rigidBody.mass <= 0.0f)
		{
			continue;
		}

		// --- 1. 力の適用 (加速度の更新) ---

		// プレイヤー入力や外部からの力を考慮しない場合、加速度は主に重力のみ
		rigidBody.acceleration.y -= GRAVITY; // 下向き（Y軸マイナス方向）に重力を適用

		// --- 2. 運動積分 (速度の更新) ---

		// 速度を更新: V_new = V_old + A * dt
		rigidBody.velocity.x += rigidBody.acceleration.x * deltaTime;
		rigidBody.velocity.y += rigidBody.acceleration.y * deltaTime;
		rigidBody.velocity.z += rigidBody.acceleration.z * deltaTime;

		// --- 3. 摩擦 (Basic Damping) ---

		// 摩擦係数を速度に適用し、徐々に減速させる (X/Z軸のみ)
		rigidBody.velocity.x *= rigidBody.friction;
		rigidBody.velocity.z *= rigidBody.friction;

		// 摩擦による微小な速度をゼロにクリッピング
		if (fabs(rigidBody.velocity.x) < 0.0001f) rigidBody.velocity.x = 0.0f;
		if (fabs(rigidBody.velocity.z) < 0.0001f) rigidBody.velocity.z = 0.0f;

		// --- 4. 位置の更新 ---

		// 位置を更新: P_new = P_old + V_new * dt
		transform.position.x += rigidBody.velocity.x * deltaTime;
		transform.position.y += rigidBody.velocity.y * deltaTime;
		transform.position.z += rigidBody.velocity.z * deltaTime;

		// --- 5. 加速度のクリア ---

		// 力を適用し終わったので、加速度をリセット（次のフレームで再度力を適用するため）
		rigidBody.acceleration.x = 0.0f;
		rigidBody.acceleration.y = 0.0f;
		rigidBody.acceleration.z = 0.0f;

		// TODO: CollisionComponentを持つEntityに対して、衝突判定・応答処理を実装する
	}
}