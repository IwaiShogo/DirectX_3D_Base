/*****************************************************************//**
 * @file	CollisionSystem.h
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

#ifndef ___COLLISIONSYSTEM_H___
#define ___COLLISIONSYSTEM_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include "ECS/Coordinator.h"
// Component�̈ˑ�
#include "ECS/Component/Transform.h"
#include "ECS/Component/Collider.h"

// �O���V�X�e���ւ̈ˑ�
#include "Systems/Geometory.h" // �����̏Փ˔���w���p�[�֐���z��
#include <DirectXMath.h>
#include <algorithm> // std::min, std::max�p

// Coordinator�̃O���[�o���Q��
extern Coordinator* g_Coordinator;

// --------------------------------------------------
// �w���p�[�\����: World AABB (�����s�o�E���f�B���O�{�b�N�X)
// --------------------------------------------------
// �Փ˔��胍�W�b�N��Geometory.h�ɓn�����߂̃��[���h���W�n�f�[�^�\��
struct WorldAABB
{
    DirectX::XMFLOAT3 min = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 max = { 0.0f, 0.0f, 0.0f };
};

/**
 * @class CollisionSystem
 * @brief Transform��Collider������Entity�Ԃ̏Փ˔�������s����V�X�e��
 * @note �����I�ȉ����i�����߂��Ȃǁj�́A���̃V�X�e���̌�Ɏ��s�����ʂ�PhysicsSystem���S�����ׂ��ł��B
 */
class CollisionSystem
    : public System
{
public:
    // Update�͏������z�֐��Ȃ̂ŕK������
    void Update(float deltaTime) override
    {
        if (entities->empty()) return;

        // --------------------------------------------------
        // 1. �S�Ă�Collider�̏Փ˃t���O�����Z�b�g
        // --------------------------------------------------
        for (const Entity entity : *entities)
        {
            g_Coordinator->GetComponent<Collider>(entity).collidedThisFrame = false;
        }

        // --------------------------------------------------
        // 2. �S�Ă�Entity�y�A�ɑ΂��ău���[�g�t�H�[�X�Փ˔�������s
        // --------------------------------------------------

        // Entity ID�̃Z�b�g���C�e���[�^���g���đ���
        auto itA = entities->begin();
        for (size_t i = 0; i < entities->size(); ++i, ++itA)
        {
            Entity entityA = *itA;
            Transform& tA = g_Coordinator->GetComponent<Transform>(entityA);
            Collider& cA = g_Coordinator->GetComponent<Collider>(entityA);

            // ���g�̌��Entity�݂̂��`�F�b�N (A vs B, B vs A�̏d���`�F�b�N�������)
            auto itB = std::next(itA);
            for (size_t j = i + 1; j < entities->size(); ++j, ++itB)
            {
                Entity entityB = *itB;
                Transform& tB = g_Coordinator->GetComponent<Transform>(entityB);
                Collider& cB = g_Coordinator->GetComponent<Collider>(entityB);

                // **Step 2-a: ���O���� (���C���[/�g���K�[)**
                if (cA.isKinematic && cB.isKinematic) continue; // �����L�l�}�e�B�b�N�Ȃ畨���I�ȏՓ˂𖳎�

                // **Step 2-b: ���[���h��Ԃł�AABB���v�Z**
                // Collider��local extent��Transform��world position��g�ݍ��킹��AABB���쐬 (��]�͖�������ȈՔ�)
                WorldAABB boxA;
                boxA.min.x = tA.position.x + cA.center.x - cA.extent.x;
                boxA.max.x = tA.position.x + cA.center.x + cA.extent.x;
                boxA.min.y = tA.position.y + cA.center.y - cA.extent.y;
                boxA.max.y = tA.position.y + cA.center.y + cA.extent.y;
                boxA.min.z = tA.position.z + cA.center.z - cA.extent.z;
                boxA.max.z = tA.position.z + cA.center.z + cA.extent.z;

                WorldAABB boxB; // Entity B�����l�Ɍv�Z (�ȗ�)

                // --------------------------------------------------
                // **Step 2-c: ���ۂ̏Փ˔��胍�W�b�N�̌Ăяo��**
                // --------------------------------------------------

                // �� ������Geometory::CheckAABBCollision(AABB_A, AABB_B)�Ƃ����֐������݂���Ɖ���
                // bool hit = Geometory::CheckAABBCollision(boxA, boxB); // �[���R�[�h

                // �����ł́A�Փ˔��肪���������Ɖ��肵�ăt���O��ݒ肵�܂�
                bool hit = false;
                // AABB�Փ˂̃��W�b�N�𒼐ڋL�q
                if (
                    boxA.min.x <= boxB.max.x && boxA.max.x >= boxB.min.x &&
                    boxA.min.y <= boxB.max.y && boxA.max.y >= boxB.min.y &&
                    boxA.min.z <= boxB.max.z && boxA.max.z >= boxB.min.z
                    )
                {
                    hit = true;
                }

                if (hit)
                {
                    // **Step 2-d: ���ʂ̏�������**
                    // �Փ˂������������Ƃ�Component�ɋL�^
                    cA.collidedThisFrame = true;
                    cB.collidedThisFrame = true;

                    // �� ���ۂ̃Q�[���ł́A�����ŏՓ˃C�x���g�𔭐�������K�v������܂�
                }
            }
        }
    }

    // CollisionSystem�̓��W�b�N�V�X�e���Ȃ̂�Initialize�͏ȗ��\
};

#endif // !___COLLISIONSYSTEM_H___