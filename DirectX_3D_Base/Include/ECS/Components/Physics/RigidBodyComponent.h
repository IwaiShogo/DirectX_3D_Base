/*****************************************************************//**
 * @file	RigidBodyComponent.h
 * @brief	Entityの運動状態（質量、速度、加速度など）を定義するComponent
 * 
 * @details	
 * PhysicsSystemによって更新され、TransformComponentの位置に影響を与える。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：物理演算に必要な速度、加速度、質量、摩擦係数などを保持する `RigidBodyComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___RIGID_BODY_COMPONENT_H___
#define ___RIGID_BODY_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>

 /**
  * @struct RigidBodyComponent
  * @brief Entityの運動状態に関するデータ
  */
struct RigidBodyComponent
{
	DirectX::XMFLOAT3 velocity;		///< 速度 (m/s)
	DirectX::XMFLOAT3 acceleration;	///< 加速度 (m/s^2)
	float mass;						///< 質量 (kg). 0の場合は無限大（静止オブジェクト）として扱う。
	float friction;					///< 摩擦係数 (0.0f～1.0f)
	float restitution;				///< 反発係数 (0.0f～1.0f)

	/**
	 * @brief コンストラクタ
	 */
	RigidBodyComponent(
		DirectX::XMFLOAT3 vel = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3 acc = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		float mass = 1.0f,
		float friction = 0.8f,
		float restitution = 0.2f
	) : velocity(vel), acceleration(acc), mass(mass), friction(friction), restitution(restitution)
	{
	}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(RigidBodyComponent)

#endif // !___RIGID_BODY_COMPONENT_H___