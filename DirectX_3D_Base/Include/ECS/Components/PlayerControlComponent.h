/*****************************************************************//**
 * @file	PlayerControlComponent.h
 * @brief	Entity���v���C���[�ɂ���đ��삳��邱�Ƃ������^�O�E�f�[�^�R���|�[�l���g�B
 * 
 * @details	
 * �v���C���[����ɕK�v�Ȉړ����x�A�W�����v�͂Ȃǂ̃p�����[�^��ێ�����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�v���C���[�̈ړ����x�A�W�����v�͂Ȃǂ�ێ����� `PlayerControlComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___PLAYER_CONTROL_COMPONENT_H___
#define ___PLAYER_CONTROL_COMPONENT_H___

// ===== �C���N���[�h =====
#include "Main.h"

/**
 * @struct PlayerControlComponent
 * @brief �v���C���[����\��Entity�������^�O�ƃp�����[�^
 */
struct PlayerControlComponent
{
	float MoveSpeed;	///< ���������̈ړ����x (METER/�b)
	float JumpPower;	///< Y�������̃W�����v���� (METER/�b)
	bool IsGrounded;	///< �n�ʂɐڐG���Ă��邩�ǂ����̃t���O (�����̏Փ˃V�X�e���Ŏg�p)

	/**
	 * @brief �R���X�g���N�^
	 */
	PlayerControlComponent(
		float moveSpeed = METER(4.0f),
		float jumpPower = METER(7.0f)
	) : MoveSpeed(moveSpeed), JumpPower(jumpPower), IsGrounded(false)
	{}
};

// Component�̎����o�^
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(PlayerControlComponent)

#endif // !___PLAYER_CONTROL_COMPONENT_H___