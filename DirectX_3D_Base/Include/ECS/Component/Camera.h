#pragma once

#include "ECS/Types.h"
#include <DirectXMath.h>

/**
 * @struct CameraSetting
 * @brief �J�����̓��e�ݒ肨���View�s��̌v�Z���ʂ�ێ�����R���|�[�l���g
 * @note Transform Component�Ƒg�ݍ��킹�Ďg�p����܂��B
 */
struct CameraSetting : public IComponent
{
    // --------------------------------------------------
    // Projection (���e) �ݒ�
    // --------------------------------------------------
    float fieldOfViewY = 0.785398f; // 45�x (���W�A��)
    float aspectRatio = 16.0f / 9.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;

    // --------------------------------------------------
    // View (���_) �ݒ� (�����_�A�����)
    // --------------------------------------------------
    DirectX::XMFLOAT3 lookAtOffset = { 0.0f, 0.0f, 1.0f }; // Transform.position����̑��ΓI�Ȓ����_�I�t�Z�b�g
    DirectX::XMFLOAT3 upVector = { 0.0f, 1.0f, 0.0f };

    // --------------------------------------------------
    // ���� (CameraSystem�ɂ���Ė��t���[���v�Z�E�X�V�����)
    // --------------------------------------------------
    // World/View/Projection�s���ێ����邱�ƂŁA����System�iRenderSystem�Ȃǁj�������ɗ��p�ł���悤�ɂ��܂��B
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projectionMatrix;
    DirectX::XMFLOAT4X4 viewProjectionMatrix;

    CameraSetting()
    {
        DirectX::XMStoreFloat4x4(&viewMatrix, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&projectionMatrix, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&viewProjectionMatrix, DirectX::XMMatrixIdentity());
    }
};