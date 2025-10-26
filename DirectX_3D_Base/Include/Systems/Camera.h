#ifndef ___CAMERA_H___
#define ___CAMERA_H___


#include <DirectXMath.h>

/**
 * @class Camera
 * @brief View�s���Projection�s��̌v�Z���s���w���p�[�N���X
 * @note ���̃N���X��ECS Component�ł͂���܂���BECS Component (CameraSetting)�̃f�[�^����ɁA
 * CameraSystem�����̃w���p�[�N���X�𗘗p���A�s����v�Z���܂��B
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
    // �s��v�Z
    // --------------------------------------------------

    /**
     * @brief View�s����v�Z���A�����ɃL���b�V������
     * @param[in] eye ���_���W
     * @param[in] target �����_���W
     * @param[in] up ������x�N�g��
     */
    void UpdateView(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
    {
        DirectX::XMVECTOR eyePos = DirectX::XMLoadFloat3(&eye);
        DirectX::XMVECTOR lookAt = DirectX::XMLoadFloat3(&target);
        DirectX::XMVECTOR upDir = DirectX::XMLoadFloat3(&up);

        viewMatrix_ = DirectX::XMMatrixLookAtLH(eyePos, lookAt, upDir);
    }

    /**
     * @brief Projection�s����v�Z���A�����ɃL���b�V������
     * @param[in] fovY ����p�i���W�A���j
     * @param[in] aspect �A�X�y�N�g�� (width / height)
     * @param[in] nearZ �j�A�N���b�v����
     * @param[in] farZ �t�@�[�N���b�v����
     */
    void UpdateProjection(float fovY, float aspect, float nearZ, float farZ)
    {
        // DirectXMath�ł́AXMMatrixPerspectiveFovLH (������W�n) ���g�p
        projectionMatrix_ = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    }

    // --------------------------------------------------
    // Getter
    // --------------------------------------------------

    DirectX::XMMATRIX GetViewMatrix() const { return viewMatrix_; }
    DirectX::XMMATRIX GetProjectionMatrix() const { return projectionMatrix_; }
    DirectX::XMMATRIX GetViewProjectionMatrix() const { return viewMatrix_ * projectionMatrix_; }

    // �� �s��̓]�u���K�v�ȏꍇ�́ARenderSystem�܂���ShaderList���ōs�����Ƃ𐄏�
};

#endif // !___CAMERA_H___