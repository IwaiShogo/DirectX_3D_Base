/*****************************************************************//**
 * @file	MeshRendererComponent.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/20	����쐬��
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
#include <DirectXMath.h>
#include "../Component.h"
// ������DirectX���\�[�X�Ǘ��V�X�e���ւ̈ˑ���錾
// ���ۂ̃v���W�F�N�g�\���ɍ��킹�ăp�X�𒲐����Ă��������B
#include "Systems/DirectX/Texture.h" 
#include "Systems/DirectX/MeshBuffer.h" // �� �`��ɕK�v�ȃ��b�V�������K�v�Ƒz�肵�ǉ�

/**
 * @file MeshRenderer.h
 * @brief ���b�V���`��R���|�[�l���g�̒�`
 * @details ���̃R���|�[�l���g��3D�I�u�W�F�N�g�́u�����ځv�𐧌䂷��f�[�^�R���|�[�l���g�ł��B
 * @note �`�掩�̂́A���̃R���|�[�l���g�����G���e�B�e�B�ɑ΂���RenderSystem���s���܂��B
 */

/**
 * @struct MeshRenderer
 * @brief �I�u�W�F�N�g�̌����ځi�F�E�e�N�X�`���E���b�V���j���Ǘ�����f�[�^�R���|�[�l���g
 * @par �g�p��i�r���_�[�p�^�[���j:
 * @code
 * Entity cube = world.Create()
 * .With<Transform>(...)
 * .With<MeshRenderer>(MeshManager::CUBE_MESH_ID, DirectX::XMFLOAT3{1, 0, 0}) // �Ԃ��L���[�u
 * .Build();
 * @endcode
 */
DEFINE_DATA_COMPONENT(MeshRenderer,
    /**
     * @var meshId
     * @brief �`��Ɏg�p���郁�b�V����ID
     */
    MeshManager::MeshHandle meshId = MeshManager::INVALID_MESH;

    /**
     * @var color
     * @brief �P�F�`�掞�̐F�A�܂��̓e�N�X�`���̐F��
     */
    DirectX::XMFLOAT3 color{ 0.3f, 0.7f, 1.0f };
    
    /**
     * @var texture
     * @brief �\�ʂɓ\��t����e�N�X�`���摜�̃n���h��
     * @note �f�t�H���g��INVALID_TEXTURE�i�e�N�X�`���Ȃ��j
     */
    TextureManager::TextureHandle texture = TextureManager::INVALID_TEXTURE;
    
    /**
     * @var uvOffset
     * @brief UV���W�̃I�t�Z�b�g�i�e�N�X�`���ʒu�̂��炵�j�B�A�j���[�V�����Ɏg�p
     */
    DirectX::XMFLOAT2 uvOffset{ 0.0f, 0.0f };

    /**
     * @var uvScale
     * @brief UV���W�̃X�P�[���i�e�N�X�`���̌J��Ԃ��񐔁j
     */
    DirectX::XMFLOAT2 uvScale{ 1.0f, 1.0f };

    // �R���X�g���N�^
    MeshRenderer(
        MeshManager::MeshHandle mesh = MeshManager::INVALID_MESH,
        DirectX::XMFLOAT3 col = { 0.3f, 0.7f, 1.0f },
        TextureManager::TextureHandle tex = TextureManager::INVALID_TEXTURE)
        : meshId(mesh), color(col), texture(tex) {}
);

#endif // !___MESHRENDERER_H___