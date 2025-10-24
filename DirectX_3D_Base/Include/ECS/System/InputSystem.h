#pragma once

#include "ECS/Types.h"
#include "ECS/Coordinator.h"
#include "ECS/Component/Input.h"

// �����̓��͏����V�X�e���ւ̈ˑ�
#include "Systems/Input.h" // ������DirectX��Ղ̓��̓w���p�[�N���X (Input::IsKeyPresed()�Ȃǂ�z��)

// Coordinator�̃O���[�o���Q�� (GameScene.cpp�Ń|�C���^�[�Ƃ��Ē�`�ς�)
extern Coordinator* g_pCoordinator;

/**
 * @class InputSystem
 * @brief �O���̓��͏�Ԃ��Ď����AInput Component�ɂ��̏�Ԃ��������ރV�X�e��
 * @note ���̃V�X�e���́AInput Component�����S�Ă�Entity�������ΏۂƂ��܂��B
 */
class InputSystem : public System
{
public:
    // �Q�[���̓��͐ݒ�i�L�[�R�[�h�ƃA�N�V�����̃}�b�s���O�j�́A���̃N���X�̊O���܂��͓����Őݒ�\

    // System���N���X��Initialize�̓I�[�o�[���C�h���Ă��ǂ����A�����ł̓V���v����Update�̂�
    void Initialize() override
    {
        // �Ⴆ�΁A�}�E�X�J�[�\���̃��b�N/��\���Ȃǂ������ōs��
    }

    /**
     * @brief ���t���[���̓��͏�Ԃ�Input Component�ɏ�������
     * @param[in] deltaTime �O�t���[������̌o�ߎ��ԁi�b�j
     */
    void Update(float deltaTime) override
    {
        // ���̓f�[�^���擾����O�ɁAEntity�����݂��邩�m�F
        if (entities->empty()) return;

        // --------------------------------------------------
        // 1. �O���̃L�[�{�[�h/�}�E�X���͏�Ԃ��擾
        // --------------------------------------------------
        // ������ Input.h/Input.cpp �Ɉˑ�
        bool keyW = IsKeyPress('W'); // �O�i
        bool keyS = IsKeyPress('S'); // ���
        bool keyA = IsKeyPress('A'); // ���ړ�
        bool keyD = IsKeyPress('D'); // �E�ړ�
        bool keySpace = IsKeyPress(VK_SPACE); // �W�����v

        // �}�E�X�ړ��ʁi�����ł͋[���I�Ɏ擾�B���ۂ�Input�V�X�e������擾���K�v�j
        float mouseX = 0.0f; // Input::GetMouseDeltaX();
        float mouseY = 0.0f; // Input::GetMouseDeltaY();


        // --------------------------------------------------
        // 2. �����Ώۂ�Entity�iInput Component�������́j�ɑ΂��ă��[�v
        // --------------------------------------------------
        for (const Entity entity : *entities)
        {
            // Coordinator��ʂ���Input Component�f�[�^���擾
            // g_Coordinator�̓|�C���^�[�Ȃ̂� -> �ŃA�N�Z�X
            Input& input = g_pCoordinator->GetComponent<Input>(entity);

            // a. �����̒l�����Z�b�g
            if (input.shouldResetAfterUse)
            {
                // �ړ��x�N�g���͉��Z�����\�������邽�߁A�����ł̓t���O�̂݃��Z�b�g����݌v���\�����A
                // MovementSystem�Ƃ̘A�g���l���A����͓���Component�����S�ɍX�V���܂��B
                input.movementVector = { 0.0f, 0.0f, 0.0f };
                input.mouseDeltaX = 0.0f;
                input.mouseDeltaY = 0.0f;
            }

            // b. �L�[��Ԃ̏�������
            input.isMovingForward = keyW;
            input.isMovingBackward = keyS;
            input.isMovingLeft = keyA;
            input.isMovingRight = keyD;
            input.isJumpPressed = keySpace;

            // c. �ړ��x�N�g���̌v�Z
            if (keyW) input.movementVector.z += 1.0f;
            if (keyS) input.movementVector.z -= 1.0f;
            if (keyA) input.movementVector.x -= 1.0f;
            if (keyD) input.movementVector.x += 1.0f;

            // �ړ��x�N�g���𐳋K���i�΂߈ړ��̑��x�𓙂������邽�߁j
            if (DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&input.movementVector)).m128_f32[0] > 0.0f)
            {
                DirectX::XMStoreFloat3(&input.movementVector,
                    DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&input.movementVector)));
            }

            // d. �}�E�X�ړ��ʂ̏�������
            input.mouseDeltaX = mouseX;
            input.mouseDeltaY = mouseY;

            // Input Component�f�[�^���X�V����܂����B
        }
    }
};