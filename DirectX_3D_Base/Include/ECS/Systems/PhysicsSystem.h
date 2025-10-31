/*****************************************************************//**
 * @file	PhysicsSystem.h
 * @brief	TransformComponent��RigidBodyComponent������Entity�̉^�����v�Z����System�B
 * 
 * @details	
 * �d�͂Ɖ^���ϕ���K�p���AEntity�̈ʒu���X�V����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS::System���p������ `PhysicsSystem` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___PHYSICS_SYSTEM_H___
#define ___PHYSICS_SYSTEM_H___

// ===== �C���N���[�h =====
#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
// Components
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RigidBodyComponent.h"
#include "ECS/Components/CollisionComponent.h" // �����I�ȏՓˏ����ɔ����ăC���N���[�h

// Scene
#include "Scene/GameScene.h" 
#include "Main.h" // GRAVITY��fFPS�̒萔�ɃA�N�Z�X

#include <memory>

 /**
  * @class PhysicsSystem
  * @brief Entity�̕����^�����V�~�����[�V��������System
  * * �����Ώ�: TransformComponent �� RigidBodyComponent �����S�Ă�Entity
  */
class PhysicsSystem : public ECS::System
{
private:
	// ComponentManager�ւ̃A�N�Z�X�ȗ����̂��߁ACoordinator��ێ�
	ECS::Coordinator* m_coordinator;

public:
	// System�̏�����
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

	/// @brief �^���ϕ��Ɗ�{�I�ȏՓˑO�������s��
	void Update();
};

#endif // !___PHYSICS_SYSTEM_H___