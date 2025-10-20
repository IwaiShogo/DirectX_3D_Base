#pragma once
#include "graphics/GfxDevice.h" // DirectX�f�o�C�X�ւ̃A�N�Z�X
#include "graphics/Camera.h"   // �J�����iView/Proj�s��j�ւ̃A�N�Z�X
#include "ECS/World.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"
#include <DirectXMath.h>
// ... ���ɕK�v��DirectX/Shader�̃w�b�_�[

/**
 * @file RenderSystem.h
 * @brief ���b�V�������_�����O�V�X�e��
 * @details World����MeshRenderer�R���|�[�l���g�����G���e�B�e�B��`�悵�܂��B
 */

 /**
  * @struct RenderSystem
  * @brief ECS���[���h���̕`��\�ȃG���e�B�e�B��`�悷��V�X�e��
  * @note ���̃V�X�e����World::Update()�Ƃ͓Ɨ����āA���C�����[�v���疾���I��Draw()���Ăяo���K�v������܂��B
  */
struct RenderSystem
{
    // �����R�[�h�̈ˑ��֌W�������o�ϐ��Ƃ��ĕێ����邱�Ƃ�z��
    GfxDevice& gfx_;
    Camera& cam_;
    // ... �萔�o�b�t�@��V�F�[�_�֘A�̃����o�������ɒ�`

    // �萔�o�b�t�@�̍\���� (RenderSystem.h��RenderSystem::Update�̃R�[�h���琄��)
    struct VSConstants {
        DirectX::XMMATRIX WVP;
        DirectX::XMFLOAT4 uvTransform;
    };
    struct PSConstants {
        DirectX::XMFLOAT4 color;
        float useTexture;
        DirectX::XMFLOAT3 padding;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> cb_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> psCb_;
    // ... Shader��InputLayout�̃����o���K�v

    /**
     * @brief ������
     */
    RenderSystem(GfxDevice& gfx, Camera& cam) : gfx_(gfx), cam_(cam)
    {
        // �����R�[�h����Shader/ConstantBuffer�̏��������W�b�N���ڐA
        // ��: cb_ = GfxDevice::CreateConstantBuffer(sizeof(VSConstants));
    }

    /**
     * @brief �`����s�̃��C���G���g���[�|�C���g
     * @param[in,out] w World�ւ̎Q��
     * * @details
     * ���̃��\�b�h�́A�Q�[���̃��C�����[�v�܂���GameScene::Draw()���ŌĂяo����܂��B
     */
    void Draw(World& w)
    {
        // 1. �J�����s��̎擾�Ɛݒ�iView/Proj�j
        DirectX::XMMATRIX view = cam_.GetViewMatrix();
        DirectX::XMMATRIX proj = cam_.GetProjectionMatrix();

        // 2. �`��R���e�L�X�g�̐ݒ� (InputLayout, Shader�̃Z�b�g�A�b�v)
        // ... �����R�[�h����V�F�[�_�ݒ���ڐA

        // 3. �S�Ă̕`��\�ȃG���e�B�e�B�ɑ΂��ă��[�v
        Store<Transform>* transformStore = w.GetStore<Transform>();
        Store<MeshRenderer>* meshStore = w.GetStore<MeshRenderer>();

        for (const auto& [id, mr] : meshStore->GetAll())
        {
            Entity e = { id };
            Transform* t = transformStore->TryGet(e);
            if (!t) continue; // Transform���Ȃ��G���e�B�e�B�͕`�悵�Ȃ�

            // a. World�s��̌v�Z�Ɛݒ� (RenderSystem.h�̃R�[�h���Q�l�Ɏ���)
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t->scale.x, t->scale.y, t->scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t->rotation.x),
                DirectX::XMConvertToRadians(t->rotation.y),
                DirectX::XMConvertToRadians(t->rotation.z));
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t->position.x, t->position.y, t->position.z);
            DirectX::XMMATRIX W = S * R * T;

            // WorldMatrix��Transform�ɃL���b�V�� (�I�v�V����)
            DirectX::XMStoreFloat4x4(&t->worldMatrix, W);

            // b. VS�萔�o�b�t�@�̍X�V (WVP�s��, UV�ϊ�)
            VSConstants vsCbuf;
            vsCbuf.WVP = DirectX::XMMatrixTranspose(W * view * proj);
            vsCbuf.uvTransform = DirectX::XMFLOAT4{ mr.uvOffset.x, mr.uvOffset.y, mr.uvScale.x, mr.uvScale.y };
            gfx_.Ctx()->UpdateSubresource(cb_.Get(), 0, nullptr, &vsCbuf, 0, 0);

            // c. PS�萔�o�b�t�@�̍X�V (�F, �e�N�X�`���g�p�t���O)
            PSConstants psCbuf;
            psCbuf.color = DirectX::XMFLOAT4{ mr.color.x, mr.color.y, mr.color.z, 1.0f };
            psCbuf.useTexture = (mr.texture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
            gfx_.Ctx()->UpdateSubresource(psCb_.Get(), 0, nullptr, &psCbuf, 0, 0);

            // d. �e�N�X�`���̐ݒ�
            // TextureManager::BindTexture(mr.texture); // �������\�[�X�}�l�[�W������ăo�C���h

            // e. �`����s
            // MeshManager::DrawMesh(mr.meshId); // �������\�[�X�}�l�[�W������ăh���[�R�[��
        }
    }
};