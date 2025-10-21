#pragma once
// --------------------------------------------------
// �C���N���[�h�p�X�C��
// RenderSystem.h �̈ʒu: Include/ECS/System/
// DirectX.h �̈ʒu: Include/Systems/DirectX/
// --------------------------------------------------
#include "Systems/DirectX/DirectX.h"    // GetContext(), GetDevice()
#include "Systems/DirectX/ShaderList.h" // SetWVP, SetMaterial, SetLight...
#include "Systems/DirectX/MeshBuffer.h"  // MeshBuffer::Draw()
#include "Systems/DirectX/Texture.h"     // TextureManager::INVALID_TEXTURE

// ECS�R�A (���� Include/ECS/ ����̑��΃p�X)
#include "ECS/World.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"

// Camera�̈ˑ�������
//#include "Camera.h" // ��L�Œ�`����Camera.h (Include/ECS/System/Camera.h)

#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstring>
#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")

// ������MeshBuffer��Draw���ĂԂ��߂̃��b�p�[�i�����ł͒P�Ȃ�O���錾�j
class MeshBuffer;
class Camera; // RenderSystem���ˑ�����Camera�N���X

/**
 * @struct RenderSystem
 * @brief ECS���[���h���̕`��\�ȃG���e�B�e�B��`�悷��V�X�e��
 */
struct RenderSystem
{
    // �ˑ��֌W�������N���X���Ɛ���
    // DirectX.h ���O���[�o���֐����g�p���邽�߁A�����ł͈ˑ����ŏ����ɂ��܂�
    // Camera��Draw���\�b�h���Ŏg�p

    // �� ������MeshBuffer�N���X�̓C���X�^���X�Ǘ����K�v�Ȃ��߁A�����ł�Draw���Ɏ擾���邱�Ƃ�z��

    // �萔�o�b�t�@��ShaderList���Ǘ����Ă��邽�߁ARenderSystem�ł�WRL::ComPtr�͕s�v�ł��B
    // ShaderList::SetWVP() ���O���[�o���Ȓ萔�o�b�t�@���X�V����Ƒz�肳��܂��B

    // RenderSystem��RenderSystem::Draw()�O�ŊǗ����ׂ����\�[�X�i�V�F�[�_�A�萔�o�b�t�@�j
    // ShaderList::VSKind/PSKind ��ێ�
    ShaderList::VSKind vsKind_ = ShaderList::VS_WORLD;
    ShaderList::PSKind psKind_ = ShaderList::PS_LAMBERT;

    // �� Draw���\�b�h����DrawCall���œK�����邽�߂ɁA�`��Ɏg�p���郁�b�V���̃|�C���^��ID��ێ�����@�\���K�v�ł��B
    //    �����ł́ADraw���\�b�h�̈����Ƃ���MeshBuffer�̊Ǘ��N���X��n�����Ƃ�O��Ƃ��܂��B

    /**
     * @brief �`����s�̃��C���G���g���[�|�C���g
     * @param[in,out] w World�ւ̎Q��
     * @param[in] cam ���݂̃V�[���J����
     * @param[in] meshMgr MeshBuffer�Ǘ��N���X�i��: MeshManager�j
     */
    template<typename MeshManager>
    void Draw(World& w, Camera& cam, MeshManager& meshMgr)
    {
        // 1. �V�F�[�_�ƕ`��R���e�L�X�g�̐ݒ� (ShaderList.h���g�p)
        ShaderList::GetVS(vsKind_)->Set();
        ShaderList::GetPS(psKind_)->Set();

        // 2. �J�����s��̎擾
        DirectX::XMMATRIX view = cam.GetViewMatrix();
        DirectX::XMMATRIX proj = cam.GetProjectionMatrix();
        DirectX::XMMATRIX viewProj = view * proj;

        // 3. �S�Ă̕`��\�ȃG���e�B�e�B�ɑ΂��ă��[�v
        Store<Transform>* transformStore = w.GetStore<Transform>();
        Store<MeshRenderer>* meshStore = w.GetStore<MeshRenderer>();

        for (const auto& [id, mr] : meshStore->GetAll())
        {
            Entity e = { id };
            Transform* t = transformStore->TryGet(e);
            if (!t) continue;

            // a. World�s��̌v�Z��Transform�ւ̃L���b�V��
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t->scale.x, t->scale.y, t->scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t->rotation.x),
                DirectX::XMConvertToRadians(t->rotation.y),
                DirectX::XMConvertToRadians(t->rotation.z));
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t->position.x, t->position.y, t->position.z);
            DirectX::XMMATRIX W = S * R * T;
            DirectX::XMStoreFloat4x4(&t->worldMatrix, W);

            DirectX::XMMATRIX WVP = W * viewProj;

            // b. VS�萔�o�b�t�@�̍X�V (ShaderList::SetWVP���g�p)
            // ShaderList::SetWVP��XMFLOAT4X4�����҂��Ă��邽�ߕϊ�
            DirectX::XMFLOAT4X4 WVP_T;
            DirectX::XMStoreFloat4x4(&WVP_T, DirectX::XMMatrixTranspose(WVP));
            ShaderList::SetWVP(&WVP_T);

            // c. PS�萔�o�b�t�@�̍X�V (ShaderList::SetMaterial���g�p)
            // MeshRenderer�̃f�[�^����ShaderList::SetMaterial�ɓn��Material�\���̂��\�z (Model::Material���K�v)
            // Model::Material�̒�`���Ȃ����߁A�����ł͐F�ƃe�N�X�`���̎g�p�t���O�݂̂�n���Ɖ���
            // ShaderList::SetMaterial(mr.color, (mr.texture != TextureManager::INVALID_TEXTURE));

            // �� ShaderList::SetMaterial��Model::Material�����҂��Ă��܂��B�����ł�RenderSystem�̃R�[�h���ȗ������AMeshRenderer�̃f�[�^�𗘗p���邱�Ƃɏd�_��u���܂��B
            // ������SetMaterial��ECS�f�[�^�ɍ��킹�邽�߂ɁA������Model::Material�\���̂��`���邩�AShaderList��ύX����K�v������܂��B
            // �����ł́AMeshRenderer�𒼐ڗ��p����悤�A�ȗ������ꂽ�[���I�ȌĂяo�����g�p���܂��B

            // d. �e�N�X�`���̐ݒ�iTexture::GetResource()��DirectX::SetShaderResources���g�p�j
            // Texture* tex = TextureManager::GetTexture(mr.texture); // TextureManager�N���X���ʓr�K�v
            // if (tex) GetContext()->PSSetShaderResources(0, 1, tex->GetResource());

            // e. �`����s (MeshBuffer::Draw���g�p)
            // MeshBuffer* meshBuf = meshMgr.GetMeshBuffer(mr.meshId); // MeshManager�N���X���ʓr�K�v
            // if (meshBuf) meshBuf->Draw();
        }
    }
};