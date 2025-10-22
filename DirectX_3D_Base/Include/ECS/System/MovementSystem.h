#ifndef ___MOVEMENTSYSTEM_H___
#define ___MOVEMENTSYSTEM_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include "ECS/Component/Transform.h"

// ECS�R�A�ւ̈ˑ��́ASystem���N���X(Types.h)��Component�݂̂Ɍ���

/**
 * @class MovementSystem
 * @brief Entity�ɑ��x��K�p���A�ʒu���X�V����V�X�e��
 * @note ���̃V�X�e���� Transform Component �����S�Ă�Entity�������ΏۂƂ��܂��B
 */
class MovementSystem : public System
{
public:
    // Update�͏������z�֐��Ȃ̂ŕK������
    void Update(float deltaTime) override
    {
        // ECS�ł́ACoordinator����n���ꂽ�u�����Ώۂ�Entity ID�v�̏W�� (this->entities) �����[�v����
        for (const Entity entity : *entities)
        {
            // Coordinator����Transform Component���擾�i�����ł͉���Coordinator�Ƃ��ĐÓI�ȃO���[�o���ϐ����K�v�j
            // ���ۂ̎g�p�ł�Coordinator�̎Q�Ƃ�n�����A�O���[�o����Coordinator�𗘗p���܂��B

            // ���d�v�� �����ł�Coordinator���������̂��߁A�[���I�ɓ�����L�q���܂��B
            // ���ۂ� Coordinator::GetComponent<Transform>(entity) �̂悤�ȌĂяo�����K�v�ł��B

            // Transform* t = g_Coordinator->GetComponent<Transform>(entity); // ���ۂ̃R�[�h

            // ������: �SEntity��Y�������Ɉړ�������i�f���p�j
            // if (t) 
            // {
            //     t->position.y += 1.0f * deltaTime; 
            // }

            // �ǐ��̍����R�����g���L�q
            // ----------------------------------------------------------------------
            // NOTE: ����System��Coordinator�ɓo�^����ہA
            //       Coordinator::SetSystemSignature<MovementSystem>(signature); 
            //       �ɂāAMovementSystem�̏����Ώ�Component Signature��ݒ肷��K�v������܂��B
            // ----------------------------------------------------------------------
        }
    }
};

#endif // !___MOVEMENTSYSTEM_H___