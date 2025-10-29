/*****************************************************************//**
 * @file	CameraComponent.cpp
 * @brief	�J�����̐U�镑���ƃv���W�F�N�V�����ݒ���`����Component�B
 *
 * @details	�Ǐ]�Ώۂ�EntityID�A�Ǐ]�I�t�Z�b�g�A�Ǐ]���x�Ȃǂ̃f�[�^��ێ�����B
 *
 * ------------------------------------------------------------
 * @author	IwaiShogo
 * ------------------------------------------------------------
 *
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�J�����̒Ǐ]�ΏہA�I�t�Z�b�g�A�v���W�F�N�V�����ݒ��ێ�����'CameraComponent'���쐬�B
 *
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 *
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___CAMERA_COMPONENT_H___
#define ___CAMERA_COMPONENT_H___

// ===== �C���N���[�h =====
#include <DirectXMath.h>
#include "ECS/Types.h" // EntityID���g�p���邽��
#include "Main.h" // METER�}�N�����g�p

/**
 * @struct CameraComponent
 * @brief �Ǐ]���J�����̐U�镑���Ɛݒ�
 */
struct CameraComponent
{
	ECS::EntityID FocusEntityID;	///< �Ǐ]����Entity��ID
	DirectX::XMFLOAT3 Offset;		///< �Ǐ]�Ώۂ���̑��Έʒu�I�t�Z�b�g
	float FollowSpeed;				///< �Ǐ]�̕�ԑ��x (0.0f�`1.0f: �l���傫���قǒǏ]������)

	float FOV;						///< ����p (���W�A��)
	float NearClip;					///< �߃N���b�v����
	float FarClip;					///< ���N���b�v����

	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
	DirectX::XMFLOAT3	WorldPosition;

	/**
	 * @brief �R���X�g���N�^
	 */
	CameraComponent(
		ECS::EntityID focusID = ECS::INVALID_ENTITY_ID,
		DirectX::XMFLOAT3 offset = DirectX::XMFLOAT3(0.0f, METER(3.0f), METER(-5.0f)), // �v���C���[�̎΂ߌ�����z��
		float followSpeed = 0.1f, // �ɂ₩�ɒǏ]
		float fovDegrees = 60.0f,
		float nearClip = 0.1f,
		float farClip = 1000.0f
	) : FocusEntityID(focusID), Offset(offset), FollowSpeed(followSpeed), NearClip(nearClip), FarClip(farClip)
	{
		// ����p��x���烉�W�A���ɕϊ�
		FOV = DirectX::XMConvertToRadians(fovDegrees);
	}
};

#endif // !___CAMERA_COMPONENT_H___