/*****************************************************************//**
 * @file	CollisionComponent.h
 * @brief	Entity�̓����蔻��̌`��Ƒ������`����Component�B
 * 
 * @details	
 * AABB (Axis-Aligned Bounding Box) ���g�p���A�Փ˔���ɕK�v�ȃT�C�Y��
 * �Փ˃^�C�v�Ȃǂ̏���ێ�����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�Փ˔���ɕK�v�ȃT�C�Y�A�I�t�Z�b�g�A�Փ˃^�C�v�Ȃǂ�ێ����� `CollisionComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COLLISION_COMPONENT_H___
#define ___COLLISION_COMPONENT_H___

// ===== �C���N���[�h =====
#include <DirectXMath.h>
#include <cstdint>

 /**
  * @enum ColliderType
  * @brief �����蔻��̑����^�C�v�B
  * * Static: �����Ȃ��I�u�W�F�N�g�i���A�ǂȂǁj�B
  * * Dynamic: �������Z�̉e�����󂯂铮���I�u�W�F�N�g�i�v���C���[�A�G�Ȃǁj�B
  */
enum ColliderType : uint8_t
{
	COLLIDER_STATIC,	///< �ÓI�ȏՓˑ́i�ʒu�E�T�C�Y�͕ω����Ȃ��j
	COLLIDER_DYNAMIC,	///< ���I�ȏՓˑ́i�ʒu���������Z�ɂ���ĕω�����j
	COLLIDER_TRIGGER,	///< �Փˉ����͍s�킸�A�ʉ߁E�C�x���g�̂ݔ���������i�������j
};

/**
 * @struct CollisionComponent
 * @brief Entity�̓����蔻����
 */
struct CollisionComponent
{
	DirectX::XMFLOAT3 Size;		///< �����蔻��̔����̃T�C�Y (�n�[�t�G�N�X�e���g)
	DirectX::XMFLOAT3 Offset;	///< TransformComponent.Position����̑��ΓI�ȃI�t�Z�b�g
	ColliderType Type;			///< �Փˑ̂̃^�C�v�i�ÓI/���I�j
	uint32_t CollisionGroup;	///< �Փ˃t�B���^�����O�p�O���[�vID (�r�b�g�}�X�N�𐄏�)

	/**
	 * @brief �R���X�g���N�^
	 */
	CollisionComponent(
		DirectX::XMFLOAT3 size = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),
		DirectX::XMFLOAT3 offset = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		ColliderType type = COLLIDER_DYNAMIC,
		uint32_t group = 1 // �f�t�H���g��Group 0 (�r�b�g0)
	) : Size(size), Offset(offset), Type(type), CollisionGroup(group)
	{
	}
};

// Component�̎����o�^
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(CollisionComponent)

#endif // !___COLLISION_COMPONENT_H___