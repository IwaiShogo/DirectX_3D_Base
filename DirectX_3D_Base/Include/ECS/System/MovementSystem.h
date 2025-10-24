#ifndef ___MOVEMENTSYSTEM_H___
#define ___MOVEMENTSYSTEM_H___

// ===== �C���N���[�h =====
#include "ECS/Types.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/Input.h"

// ECS�R�A�ւ̈ˑ��́ASystem���N���X(Types.h)��Component�݂̂Ɍ���

// Coordinator�̃O���[�o���Q�� (GameScene.cpp�Ń|�C���^�[�Ƃ��Ē�`�ς�)
extern Coordinator* g_pCoordinator;

/**
 * @class MovementSystem
 * @brief Entity�ɑ��x��K�p���A�ʒu���X�V����V�X�e��
 * @note ���̃V�X�e���� Transform Component �����S�Ă�Entity�������ΏۂƂ��܂��B
 */
class MovementSystem 
    : public System
{
public:
    // Update�͏������z�֐��Ȃ̂ŕK������
    void Update(float deltaTime) override
    {
        // ECS�ł́ACoordinator����n���ꂽ�u�����Ώۂ�Entity ID�v�̏W�� (this->entities) �����[�v����
        for (const Entity entity : *entities)
        {
            // �����Ώۂ�Entity ID�̏W�� (this->entities) �����[�v����
            for (const Entity entity : *entities)
            {
                // Coordinator��ʂ���Component�f�[�^���擾
                // Transform�͏������� (in/out)
                Transform& t = g_pCoordinator->GetComponent<Transform>(entity);
                // Input�͓ǂݎ�� (in)
                Input& input = g_pCoordinator->GetComponent<Input>(entity);

                // --------------------------------------------------
                // 1. �ړ��x�N�g���̓K�p
                // --------------------------------------------------

                // Input Component����ړ��x�N�g�����擾
                DirectX::XMVECTOR moveVec = DirectX::XMLoadFloat3(&input.movementVector);

                // �ړ��x�N�g�����[���ł͂Ȃ����`�F�b�N
                if (DirectX::XMVector3LengthSq(moveVec).m128_f32[0] > 0.0f)
                {
                    // �ړ��� = �ړ��x�N�g�� * ���x * �f���^�^�C��
                    float scalar = MOVEMENT_SPEED * deltaTime;
                    DirectX::XMVECTOR deltaPos = DirectX::XMVectorScale(moveVec, scalar);

                    // ���݂̈ʒu�Ɉړ��ʂ����Z
                    DirectX::XMVECTOR currentPos = DirectX::XMLoadFloat3(&t.position);
                    DirectX::XMVECTOR newPos = DirectX::XMVectorAdd(currentPos, deltaPos);

                    // ���ʂ�Transform Component�ɏ����߂�
                    DirectX::XMStoreFloat3(&t.position, newPos);
                }

                // --------------------------------------------------
                // 2. ���̓f�[�^�̏��� (���̃t���[���̂��߂Ƀ��Z�b�g)
                // --------------------------------------------------

                // MovementSystem���������I������AInput Component�̃f�[�^�����Z�b�g���܂��B
                // ����ɂ��A�L�[�������Ă��Ȃ��t���[���ł͈ړ�����~���܂��B
                if (input.shouldResetAfterUse)
                {
                    input.movementVector = { 0.0f, 0.0f, 0.0f };
                    input.isMovingForward = false;
                    input.isMovingBackward = false;
                    input.isMovingLeft = false;
                    input.isMovingRight = false;
                    input.isJumpPressed = false;
                    // �}�E�X���͂͒ʏ�AInputSystem���Ń��Z�b�g���邩�AMovementSystem���Ŏg�p��Ƀ��Z�b�g���܂�
                    // �����InputSystem���Ń��Z�b�g���W�b�N���������Ȃ��������߁AMovementSystem���ň�U�����ς݂Ƃ��ăt���O��|�������ł��ǂ��B
                }
            }
        }
    }

private:
    // �Q�[���̈ړ����x
    const float MOVEMENT_SPEED = 15.0f;
};

#endif // !___MOVEMENTSYSTEM_H___