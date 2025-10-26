/*****************************************************************//**
 * @file	CameraSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/24	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___CAMERASYSTEM_H___
#define ___CAMERASYSTEM_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include "ECS/Coordinator.h"
// Component�̈ˑ�
#include "ECS/Component/Transform.h"
#include "ECS/Component/Camera.h"
// �O���V�X�e���̈ˑ�
#include "Systems/Camera.h" // Camera�w���p�[�N���X

#include <DirectXMath.h>

// Coordinator�̃O���[�o���Q��
extern Coordinator* g_Coordinator;

/**
 * @class CameraSystem
 * @brief Transform��CameraSetting Component����ɁA�J������View/Projection�s����v�Z����V�X�e��
 * @note ���̃V�X�e���́AEntity�̋�ԏ��ƃJ�����ݒ�𕪗����A�f�[�^�쓮�ŃJ�������X�V���܂��B
 */
class CameraSystem : public System
{
private:
    // �J�����w���p�[�̃C���X�^���X (�s��v�Z�ɗ��p)
    // �`�揈���ōŐV�̌��ʂ��K�v�Ȃ��߁ASystem�̃����o�[�Ƃ��ĕێ����܂�
    Camera cameraHelper_;

public:
    // �v�Z���ꂽViewProj�s���RenderSystem�ɒ񋟂��邽�߂�Getter
    DirectX::XMMATRIX GetViewProjectionMatrix() const
    {
        return cameraHelper_.GetViewProjectionMatrix();
    }

    void Update(float deltaTime) override
    {
        if (entities->empty()) return;

        // --------------------------------------------------
        // 1. �S�ẴJ����Entity�ɑ΂��ă��[�v
        // --------------------------------------------------
        for (const Entity entity : *entities)
        {
            // Coordinator��ʂ���Component�f�[�^���擾
            Transform& t = g_Coordinator->GetComponent<Transform>(entity);
            CameraSetting& cs = g_Coordinator->GetComponent<CameraSetting>(entity);

            // --------------------------------------------------
            // 2. View�s��̌v�Z
            // --------------------------------------------------

            // a. ���_ (Eye) �̌v�Z: Transform.position�����̂܂ܗ��p
            DirectX::XMFLOAT3 eye = t.position;

            // b. �����_ (Target) �̌v�Z: 
            // �ȈՓI�ɁATransform�̉�]�Ɋ�Â����O���x�N�g�����v�Z���ALookAtOffset��K�p���܂��B
            DirectX::XMVECTOR forward = DirectX::XMVector3Transform(
                DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // ���[�J���O���x�N�g��
                DirectX::XMMatrixRotationRollPitchYaw( // Transform�̉�]��K�p
                    DirectX::XMConvertToRadians(t.rotation.x),
                    DirectX::XMConvertToRadians(t.rotation.y),
                    DirectX::XMConvertToRadians(t.rotation.z)
                )
            );

            DirectX::XMVECTOR targetVec = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&eye), forward);
            DirectX::XMFLOAT3 target;
            DirectX::XMStoreFloat3(&target, targetVec);

            // c. View�s��̍X�V
            cameraHelper_.UpdateView(eye, target, cs.upVector);

            // --------------------------------------------------
            // 3. Projection�s��̌v�Z (��ʃT�C�Y�ύX���Ɉ�x�s�������ŏ\��)
            // --------------------------------------------------
            cameraHelper_.UpdateProjection(
                cs.fieldOfViewY,
                cs.aspectRatio,
                cs.nearClip,
                cs.farClip
            );

            // --------------------------------------------------
            // 4. ���ʂ�Component�ɏ����߂�
            // --------------------------------------------------
            DirectX::XMStoreFloat4x4(&cs.viewMatrix, cameraHelper_.GetViewMatrix());
            DirectX::XMStoreFloat4x4(&cs.projectionMatrix, cameraHelper_.GetProjectionMatrix());
            DirectX::XMStoreFloat4x4(&cs.viewProjectionMatrix, cameraHelper_.GetViewProjectionMatrix());
        }
    }
};

#endif // !___CAMERASYSTEM_H___