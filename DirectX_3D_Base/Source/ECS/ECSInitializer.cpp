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
	// --- Component�̓o�^�iGameScene::Init()����ړ��j ---
	coordinator->RegisterComponentType<TransformComponent>();
	coordinator->RegisterComponentType<RenderComponent>();
	coordinator->RegisterComponentType<RigidBodyComponent>();
	coordinator->RegisterComponentType<CollisionComponent>();
	coordinator->RegisterComponentType<PlayerControlComponent>();
	coordinator->RegisterComponentType<CameraComponent>();
	coordinator->RegisterComponentType<ModelComponent>();
	// --- �V����Component��ǉ�����ۂ͂����֒ǋL ---

	std::cout << "ECSInitializer: All Components registered." << std::endl;
}

void ECSInitializer::RegisterSystemsAndSetSignatures(Coordinator* coordinator)
{
    Coordinator* coordPtr = coordinator;

    // --- 1. RenderSystem ---
    {
        auto system = coordinator->RegisterSystem<RenderSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(coordinator->GetComponentTypeID<RenderComponent>());
        // Coordinator���v������^�i�e���v���[�g�j�ŃV�O�l�`����ݒ�
        coordinator->SetSystemSignature<RenderSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(RenderSystem))] = system;
    }

    // --- 2. PhysicsSystem ---
    {
        auto system = coordinator->RegisterSystem<PhysicsSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(coordinator->GetComponentTypeID<RigidBodyComponent>());
        signature.set(coordinator->GetComponentTypeID<CollisionComponent>()); // GameScene���畜��
        // Coordinator���v������^�i�e���v���[�g�j�ŃV�O�l�`����ݒ�
        coordinator->SetSystemSignature<PhysicsSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(PhysicsSystem))] = system;
    }

    // --- 3. PlayerControlSystem ---
    {
        auto system = coordinator->RegisterSystem<PlayerControlSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<RigidBodyComponent>());
        signature.set(coordinator->GetComponentTypeID<PlayerControlComponent>());
        coordinator->SetSystemSignature<PlayerControlSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(PlayerControlSystem))] = system;
    }

    // --- 4. CollisionSystem ---
    {
        auto system = coordinator->RegisterSystem<CollisionSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(coordinator->GetComponentTypeID<RigidBodyComponent>());
        signature.set(coordinator->GetComponentTypeID<CollisionComponent>());
        coordinator->SetSystemSignature<CollisionSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(CollisionSystem))] = system;
    }

    // --- 5. CameraControlSystem ---
    {
        auto system = coordinator->RegisterSystem<CameraControlSystem>();

        ECS::Signature signature;
        signature.set(coordinator->GetComponentTypeID<CameraComponent>());
        coordinator->SetSystemSignature<CameraControlSystem>(signature);

        system->Init(coordPtr);
        ECSInitializer::s_systems[std::type_index(typeid(CameraControlSystem))] = system;
    }

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