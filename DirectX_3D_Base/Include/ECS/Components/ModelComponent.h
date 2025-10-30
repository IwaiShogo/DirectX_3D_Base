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
#include "Systems/Model.h"

 /**
  * @struct ModelComponent
  * @brief 3D���f���`����
  */
struct ModelComponent
{
	// 3D���f���̃��\�[�XID (Model::LoadModel�Ȃǂ��Ԃ�ID��z��)
	uint32_t ModelID;

	std::unique_ptr<Model> pModel = std::make_unique<Model>();

	// ���f���`��Ŏg�p����e�N�X�`���̃��\�[�XID
	uint32_t TextureID;

	// �A�j���[�V�������g�p����ꍇ�̃A�j���[�V����ID (�����I�Ȋg���p)
	uint32_t AnimationID;

	/**
	 * @brief �R���X�g���N�^
	 */
	ModelComponent() = default;

	// ���[�U�[���쐬���������t���R���X�g���N�^
	ModelComponent(const char* path, float scale, Model::Flip flip)
	{
		if (!pModel->Load(path, scale, flip))
		{
			MessageBox(NULL, "���f���̃��[�h�Ɏ��s", "Error", MB_OK);
		}
	}
};

#endif // !___MODEL_COMPONENT_H___