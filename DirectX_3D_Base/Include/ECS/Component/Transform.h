/*****************************************************************//**
 * @file	TransformComponent.h
 * @brief	�ʒu�E��]�E�X�P�[���R���|�[�l���g�̒�`
 * 
 * @details	���̃R���|�[�l���g��3D��Ԃɂ�����u�ꏊ�v�u�����v�u�傫���v
 *			��\����{�f�[�^�ł��B
 *			���ׂĂ�3D�I�u�W�F�N�g�ɕK�v�ƂȂ��ԏ���ێ����A
 *			����System�̌v�Z��ՂƂȂ�܂��B
 * 
 *			### ���W�n�ɂ��āF
 *			X���F�E���������AY���F����������AZ���F����������
 *			�iDirectX�̍�����W�n��z��j
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/17	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___TRANSFORMCOMPONENT_H___
#define ___TRANSFORMCOMPONENT_H___

// ===== �C���N���[�h =====
#include <DirectXMath.h>
#include "ECS/Component.h"	// DEFINE_DATA_COMPONENT���g�p

/**
 * @struct	Transform
 * @brief	3D��Ԃɂ�����G���e�B�e�B�̈ʒu�E��]�E�X�P�[�����Ǘ�����f�[�^�R���|�[�l���g
 */
DEFINE_DATA_COMPONENT(Transform,
	/**
	 * @var		position
	 * @brief	�G���e�B�e�B��3D��Ԃɂ�����ʒu���W
	 */
	DirectX::XMFLOAT3 position{ 0, 0, 0 };

	/**
	 * @var		rotation
	 * @brief	�G���e�B�e�B�̉�]�p�x�i�x���@�j
	 * 
	 * @details	�I�C���[�p�ɂ���]��\���܂��B
	 *			- rotation.x: �s�b�`�iX������j
	 *			- rotation.y: ���[�iY������j
	 *			- rotation.z: ���[���iZ������j
	 * 
	 * @note	�p�x��**�x���@**�i0-360�x�j�Ŏw�肵�܂��BWorld���̍s��v�Z���Ƀ��W�A���Ɏ����ϊ�����܂��B
	 * @warning �傫�ȉ�]���s���ꍇ�A**�W���o�����b�N**����������\��������܂��B
	 */
	DirectX::XMFLOAT3 rotation{ 0, 0, 0 };

	/**
	 * @var		scale
	 * @brief	�G���e�B�e�B�̃X�P�[���i�傫���j
	 */
	DirectX::XMFLOAT3 scale{ 1, 1, 1 };

	/**
	 * @var		worldMatrix
	 * @brief	�ʒu�E��]�E�X�P�[������v�Z���ꂽ���[���h�s��
	 * 
	 * @details	���̍s��́ARenderSystem�Ȃǂ̃V�X�e���ɂ���Čv�Z����AGPU�ɓn����܂��B
	 *			�ʏ�A���[�U�[�R�[�h����͓ǂݎ���p�Ƃ��Ĉ����܂��B
	 */
	DirectX::XMFLOAT4X4 worldMatrix;

	/**
	 * [int - Transform]
	 * @brief	�S�p�����[�^���w�肷��R���X�g���N�^
	 * 
	 * @param	[in] pos	�����ʒu
	 * @param	[in] rot	������]�p�x�i�x���@�j
	 * @param	[in] scl	�����X�P�[��
	 * 
	 * @par		�g�p��i�s���_�[�p�^�[���j�F
	 * @code
	 *	// Entity Builder��.With<>�ŏ������Ɏg�p
	 *	Entity cube = w.Create()
	 *		.With<Transform>(
	 *			DirectX::XMFLOAT3{10, 5, 0},	// �ʒu
	 *			DirectX::XMFLOAT3{0, 45, 0},	// ��]
	 *			DirectX::XMFLOAT3{2, 2, 2}		// �X�P�[��
	 *		)
	 *		.Build();
	 * @endcode	
	 */
	Transform(DirectX::XMFLOAT3 pos = { 0,0,0 }, DirectX::XMFLOAT3 rot = { 0,0,0 }, DirectX::XMFLOAT3 scl = { 1,1,1 })
		: position(pos), rotation(rot), scale(scl)
	{
		// WorldMatrix��P�ʍs��ŏ�����
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	}
);

#endif // !___TRANSFORMCOMPONENT_H___