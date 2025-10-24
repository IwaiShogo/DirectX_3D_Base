/*****************************************************************//**
 * @file	Collider.h
 * @brief	�Փ˔���ɕK�v�ȏ���ێ�����R���|�[�l���g
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/24	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COLLIDER_H___
#define ___COLLIDER_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include <DirectXMath.h>
#include <cstdint>

// �����̊􉽊w�I�ȃf�[�^�^�ւ̈ˑ�
#include "Systems/Geometory.h"	// AABB�AOBB�ASphere�Ȃǂ̒�`

enum class ColliderType
	: std::uint8_t
{
	NONE = 0,
	BOX_AABB,	// �����s�o�E���f�B���O�{�b�N�X�iAxis-Aligned Bounding Box�j
	BOX_OBB,	// �����t���o�E���f�B���O�{�b�N�X�iOriented Bounding Box�j
	SPHERE,		// �o�E���f�B���O�X�t�B�A
	CAPSULE,	// �J�v�Z���`��
	MESH,		// ���b�V���x�[�X�̏Փ˔���i���v���g�^�C�v�ł͏��O�j
};

struct Collider
	: public IComponent
{
    // --------------------------------------------------
    // �`���`
    // --------------------------------------------------

    /// @brief ����Collider���g�p����`��^�C�v
    ColliderType type = ColliderType::NONE;

    /// @brief �`��f�[�^ (���p��/�A���C�����g���l�������������K�v�ł����A�����ł̓V���v���ȍ\���̂��g�p)
    // ���ۂ̎����ł́A������������ECS�̃f�[�^�\���̃V���v������ۂ��߁A
    // ShapeData�Ƃ����`�ŕʂ̍\���̂⋤�p�̂Ƃ��Ē�`���ACoordinator�ŊǗ��𕪂��邱�Ƃ�����܂��B

    // BoundingBox (AABB�܂���OBB)
    DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f }; // ���[�J�����W�n�ł̒��S
    DirectX::XMFLOAT3 extent = { 0.5f, 0.5f, 0.5f }; // ���a/���� (AABB��Extent/OBB��Half-Extent)

    // BoundingSphere
    float radius = 0.5f;

    // --------------------------------------------------
    // �����v���p�e�B
    // --------------------------------------------------

    /// @brief �Փ˔���̃��C���[/�J�e�S�� (�ǂ̃O���[�v�ƏՓ˂��邩�𐧌�)
    std::uint32_t collisionLayer = 1;

    /// @brief �Փˎ��̉��� (�q�b�g�X�g�b�v�A�_���[�W�v�Z�Ȃ�) ��K�v�Ƃ��邩�ǂ���
    bool isTrigger = false;

    /// @brief �����I�ȑ��ݍ�p�i���������Ȃǁj���s�����ǂ���
    bool isKinematic = false; // true�̏ꍇ�A�������Z�œ�������Ȃ�

    // --------------------------------------------------
    // ��ԁiCollisionSystem�ɂ���ď������܂��j
    // --------------------------------------------------

    /// @brief �O�t���[���ŏՓ˂������������ǂ���
    bool collidedThisFrame = false;

    // �Փˑ����Entity ID�̃��X�g�i���G�ɂȂ邽�߁A�v���g�^�C�v�ł͒P���ID��ێ����邩�A�ʓr�C�x���g�V�X�e���ŏ����j
    // Entity lastHitEntity = 0;
};

#endif // !___COLLIDER_H___