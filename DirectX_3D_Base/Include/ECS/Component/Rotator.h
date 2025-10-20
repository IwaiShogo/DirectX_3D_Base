/*****************************************************************//**
 * @file	RotatorComponent.h
 * @brief	������]Behaviour�R���|�[�l���g�̒�`
 * 
 * @details	���̃t�@�C���́A�G���e�B�e�B�������I�ɉ�]������**Behaviour�R���|�[�l���g**���`���܂��B
 *          Behaviour�R���|�[�l���g�̊�{�I�Ȏ�����Ƃ��āA�w�K�ɍœK�ł��B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/20	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ROTATOR_H___
#define ___ROTATOR_H___

// ===== �C���N���[�h =====
#include "ECS/Component.h"
#include "ECS/Entity.h"
#include "ECS/World.h"
#include "ECS/Component/Transform.h"
#include <DirectXMath.h>

 /**
  * @struct Rotator
  * @brief �G���e�B�e�B�������I��Y�����S�ŉ�]������Behaviour�R���|�[�l���g
  * 
  * @details
  * ���̃R���|�[�l���g���G���e�B�e�B�ɒǉ�����ƁA���t���[�������I��
  * Y���i�㉺���j�𒆐S�ɉ�]���܂��B
  * * ### Behaviour�̎d�g��:
  * 1. World::Update()����OnUpdate()���\�b�h�������I�ɌĂ΂��B
  * 2. OnUpdate()���ŁA���g��Transform�R���|�[�l���g���擾���Arotation.y�Ɋp�x�����Z�B
  * 
  * @par �g�p��i�r���_�[�p�^�[���j:
  * @code
  * // ���b120�x�ŉ�]���� Entity ���쐬
  * Entity cube = world.Create()
  * .With<Transform>({0, 0, 10})
  * .With<Rotator>(120.0f)
  * .Build();
  * @endcode
  * 
  * @see Behaviour Behaviour�R���|�[�l���g�̊��N���X
  */
DEFINE_BEHAVIOUR(Rotator,
    /**
     * @var speedDegY
     * @brief Y������̉�]���x (�x/�b)
     * @note �p�x��**�x���@**�Ŏw�肵�܂��B
     */
    float speedDegY = 45.0f;

    /**
     * @brief ��]���x���w�肷��R���X�g���N�^
     * @param[in] s ��]���x�i�x/�b�j
     */
    explicit Rotator(float s) : speedDegY(s) {}

    /**
     * @brief �f�t�H���g�R���X�g���N�^
     * @details ��]���x��45.0�x/�b�ɐݒ肵�܂��B
     */
    Rotator() = default;
    , // <-- �f�[�^�����o�ƍX�V���W�b�N����؂�J���}

        /**
         * @brief ���t���[���Ă΂��X�V����
         * 
         * @param[in,out] w ���[���h�ւ̎Q�Ɓi�R���|�[�l���g�擾�Ɏg�p�j
         * @param[in] self ���̃R���|�[�l���g���t���Ă���G���e�B�e�B
         * @param[in] dt �f���^�^�C���i�O�t���[������̌o�ߕb���j
         * 
         * @details
         * ���̊֐������t���[�������I�ɌĂ΂�A�ȉ��̏������s���܂��F
         * 1. ���g��Transform�R���|�[�l���g���擾
         * 2. rotation.y�� speedDegY * dt �����Z���ĉ�]
         * 3. 360�x�𒴂����琳�K���i0�`360�x�͈̔͂Ɏ��߂�j
         * 
         * @note dt�i�f���^�^�C���j���|���邱�ƂŁA�t���[�����[�g�Ɉˑ����Ȃ�**���肵������**���������Ă��܂��B
         * @warning Transform�R���|�[�l���g���Ȃ��ꍇ�A�������X�L�b�v���܂��B
         */
        Transform* t = w.TryGet<Transform>(self);
    if (!t) return;

    // Y����]���f���^�^�C�����l�����ĉ��Z
    t->rotation.y += speedDegY * dt;

    // 360�x�𒴂����琳�K�� (0�`360�x)
    if (t->rotation.y > 360.0f) {
        t->rotation.y -= 360.0f;
    }
) // DEFINE_BEHAVIOUR �}�N���̏I���

#endif // !___ROTATOR_H___