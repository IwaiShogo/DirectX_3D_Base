/*****************************************************************//**
 * @file	PhysicsSystem.cpp
 * @brief	TransformComponent��RigidBodyComponent������Entity�̉^�����v�Z����System�̎����B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FPhysicsSystem�̉^���ϕ����W�b�N�������B�d�́A���x�A�ʒu�̍X�V���s���B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "ECS/Systems/PhysicsSystem.h"
#include <iostream>

using namespace DirectX;

/**
 * @brief �^���ϕ��Ɗ�{�I�ȏՓˑO�������s��
 */
void PhysicsSystem::Update()
{
	// �^���ɕK�v�Ȏ��ԍ��� (dt = 1/FPS)
	// Main.h�Œ�`����Ă���fFPS���g�p
	const float deltaTime = 1.0f / fFPS;

	// System���ێ�����Entity�Z�b�g���C�e���[�g
	for (auto const& entity : m_entities)
	{
		// Component�������Ɏ擾
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RigidBodyComponent& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);

		// ���ʂ�0�̏ꍇ�͐ÓI�ȃI�u�W�F�N�g�Ƃ��Ĉ����A�^���v�Z���X�L�b�v
		if (rigidBody.Mass <= 0.0f)
		{
			continue;
		}

		// --- 1. �͂̓K�p (�����x�̍X�V) ---

		// �v���C���[���͂�O������̗͂��l�����Ȃ��ꍇ�A�����x�͎�ɏd�͂̂�
		rigidBody.Acceleration.y -= GRAVITY; // �������iY���}�C�i�X�����j�ɏd�͂�K�p

		// --- 2. �^���ϕ� (���x�̍X�V) ---

		// ���x���X�V: V_new = V_old + A * dt
		rigidBody.Velocity.x += rigidBody.Acceleration.x * deltaTime;
		rigidBody.Velocity.y += rigidBody.Acceleration.y * deltaTime;
		rigidBody.Velocity.z += rigidBody.Acceleration.z * deltaTime;

		// --- 3. ���C (Basic Damping) ---

		// ���C�W���𑬓x�ɓK�p���A���X�Ɍ��������� (X/Z���̂�)
		rigidBody.Velocity.x *= rigidBody.Friction;
		rigidBody.Velocity.z *= rigidBody.Friction;

		// ���C�ɂ������ȑ��x���[���ɃN���b�s���O
		if (fabs(rigidBody.Velocity.x) < 0.0001f) rigidBody.Velocity.x = 0.0f;
		if (fabs(rigidBody.Velocity.z) < 0.0001f) rigidBody.Velocity.z = 0.0f;

		// --- 4. �ʒu�̍X�V ---

		// �ʒu���X�V: P_new = P_old + V_new * dt
		transform.Position.x += rigidBody.Velocity.x * deltaTime;
		transform.Position.y += rigidBody.Velocity.y * deltaTime;
		transform.Position.z += rigidBody.Velocity.z * deltaTime;

		// --- 5. �����x�̃N���A ---

		// �͂�K�p���I������̂ŁA�����x�����Z�b�g�i���̃t���[���ōēx�͂�K�p���邽�߁j
		rigidBody.Acceleration.x = 0.0f;
		rigidBody.Acceleration.y = 0.0f;
		rigidBody.Acceleration.z = 0.0f;

		// TODO: CollisionComponent������Entity�ɑ΂��āA�Փ˔���E������������������
	}
}