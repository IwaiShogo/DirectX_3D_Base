#ifndef ___CAMERA_H___
#define ___CAMERA_H___


#include <DirectXMath.h>

/**
 * @class Camera
 * @brief View行列とProjection行列の計算を行うヘルパークラス
 * @note このクラスはECS Componentではありません。ECS Component (CameraSetting)のデータを基に、
 * CameraSystemがこのヘルパークラスを利用し、行列を計算します。
 */
class Camera
{
private:
    DirectX::XMMATRIX viewMatrix_;
    DirectX::XMMATRIX projectionMatrix_;

public:
    Camera()
    {
        viewMatrix_ = DirectX::XMMatrixIdentity();
        projectionMatrix_ = DirectX::XMMatrixIdentity();
    }

    // --------------------------------------------------
    // 行列計算
    // --------------------------------------------------

    /**
     * @brief View行列を計算し、内部にキャッシュする
     * @param[in] eye 視点座標
     * @param[in] target 注視点座標
     * @param[in] up 上方向ベクトル
     */
    void UpdateView(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
    {
        DirectX::XMVECTOR eyePos = DirectX::XMLoadFloat3(&eye);
        DirectX::XMVECTOR lookAt = DirectX::XMLoadFloat3(&target);
        DirectX::XMVECTOR upDir = DirectX::XMLoadFloat3(&up);

        viewMatrix_ = DirectX::XMMatrixLookAtLH(eyePos, lookAt, upDir);
    }

    /**
     * @brief Projection行列を計算し、内部にキャッシュする
     * @param[in] fovY 視野角（ラジアン）
     * @param[in] aspect アスペクト比 (width / height)
     * @param[in] nearZ ニアクリップ距離
     * @param[in] farZ ファークリップ距離
     */
    void UpdateProjection(float fovY, float aspect, float nearZ, float farZ)
    {
        // DirectXMathでは、XMMatrixPerspectiveFovLH (左手座標系) を使用
        projectionMatrix_ = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    }

    // --------------------------------------------------
    // Getter
    // --------------------------------------------------

    DirectX::XMMATRIX GetViewMatrix() const { return viewMatrix_; }
    DirectX::XMMATRIX GetProjectionMatrix() const { return projectionMatrix_; }
    DirectX::XMMATRIX GetViewProjectionMatrix() const { return viewMatrix_ * projectionMatrix_; }

    // ※ 行列の転置が必要な場合は、RenderSystemまたはShaderList側で行うことを推奨
};

#endif // !___CAMERA_H___