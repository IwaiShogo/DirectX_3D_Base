/*****************************************************************//**
 * @file	PlayerControlSystem.h
 * @brief	�v���C���[�̃L�[���͂Ɋ�Â���Entity�̉^����Ԃ𑀍삷��System
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�v���C���[�̃L�[���͏�������������'PlayerControlSystem'���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___PLAYER_CONTROL_SYSTEM_H___
#define ___PLAYER_CONTROL_SYSTEM_H___

// ===== �C���N���[�h =====
// ECS
#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
// Components
#include "ECS/Components/RigidBodyComponent.h"
#include "ECS/Components/PlayerControlComponent.h"
#include "Scene/GameScene.h" 
#include "Systems/Input.h" // IsKeyHold, IsKeyTrigger ���g�p

 /**
  * @class PlayerControlSystem
  * @brief �v���C���[�̃L�[�{�[�h���͂��������ARigidBody�ɗ͂�^����System
  * * �����Ώ�: RigidBodyComponent �� PlayerControlComponent �����S�Ă�Entity
  */
class PlayerControlSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator;

public:
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief ���͂ɉ�����RigidBody�̑��x���X�V����
	void Update();
};

#endif // !___PLAYER_CONTROL_SYSTEM_H___