/*****************************************************************//**
 * @file	MeshRenderer.h
 * @brief	Entity���ǂ�3D���f����`�悷�ׂ������w�肵�܂��B
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

#ifndef ___MESHRENDERER_H___
#define ___MESHRENDERER_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include <cstdint>
#include <DirectXMath.h>

/**
 * @struct MeshRenderer
 * @brief Entity��`�悷�邽�߂ɕK�v�ȃ��b�V���ƃe�N�X�`���̏���ێ�����R���|�[�l���g
 * @note ������DirectX�x�[�X�̃R�[�h�ƘA�g���邽�߁AID�x�[�X�Ń��\�[�X���Q�Ƃ��܂��B
 */
struct MeshRenderer : public IComponent
{
    // MeshBuffer/Model�Ǘ��N���X���烁�b�V���f�[�^���擾���邽�߂�ID
    // ID 0�͖����ȃ��b�V���A�܂��̓f�t�H���g���b�V�����Ӗ�����Ƒz��
    std::uint32_t meshId = 0;

    // TextureManager����e�N�X�`�����擾���邽�߂�ID
    std::uint32_t textureId = 0;

    // �ȈՓI�ȃ}�e���A���J���[ (DirectX�̒萔�o�b�t�@�ɓn��)
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA

    // ���b�V�������[�h���ꂽ���ǂ����̃t���O
    bool isLoaded = false;
};

#endif // !___MESHRENDERER_H___