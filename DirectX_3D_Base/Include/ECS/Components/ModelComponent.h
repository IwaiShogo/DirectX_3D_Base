/*****************************************************************//**
 * @file	ModelComponent.h
 * @brief	3D���f���̕`��ɕK�v�ȏ����`����Component�B
 * 
 * @details	���[�h���ꂽ���f���̃��\�[�XID�A�e�N�X�`��ID�Ȃǂ�ێ�����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/28	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F���f��ID�ƃe�N�X�`��ID��ێ����� `ModelComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___MODEL_COMPONENT_H___
#define ___MODEL_COMPONENT_H___

// ===== �C���N���[�h =====
#include <cstdint>

 /**
  * @struct ModelComponent
  * @brief 3D���f���`����
  */
struct ModelComponent
{
	// 3D���f���̃��\�[�XID (Model::LoadModel�Ȃǂ��Ԃ�ID��z��)
	uint32_t ModelID;

	// ���f���`��Ŏg�p����e�N�X�`���̃��\�[�XID
	uint32_t TextureID;

	// �A�j���[�V�������g�p����ꍇ�̃A�j���[�V����ID (�����I�Ȋg���p)
	uint32_t AnimationID;

	/**
	 * @brief �R���X�g���N�^
	 */
	ModelComponent(
		uint32_t modelId = 0,
		uint32_t textureId = 0,
		uint32_t animationId = 0
	) : ModelID(modelId), TextureID(textureId), AnimationID(animationId)
	{}
};

#endif // !___MODEL_COMPONENT_H___