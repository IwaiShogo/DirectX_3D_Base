/*****************************************************************//**
 * @file	AllSystems.h
 * @brief	�V�X�e���p�W��w�b�_�[�t�@�C��
 * 
 * @details	
 * �e�V�[����V�X�e���Ōʂ̃V�X�e�����C���N���[�h�������ɁA
 * ���̃t�@�C�����C���N���[�h���邱�ƂŋL�q���ȗ����o���܂��B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS�V�X�e���̃w�b�_�[�W��t�@�C�����쐬
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ALL_SYSTEM_H___
#define ___ALL_SYSTEM_H___

// ===== �C���N���[�h =====
// @system	�J��������
#include "ECS/Systems/CameraControlSystem.h"
// @system	�v���C���[���͐���
#include "ECS/Systems/PlayerControlSystem.h"
// @system	�Փ˔���
#include "ECS/Systems/CollisionSystem.h"
// @system	�������Z
#include "ECS/Systems/PhysicsSystem.h"
// @system	�`��
#include "ECS/Systems/RenderSystem.h"
// @system	UI
#include "ECS/Systems/UISystem.h"

#endif // !___ALL_SYSTEM_H___