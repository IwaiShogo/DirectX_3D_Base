/*****************************************************************//**
 * @file	Transform.h
 * @brief	�G���e�B�e�B�̋�ԏ��i�ʒu�A��]�A�X�P�[���j��ێ�����R���|�[�l���g
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___TRANSFORM_H___
#define ___TRANSFORM_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include <DirectXMath.h>

/**
 * @struct	Transform
 * @brief	Entity��3D��Ԃɂ�����ʒu�A��]�A�X�P�[���A����у��[���h�s���ێ�����R���|�[�l���g
 * @note	ECS�ɂ�����Component�͏����ȃf�[�^�R���e�i�ł���ׂ��ł��B
 */
struct Transform
	: public IComponent
{
	// C++�̑g�ݍ��݌^��DirectX�̃V���v���ȃx�N�g���^���g�p
	// XMFLOAT3��DirectX Math�Ƃ̌݊��������߂邽�߂ɐ�������܂�
	DirectX::XMFLOAT3 position	= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 rotation	= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale		= { 1.0f, 1.0f, 1.0f };

	// �L���b�V�����ꂽ���[���h�s��iSystem�ɂ���Ė��t���[���v�Z�E�X�V�����j
	DirectX::XMFLOAT4X4 worldMatrix;

	/**
	 * [void - Transform]
	 * @brief	�R���X�g���N�^�Ń��[���h�s���������
	 */
	Transform()
	{
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	}
};

#endif // !___TRANSFORM_H___