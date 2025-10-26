#pragma once

#include "ECS/Types.h"
#include <DirectXMath.h>

/**
 * @struct CameraSetting
 * @brief カメラの投影設定およびView行列の計算結果を保持するコンポーネント
 * @note Transform Componentと組み合わせて使用されます。
 */
struct CameraSetting : public IComponent
{
    // --------------------------------------------------
    // Projection (投影) 設定
    // --------------------------------------------------
    float fieldOfViewY = 0.785398f; // 45度 (ラジアン)
    float aspectRatio = 16.0f / 9.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;

    // --------------------------------------------------
    // View (視点) 設定 (注視点、上方向)
    // --------------------------------------------------
    DirectX::XMFLOAT3 lookAtOffset = { 0.0f, 0.0f, 1.0f }; // Transform.positionからの相対的な注視点オフセット
    DirectX::XMFLOAT3 upVector = { 0.0f, 1.0f, 0.0f };

    // --------------------------------------------------
    // 結果 (CameraSystemによって毎フレーム計算・更新される)
    // --------------------------------------------------
    // World/View/Projection行列を保持することで、他のSystem（RenderSystemなど）がすぐに利用できるようにします。
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