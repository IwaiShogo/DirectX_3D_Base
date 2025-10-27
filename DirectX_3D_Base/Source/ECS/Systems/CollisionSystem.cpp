/*****************************************************************//**
 * @file	CollisionSystem.cpp
 * @brief	CollisionSystem�̎����BAABB�Փˌ��o�Ɗ�{�I�ȉ����������B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FAABB�Փˌ��o�iCheckCollision�j�Ɖ����iResolveCollision�j�̃��W�b�N������
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "ECS/Systems/CollisionSystem.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

/**
 * @brief AABB�Ԃ̏Փˌ��o�ƍŏ��ړ��x�N�g��(MTV)���v�Z����B
 * @param entityA, entityB - �Փ˃`�F�b�N�Ώۂ�EntityID
 * @param mtv - �v�Z���ꂽ�ŏ��ړ��x�N�g���ƏՓ˖ʖ@��
 * @return bool - �Փ˂��Ă���ꍇ��true
 */
bool CollisionSystem::CheckCollision(ECS::EntityID entityA, ECS::EntityID entityB, XMFLOAT3& mtv)
{
	TransformComponent& transA = m_coordinator->GetComponent<TransformComponent>(entityA);
	CollisionComponent& collA = m_coordinator->GetComponent<CollisionComponent>(entityA);
	TransformComponent& transB = m_coordinator->GetComponent<TransformComponent>(entityB);
	CollisionComponent& collB = m_coordinator->GetComponent<CollisionComponent>(entityB);

	// AABB�̒��S���W���v�Z
	XMFLOAT3 centerA = transA.Position;
	centerA.x += collA.Offset.x;
	centerA.y += collA.Offset.y;
	centerA.z += collA.Offset.z;

	XMFLOAT3 centerB = transB.Position;
	centerB.x += collB.Offset.x;
	centerB.y += collB.Offset.y;
	centerB.z += collB.Offset.z;

	// ���S�Ԃ̃x�N�g�� (distance)
	XMFLOAT3 d;
	d.x = centerA.x - centerB.x;
	d.y = centerA.y - centerB.y;
	d.z = centerA.z - centerB.z;

	// �Փ˂��Ă��Ȃ�����T��
	// �����̍��v�T�C�Y (extent)
	XMFLOAT3 extent;
	extent.x = collA.Size.x + collB.Size.x;
	extent.y = collA.Size.y + collB.Size.y;
	extent.z = collA.Size.z + collB.Size.z;

	// �����������Ă���ꍇ
	if (std::abs(d.x) >= extent.x ||
		std::abs(d.y) >= extent.y ||
		std::abs(d.z) >= extent.z)
	{
		return false; // �Փ˂Ȃ�
	}

	// �Փ˂��Ă���ꍇ: �ŏ��ړ��x�N�g��(MTV)���v�Z

	// �߂荞�ݗ� (penetration)
	float pX = extent.x - std::abs(d.x);
	float pY = extent.y - std::abs(d.y);
	float pZ = extent.z - std::abs(d.z);

	// �ł��߂荞�ݗʂ����Ȃ�����������
	if (pX < pY&& pX < pZ)
	{
		// X����MTV
		mtv.x = d.x > 0 ? pX : -pX; // �߂荞�ݗ� pX �𕄍��t���Őݒ�
		mtv.y = 0.0f;
		mtv.z = 0.0f;
	}
	else if (pY < pZ)
	{
		// Y����MTV
		mtv.x = 0.0f;
		mtv.y = d.y > 0 ? pY : -pY; // �߂荞�ݗ� pY �𕄍��t���Őݒ�
		mtv.z = 0.0f;
	}
	else
	{
		// Z����MTV
		mtv.x = 0.0f;
		mtv.y = 0.0f;
		mtv.z = d.z > 0 ? pZ : -pZ; // �߂荞�ݗ� pZ �𕄍��t���Őݒ�
	}

	return true;
}

/**
 * @brief �Փ˂����������ꍇ�̉��������i�ʒu�C���Ƒ��x�ύX�j
 * @param entityA, entityB - �Փ˂���EntityID
 * @param mtv - �ŏ��ړ��x�N�g��
 */
