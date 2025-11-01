/*****************************************************************//**
 * @file	ComponentRegistry.cpp
 * @brief	Meyers' Singleton�p�^�[�����g�p���Ĉ��S�ɃO���[�o���ȓo�^���X�g���Ǘ����܂��B
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/01	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 *
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 *
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COMPONENT_REGISTRY_H___
#define ___COMPONENT_REGISTRY_H___

 // ===== �C���N���[�h =====
#include "Coordinator.h"
#include <vector>
#include <functional>
#include <iostream>

namespace ECS
{
    // Component�̓o�^�����̌^��`
    using ComponentRegisterFn = std::function<void(Coordinator*)>;

    /**
     * @brief Component�o�^�֐����X�g�ւ̈��S�ȃA�N�Z�T (Meyers' Singleton�p�^�[��)
     * @details Component�̌^�o�^�֐����ÓI�ɒǉ�����郊�X�g��Ԃ��܂��B
     */
    inline std::vector<ComponentRegisterFn>& GetComponentRegisterers()
    {
        // �ÓI���[�J���ϐ��Ƃ��Ē�`���邱�ƂŁA�������Ɣj�󏇏��̈��S����ۏ�
        static std::vector<ComponentRegisterFn> s_registerers;
        return s_registerers;
    }

    /**
     * @brief Component�̎����o�^�w���p�[�}�N��
     * @details �eComponent�w�b�_�[�̐ÓI�������q�𗘗p���āA�o�^�֐������X�g�ɒǉ����܂��B
     */
#define REGISTER_COMPONENT_TYPE(ComponentType) \
    namespace { \
        /* ComponentTypeID�̈�Ӑ����m�ۂ��邽�߁A�ÓI�ϐ�����ComponentType��g�ݍ��� */ \
        static struct ComponentRegistrationHelper_##ComponentType { \
            ComponentRegistrationHelper_##ComponentType() { \
                /* GetComponentRegisterers()���Ăяo���A���X�g�ɓo�^�֐���ǉ� */ \
                ECS::GetComponentRegisterers().push_back([](ECS::Coordinator* coord) { \
                    coord->RegisterComponentType<ComponentType>(); \
                    /* std::cout << "Component Registered: " #ComponentType << std::endl; */ \
                }); \
            } \
        } s_componentReg_##ComponentType; \
    }
}

#endif // !___COMPONENT_REGISTRY_H___