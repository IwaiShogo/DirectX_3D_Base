/*****************************************************************//**
 * @file	Types.h
 * @brief	ECS�̃R�A�ƂȂ�^��`�ƒ萔���W�߂��t�@�C��
 * 
 * @details	
 * ECS��EntityID�AComponentID�ASystemID�A�����Component��
 * Signature�i�r�b�g�}�X�N�j�ɕK�v�Ȓ�`���s���B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS�̃R�A�ƂȂ�^��`�iID�ASignature�j�ƒ萔���`�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___TYPES_H___
#define ___TYPES_H___

// ===== �C���N���[�h =====
#include <bitset>
#include <cstdint>
#include <limits>

/**
 * @namespace ECS
 * @brief Entity Component System�̒�`���i�[���閼�O���
 */
namespace ECS
{
	// ===== �^�G�C���A�X��` (ECS�̊�{�v�f) =====

	/// @brief �G���e�B�e�B����ӂɎ��ʂ��邽�߂̌^ (32�r�b�g����)
	using EntityID = std::uint32_t;

	/// @brief �R���|�[�l���g�̃C���f�b�N�X�����ʂ��邽�߂̌^ (8�r�b�g����)
	/// (�R���|�[�l���g�̎�ނ���������ύX������)
	using ComponentTypeID = std::uint8_t;

	/// @brief �G���e�B�e�B���ێ�����R���|�[�l���g�̑g�ݍ��킹��\���r�b�g�}�X�N (Signature)
	/// MAX_COMPONENTS�̒l�ɉ����ăr�b�g�����ω�
	using Signature = std::bitset<std::numeric_limits<ComponentTypeID>::max()>;

	/// @brief �V�X�e���̃C���f�b�N�X�����ʂ��邽�߂̌^ (8�r�b�g����)
	using SystemTypeID = std::uint8_t;


	// ===== �萔��` (ECS�̐���) =====

	/// @brief �o�^�\�ȃR���|�[�l���g�̍ő吔
	/// ComponentTypeID�̍ő�l�Ɉˑ�����
	static const size_t MAX_COMPONENTS = std::numeric_limits<ComponentTypeID>::max();

	/// @brief �쐬�\�ȃG���e�B�e�B�̍ő吔
	static const size_t MAX_ENTITIES = 5000;

	/// @brief �����ȁi�����蓖�Ắj�G���e�B�e�BID
	static const EntityID INVALID_ENTITY_ID = static_cast<EntityID>(-1);
}

#endif // !___TYPES_H___