void CollisionSystem::ResolveCollision(ECS::EntityID entityA, ECS::EntityID entityB, const XMFLOAT3& mtv)
{
	// RigidBody��Transform���擾
	TransformComponent& transA = m_coordinator->GetComponent<TransformComponent>(entityA);
	RigidBodyComponent& rigidA = m_coordinator->GetComponent<RigidBodyComponent>(entityA);
	CollisionComponent& collA = m_coordinator->GetComponent<CollisionComponent>(entityA);

	TransformComponent& transB = m_coordinator->GetComponent<TransformComponent>(entityB);
	CollisionComponent& collB = m_coordinator->GetComponent<CollisionComponent>(entityB);

	// �ÓI�ȃI�u�W�F�N�g(entityB)�ɂ߂荞�񂾓��I�ȃI�u�W�F�N�g(entityA)���C������A�V���v���ȉ���
	if (collA.Type == COLLIDER_DYNAMIC && collB.Type == COLLIDER_STATIC)
	{
		// 1. �ʒu�̏C�� (�߂荞�݉���)
		transA.Position.x += mtv.x;
		transA.Position.y += mtv.y;
		transA.Position.z += mtv.z;

		// 2. ���x�̏C�� (����/��~)

		// �Փ˖ʖ@�� (mtv�̔�[����)
		XMFLOAT3 normal = { 0.0f, 0.0f, 0.0f };
		if (mtv.x != 0.0f) normal.x = mtv.x > 0 ? 1.0f : -1.0f;
		if (mtv.y != 0.0f) normal.y = mtv.y > 0 ? 1.0f : -1.0f;
		if (mtv.z != 0.0f) normal.z = mtv.z > 0 ? 1.0f : -1.0f;

		// �Փ˖ʂɉ��������x���L�����Z�����A�����W����K�p
		// Y���i�n�ʁj�Փ˂̏ꍇ
		if (normal.y != 0.0f)
		{
			// �������̑��x (normal.y > 0 ��A���ォ��Փ�) �𔽔�������
			if (rigidA.Velocity.y * normal.y < 0)
			{
				rigidA.Velocity.y *= -rigidA.Restitution; // �����𔺂�����

				// IsGrounded�t���O�̍X�V (PlayerControlComponent�����ꍇ)
				if (m_coordinator->m_entityManager->GetSignature(entityA).test(m_coordinator->GetComponentTypeID<PlayerControlComponent>()))
				{
					PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entityA);
					playerControl.IsGrounded = true;
				}
			}

			// �W�����v�������d�͂ɕ����Ȃ��悤�ɁA���x�����ɏ������ꍇ�̓[���ɃN���b�s���O
			if (std::abs(rigidA.Velocity.y) < 0.1f)
			{
				rigidA.Velocity.y = 0.0f;
			}
		}

		// X���Փ˂̏ꍇ (��)
		if (normal.x != 0.0f && rigidA.Velocity.x * normal.x < 0)
		{
			rigidA.Velocity.x *= -rigidA.Restitution;
		}
	}

	// TODO: Dynamic vs Dynamic �̏Փˉ������K�v�ɉ����Ēǉ�����
}

/**
 * @brief �Փˌ��o�Ɖ������s��
 */
void CollisionSystem::Update()
{
	// Static��Entity��Dynamic��Entity�̃��X�g���쐬
	std::vector<ECS::EntityID> dynamicEntities;
	std::vector<ECS::EntityID> staticEntities;

	for (auto const& entity : m_entities)
	{
		CollisionComponent& coll = m_coordinator->GetComponent<CollisionComponent>(entity);
		if (coll.Type == COLLIDER_DYNAMIC)
		{
			dynamicEntities.push_back(entity);
		}
		else if (coll.Type == COLLIDER_STATIC)
		{
			staticEntities.push_back(entity);
		}
	}

	// Dynamic Entity�� Static Entity�Ԃ̏Փ˃`�F�b�N (Dynamic vs Static)
	for (ECS::EntityID dynamicEntity : dynamicEntities)
	{
		// IsGrounded����U���Z�b�g (Update�̎n�߂Ɏ��s�����ׂ������A�����ł͏ՓˑO�Ƀ��Z�b�g)
		if (m_coordinator->m_entityManager->GetSignature(dynamicEntity).test(m_coordinator->GetComponentTypeID<PlayerControlComponent>()))
		{
			PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(dynamicEntity);
			playerControl.IsGrounded = false;
		}

		for (ECS::EntityID staticEntity : staticEntities)
		{
			XMFLOAT3 mtv = { 0.0f, 0.0f, 0.0f };

			if (CheckCollision(dynamicEntity, staticEntity, mtv))
			{
				ResolveCollision(dynamicEntity, staticEntity, mtv);
			}
		}
	}
}