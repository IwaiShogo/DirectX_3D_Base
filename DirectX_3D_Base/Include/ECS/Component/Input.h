/*****************************************************************//**
 * @file	Input.h
 * @brief	���͏�Ԃ�ێ�����R���|�[�l���g
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/23	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___INPUT_H___
#define ___INPUT_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include <DirectXMath.h>
#include <map>
#include <string>

/**
 * @struct	Input
 * @brief	���[�U�[����̓��͏�ԁi�ړ��A�A�N�V�����Ȃǁj��ێ�����R���|�[�l���g
 * @note	InputSystem�ɂ���Ēl���������܂�AMovementSystem�Ȃǂɂ���ēǂݎ���܂��B
 */
struct Input
	: public IComponent
{
    // --------------------------------------------------
    // �ړ��֘A�̓���
    // --------------------------------------------------

    // �v���C���[�̈ړ��x�N�g���i���K�����ꂽ�����A��: �O��(0,0,1), ���(0,0,-1)�j
    DirectX::XMFLOAT3 movementVector = { 0.0f, 0.0f, 0.0f };

    // �ǂ̃L�[/�{�^�������݉�����Ă��邩���Ǘ�����t���O
    bool isMovingForward = false;
    bool isMovingBackward = false;
    bool isMovingLeft = false;
    bool isMovingRight = false;

    // --------------------------------------------------
    // �A�N�V�����֘A�̓���
    // --------------------------------------------------

    bool isAction1Pressed = false; // ��: �U���{�^��
    bool isJumpPressed = false; // ��: �W�����v�{�^��

    // --------------------------------------------------
    // �}�E�X�֘A�̓��� (�J��������Ȃǂɗ��p)
    // --------------------------------------------------

    float mouseDeltaX = 0.0f; // �O�t���[�������X���ړ���
    float mouseDeltaY = 0.0f; // �O�t���[�������Y���ړ���
    bool isMouseRightClick = false; // �E�N���b�N�̏��

    // ������ɓ��͒l�����Z�b�g����K�v�����邩�ǂ���
    bool shouldResetAfterUse = true;
};

#endif // !___INPUT_H___