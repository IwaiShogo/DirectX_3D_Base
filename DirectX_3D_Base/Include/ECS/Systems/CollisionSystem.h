/*****************************************************************//**
 * @file	CollisionSystem.h
 * @brief	CollisionComponent������Entity�Ԃ̏Փˌ��o�Ɖ�������������System
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS::System���p������'CollisionSystem'���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COLLISION_SYSTEM_H___
#define ___COLLISION_SYSTEM_H___

// ===== �C���N���[�h =====
// ECS
#include "../ECS/Coordinator.h"
#include "../ECS/SystemManager.h"
// Components
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/CollisionComponent.h"
#include "../Components/PlayerControlComponent.h" // �Փˉ�����IsGrounded�t���O�X�V�Ɏg�p
// Scene
#include "../Scene/GameScene.h" 

#include <DirectXMath.h>
#include <memory>
#include <tuple> // �Փˏ���ێ����邽��

/**
 * @class CollisionSystem
 * @brief Entity�Ԃ̏Փ˂����o���A���̂̈ʒu�Ƒ��x���C������System
 * * �����Ώ�: TransformComponent, RigidBodyComponent, CollisionComponent������Entity
 */
class CollisionSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator;

	/**
	 * @brief AABB�Ԃ̏Փˌ��o�ƍŏ��ړ��x�N�g��(MTV)���v�Z����B
	 * @param entityA, entityB - �Փ˃`�F�b�N�Ώۂ�EntityID
	 * @param mtv - �v�Z���ꂽ�ŏ��ړ��x�N�g���ƏՓ˖ʖ@��
	 * @return bool - �Փ˂��Ă���ꍇ��true
	 */
	bool CheckCollision(ECS::EntityID entityA, ECS::EntityID entityB, DirectX::XMFLOAT3& mtv);

	/**
	 * @brief �Փ˂����������ꍇ�̉��������i�ʒu�C���Ƒ��x�ύX�j
	 * @param entityA, entityB - �Փ˂���EntityID
	 * @param mtv - �ŏ��ړ��x�N�g��
	 */
	void ResolveCollision(ECS::EntityID entityA, ECS::EntityID entityB, const DirectX::XMFLOAT3& mtv);

public:
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief �Փˌ��o�Ɖ������s��
	void Update();
};

#endif // !___COLLISION_SYSTEM_H___