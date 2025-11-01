/*****************************************************************//**
 * @file	ECSInitializer.cpp
 * @brief	ECS�V�X�e���S�̂̏��������W�񂵁A�V�[����Init()����Ӗ��𕪗����邽�߂̃w���p�[�N���X�̎����B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "ECS/ECSInitializer.h"
#include "ECS/AllComponents.h"
#include "ECS/AllSystems.h"
#include <iostream>

using namespace ECS;

// �ÓI�����o�[�ϐ� s_systems �̎��̂��`���A���������m�ۂ���
std::unordered_map<std::type_index, std::shared_ptr<ECS::System>> ECS::ECSInitializer::s_systems;

/**
 * [void - RegisterComponents]
 * @brief	�S�ẴR���|�[�l���g��Coordinator�ɓo�^����B
 * 
 * @param	[in] coordinator 
 */
void ECSInitializer::RegisterComponents(Coordinator* coordinator)
{
    // �R���|�[�l���g�̓o�^�i�����œo�^�����j
    for (const auto& registerFn : GetComponentRegisterers())
    {
        registerFn(coordinator);
    }

	std::cout << "ECSInitializer: All Components registered." << std::endl;
}

void ECSInitializer::RegisterSystemsAndSetSignatures(Coordinator* coordinator)
{
    Coordinator* coordPtr = coordinator;

    // ============================================================
    // �V�X�e���̓o�^�ƃV�O�l�`���̐ݒ�i�������牺�ɒǉ��j
    // ============================================================

    // --- RenderSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  RenderSystem,
        /* Components   */  RenderComponent, TransformComponent
    );

    // --- PhysicsSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PhysicsSystem,
        /* Components   */  RigidBodyComponent, TransformComponent, CollisionComponent
    );

    // --- PlayerControlSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PlayerControlSystem,
        /* Components   */  PlayerControlComponent, RigidBodyComponent
    );
    
    // --- CollisionSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollisionSystem,
        /* Components   */  CollisionComponent, TransformComponent, RigidBodyComponent
    );

    // --- CameraControlSystem ---
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CameraControlSystem,
        /* Components   */  CameraComponent
    );

    std::cout << "ECSInitializer: All Systems registered and initialized." << std::endl;
}

/**
 * [void - InitECS]
 * @brief	Coordinator��System���֘A�t����G���g���|�C���g�B
 * 
 * @param	[in] coordinator 
 */
void ECSInitializer::InitECS(std::shared_ptr<Coordinator>& coordinator)
{
	// Coordinator�̐��|�C���^���擾
	Coordinator* rawCoordinator = coordinator.get();

	// 1. Coordinator���̂̏����� (ECS�R�A�����̃f�[�^�\���̏�����)
	rawCoordinator->Init();

	// GameScene::s_coordinator �ւ̐ݒ�́A���̃X�e�b�v��GameScene::Init()�Ɉړ����܂��B

	// 2. �R���|�[�l���g�̓o�^
	RegisterComponents(rawCoordinator);

	// 3. �V�X�e���̓o�^�ƃV�O�l�`���̐ݒ� (�ÓI�}�b�v�Ɋi�[�����)
	RegisterSystemsAndSetSignatures(rawCoordinator);
}

/**
 * @brief ECS�Ɋ֘A����S�Ă̐ÓI���\�[�X���N���[���A�b�v����B
 */
void ECSInitializer::UninitECS()
{
	s_systems.clear(); // �S�ẴV�X�e��SharedPtr�����
}