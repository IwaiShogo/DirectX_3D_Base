/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	ECS���[���h���̕`��\�ȃG���e�B�e�B��`�悷��V�X�e��
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

#ifndef ___RENDERSYSTEM_H___
#define ___RENDERSYSTEM_H___

// ===== �C���N���[�h =====
// --------------------------------------------------
// ECS �R�A
// --------------------------------------------------
#include "ECS/Types.h"
#include "ECS/Coordinator.h" // Coordinator�̎Q�Ƃ�K�v�Ƃ��邽�߁A�����ŃC���N���[�h

// �`��ɕK�v��Component
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"

// --------------------------------------------------
// DirectX �ˑ��V�X�e�� (RenderSystem�̎��s�ɕK�v�ȊO���T�[�r�X)
// --------------------------------------------------
#include "Systems/DirectX/DirectX.h"    
#include "Systems/DirectX/ShaderList.h" 
#include "Systems/DirectX/MeshBuffer.h"  
#include "Systems/DirectX/Texture.h"     

#include <DirectXMath.h>
#include <wrl/client.h>
#include <map>

// MeshBuffer�̊Ǘ��N���X�͂܂���`����Ă��܂��񂪁ADraw���\�b�h�̈����Ƃ��ĕK�v�ł��B
class MeshManager; // ���f���E���b�V�����Ǘ�����N���X (��)

// Coordinator�̃O���[�o���Q�� (GameScene���Ǘ����邱�Ƃ�z�肵�A�O���A�g�p�Ƃ��Ē�`)
extern Coordinator *g_pCoordinator;

/**
 * @class RenderSystem
 * @brief ECS���[���h���̕`��\�ȃG���e�B�e�B��`�悷��V�X�e��
 * @note System���N���X���p�����AUpdate(�܂���Draw)���\�b�h�Ń��W�b�N�����s���܂��B
 */
class RenderSystem
    : public System
{
public:
    // �`��ɕK�v�ȃV�F�[�_�[�̎�ށi�K�v�ɉ����ĊO������ݒ�\�j
    ShaderList::VSKind vsKind_ = ShaderList::VS_WORLD;
    ShaderList::PSKind psKind_ = ShaderList::PS_LAMBERT;

    // System���N���X�̏������z�֐��������iRenderSystem��Update�ł͉������Ȃ��j
    void Update(float deltaTime) override {}

    /**
     * @brief �`����s�̃��C���G���g���[�|�C���g (DirectX��DrawCall�����s)
     * @tparam MeshManager MeshBuffer�̊Ǘ��N���X�i�e���v���[�g�ɂ�薢��`�̂܂ܗ��p�\�j
     * @param[in] viewProjMatrix CameraSystem����v�Z���ꂽView * Projection�s��
     * @param[in] meshMgr MeshBuffer�Ǘ��N���X�̎Q�Ɓi�_�~�[�܂��͎��ۂ̃}�l�[�W���[�j
     */
    template<typename MeshManager>
    void Draw(const DirectX::XMMATRIX& viewProjMatrix, MeshManager& meshMgr)
    {
        // 1. �V�F�[�_�ƕ`��R���e�L�X�g�̐ݒ�
        ShaderList::GetVS(vsKind_)->Set();
        ShaderList::GetPS(psKind_)->Set();

        // 2. �S�Ă̕`��\�ȃG���e�B�e�B�ɑ΂��ă��[�v�iECS�̊j�S�j
        for (const Entity entity : *entities)
        {
            // Coordinator��ʂ��ăf�[�^�iComponent�j���擾
            Transform& t = g_pCoordinator->GetComponent<Transform>(entity);
            MeshRenderer& mr = g_pCoordinator->GetComponent<MeshRenderer>(entity);

            // a. World�s��̌v�Z��Transform�ւ̃L���b�V��
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t.rotation.x),
                DirectX::XMConvertToRadians(t.rotation.y),
                DirectX::XMConvertToRadians(t.rotation.z)
            );
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
            DirectX::XMMATRIX W = S * R * T;
            DirectX::XMStoreFloat4x4(&t.worldMatrix, W); // ���ʂ�Component�ɏ����߂�

            // b. WVP�s��̌v�Z
            DirectX::XMMATRIX WVP = W * viewProjMatrix; // CameraSystem����̍s����g�p

            // c. VS�萔�o�b�t�@�̍X�V (WVP���V�F�[�_�[�ɓ]��)
            DirectX::XMFLOAT4X4 WVP_T;
            DirectX::XMStoreFloat4x4(&WVP_T, DirectX::XMMatrixTranspose(WVP));
            ShaderList::SetWVP(&WVP_T);

            // d. PS�萔�o�b�t�@�̍X�V (Material/Color)
            // (mr.color, mr.textureId �Ȃǂ��g�p���ADirectX�̒萔�o�b�t�@��ݒ�)

            // e. �`����s (MeshBuffer::Draw���g�p)
            // MeshBuffer* meshBuf = meshMgr.GetMeshBuffer(mr.meshId); 
            // if (meshBuf && mr.isLoaded) meshBuf->Draw(); 
        }
    }
};

#endif // !___RENDERSYSTEM_H___