/*****************************************************************//**
 * @file	AllComponents.h
 * @brief	�R���|�[�l���g�p�W��w�b�_�[�t�@�C��
 * 
 * @details	
 * �e�V�[����V�X�e���Ōʂ̃R���|�[�l���g���C���N���[�h�������ɁA
 * ���̃t�@�C�����C���N���[�h���邱�ƂŋL�q���ȗ����o���܂��B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS�R���|�[�l���g�̃w�b�_�[�W��t�@�C�����쐬
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ALL_COMPONENTS_H___
#define ___ALL_COMPONENTS_H___

// ===== �C���N���[�h =====
// @brief	�K�v��ECS�̋��ʌ^��`
#include "ECS/Types.h"

// --- �S�ẴR���|�[�l���g�w�b�_�[���W�� ---
// @component	�J����
#include "ECS/Components/CameraComponent.h"
// @component	�Փ˔���
#include "ECS/Components/CollisionComponent.h"
// @component	���f���`��
#include "ECS/Components/ModelComponent.h"
// @component	�v���C���[���͐���
#include "ECS/Components/PlayerControlComponent.h"
// @component	�`��
#include "ECS/Components/RenderComponent.h"
// @component	���� / �������Z
#include "ECS/Components/RigidBodyComponent.h"
// @component	�ʒu�E��]�E�X�P�[��
#include "ECS/Components/TransformComponent.h"
// @component	UI�v�f
#include "ECS/Components/UIComponent.h"

#endif // !___ALL_COMPONENTS_H___