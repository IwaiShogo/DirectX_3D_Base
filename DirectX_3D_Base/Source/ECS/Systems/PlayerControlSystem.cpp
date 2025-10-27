/*****************************************************************//**
 * @file	PlayerControlSystem.cpp
 * @brief	�v���C���[�̃L�[���͂Ɋ�Â���Entity�̉^����Ԃ𑖍�����System�̎����B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�L�[���́i���E�ړ��A�W�����v�j�Ɋ�Â���RigidBody�̑��x���X�V���郍�W�b�N�������B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "ECS/Systems/PlayerControlSystem.h"
#include <iostream>

using namespace DirectX;

/**
 * @brief ���͂ɉ�����RigidBody�̑��x���X�V����
 */
void PlayerControlSystem::Update()
{
	// System���ێ�����Entity�Z�b�g���C�e���[�g
	for (auto const& entity : m_entities)
	{
		// Component�������Ɏ擾
		RigidBodyComponent& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
		PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entity);

		// ���ʂ�0�i�ÓI�I�u�W�F�N�g�j�̏ꍇ�͑�����X�L�b�v
		if (rigidBody.Mass <= 0.0f)
		{
			continue;
		}

		// --- 1. �����ړ��̏��� (X��) ---

		float targetX = 0.0f;
		float targetZ = 0.0f;

		// ���ړ� (A�L�[)
		if (IsKeyPress('A'))
		{
			targetX -= playerControl.MoveSpeed;
		}
		// �E�ړ� (D�L�[)
		if (IsKeyPress('D'))
		{
			targetX += playerControl.MoveSpeed;
		}
		// ��ړ� (W�L�[)
		if (IsKeyPress('W'))
		{
			targetZ += playerControl.MoveSpeed;
		}
		// ���ړ� (S�L�[)
		if (IsKeyPress('S'))
		{
			targetZ -= playerControl.MoveSpeed;
		}

		// PhysicsSystem�ł̖��C�����Ƌ������Ȃ��悤�A�������x�𒼐ڐݒ�
		rigidBody.Velocity.x = targetX;
		rigidBody.Velocity.z = targetZ;


		// --- 2. �W�����v�̏��� (Y��) ---

		// �X�y�[�X�L�[���g���K�[����A���n�ʂɂ���ꍇ�i�����݂͖������̂��߈�U��ɋ��j
		// ���m�Ȏ����ɂ�CollisionSystem��IsGrounded�t���O��ݒ肷��K�v������
		if (IsKeyTrigger(VK_SPACE)) // || IsKeyTrigger(VK_UP)
		{
			// if (playerControl.IsGrounded) // �{���̃��W�b�N
			{
				rigidBody.Velocity.y = playerControl.JumpPower; // Y��������ɏ�����^����
			}
		}
	}
}