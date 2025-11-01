/*****************************************************************//**
 * @file	TransformComponent.h
 * @brief	Entity�̋�ԓI�Ȉʒu�A��]�A�X�P�[�����`����Component�B
 * 
 * @details	
 * DirectXMath�̃x�N�g���^���g�p���A3D��Ԃɂ�����Entity�̏�Ԃ�ێ�����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�ʒu�A��]�A�X�P�[����DirectX::XMFLOAT3�ŕێ����� `TransformComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___TRANSFORM_COMPONENT_H___
#define ___TRANSFORM_COMPONENT_H___

#include <DirectXMath.h>

 /**
  * @struct TransformComponent
  * @brief Entity�̈ړ��A��]�A�X�P�[�����i���[���h���W�ϊ��f�[�^�j
  */
struct TransformComponent
{
	DirectX::XMFLOAT3 Position;		///< ���[���h���W�n�ɂ�����ʒu (X, Y, Z)
	DirectX::XMFLOAT3 Rotation;		///< �I�C���[�p�ł̉�]�ʁi���W�A���j
	DirectX::XMFLOAT3 Scale;		///< �e���̃X�P�[���i�傫���j

	/**
	 * @brief �R���X�g���N�^
	 * @param pos - �����ʒu
	 * @param rot - ������]
	 * @param scale - �����X�P�[��
	 */
	TransformComponent(
		DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3 rot = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)
	) : Position(pos), Rotation(rot), Scale(scale)
	{}
};

// Component�̎����o�^
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TransformComponent)

#endif // !___TRANSFORM_COMPONENT_H___