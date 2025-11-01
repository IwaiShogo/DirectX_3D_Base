/*****************************************************************//**
 * @file	RigidBodyComponent.h
 * @brief	Entity�̉^����ԁi���ʁA���x�A�����x�Ȃǁj���`����Component
 * 
 * @details	
 * PhysicsSystem�ɂ���čX�V����ATransformComponent�̈ʒu�ɉe����^����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�������Z�ɕK�v�ȑ��x�A�����x�A���ʁA���C�W���Ȃǂ�ێ����� `RigidBodyComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___RIGID_BODY_COMPONENT_H___
#define ___RIGID_BODY_COMPONENT_H___

// ===== �C���N���[�h =====
#include <DirectXMath.h>

 /**
  * @struct RigidBodyComponent
  * @brief Entity�̉^����ԂɊւ���f�[�^
  */
struct RigidBodyComponent
{
	DirectX::XMFLOAT3 Velocity;		///< ���x (m/s)
	DirectX::XMFLOAT3 Acceleration;	///< �����x (m/s^2)
	float Mass;						///< ���� (kg). 0�̏ꍇ�͖�����i�Î~�I�u�W�F�N�g�j�Ƃ��Ĉ����B
	float Friction;					///< ���C�W�� (0.0f�`1.0f)
	float Restitution;				///< �����W�� (0.0f�`1.0f)

	/**
	 * @brief �R���X�g���N�^
	 */
	RigidBodyComponent(
		DirectX::XMFLOAT3 vel = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3 acc = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		float mass = 1.0f,
		float friction = 0.8f,
		float restitution = 0.2f
	) : Velocity(vel), Acceleration(acc), Mass(mass), Friction(friction), Restitution(restitution)
	{
	}
};

// Component�̎����o�^
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(RigidBodyComponent)

#endif // !___RIGID_BODY_COMPONENT_H___