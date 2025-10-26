/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	�Q�[���̃��C�����W�b�N���܂ރV�[���N���X�̎����B
 * 
 * @details	
 * ECS�̏������Ǝ��s�A�f��Entity�̍쐬���W�b�N������B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS�̃��C�t�T�C�N���ƃf�����W�b�N���Ǘ����� `GameScene` �N���X�̎����B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "Scene/GameScene.h"

// Component
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/RigidBodyComponent.h"

// System
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/PhysicsSystem.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManager�����RenderSystem�擾�Ɏg�p

// ===== �ÓI�����o�[�ϐ��̒�` =====
// ���̃V�X�e������ECS�ɃA�N�Z�X���邽�߂̐ÓI�|�C���^
ECS::Coordinator* GameScene::s_coordinator = nullptr;

// ===== �f���p�̕ϐ� (Main.cpp����ڊ�) =====
namespace SceneDemo
{
	static float RotationRad = 0.0f;
	static float Time = 0.0f;
	const float ROTATION_INCREMENT = 0.01f; // 1�t���[��������0.01���W�A����]
	const float TIME_INCREMENT = 0.05f; // ���t���[���̎��Ԃ̐i�݋
}

using namespace DirectX;

/**
 * @brief �f���p��Entity�i�n�ʂƉ�]���锠�j���쐬���AECS�ɓo�^����
 * @param coordinator - Coordinator�C���X�^���X�ւ̃|�C���^
 */
static void CreateDemoEntities(ECS::Coordinator* coordinator)
{
	// --- 1. 1�ڂ̒n�� (Transform + Render) ---
	ECS::EntityID ground1 = coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(0.0f, -0.1f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(10.0f, 0.2f, 10.0f)
		),
		RenderComponent(
			MESH_BOX,
			XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
		)
	);

	// --- 2. 2�ڂ̒n�ʁi���j (Transform + Render) ---
	ECS::EntityID ground2 = coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(0.0f, 1.0f, 0.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			XMFLOAT3(0.2f, 3.0f, 0.2f)
		),
		RenderComponent(
			MESH_BOX,
			XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f)
		),
		RigidBodyComponent(

		)
	);

	// --- 3. ��]���锠 (Transform + Render) ---
	ECS::EntityID rotatingBox = coordinator->CreateEntity(
		TransformComponent(
			XMFLOAT3(1.0f, 1.5f, 0.0f),
			XMFLOAT3(0.0f, SceneDemo::RotationRad, 0.0f),
			XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			MESH_BOX,
			XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
		)
	);
}


// ===== GameScene �����o�[�֐��̎��� =====

GameScene::GameScene()
{
	// �R���X�g���N�^�œ��ɏ����͍s�킸�AInit()��ECS������������
}

GameScene::~GameScene()
{
	// �f�X�g���N�^��Uninit()���Ă΂�Am_coordinator����������
}

void GameScene::Init()
{
	// --- 1. ECS Coordinator�̏����� ---
	m_coordinator = std::make_unique<ECS::Coordinator>();
	m_coordinator->Init();

	// �ÓI�|�C���^�Ɍ��݂�Coordinator��ݒ�
	s_coordinator = m_coordinator.get();

	// --- 2. Component�̓o�^ ---
	m_coordinator->RegisterComponentType<TransformComponent>();
	m_coordinator->RegisterComponentType<RenderComponent>();
	m_coordinator->RegisterComponentType<RigidBodyComponent>();

	// --- 3. System�̓o�^��Signature�̐ݒ� ---
	// RenderSystem�̓o�^
	m_renderSystem = m_coordinator->RegisterSystem<RenderSystem>();

	ECS::Signature renderSignature;
	renderSignature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
	renderSignature.set(m_coordinator->GetComponentTypeID<RenderComponent>());
	m_coordinator->SetSystemSignature<RenderSystem>(renderSignature);
	m_renderSystem->Init();

	// PhysicsSystem�̓o�^
	m_physicsSystem = m_coordinator->RegisterSystem<PhysicsSystem>();

	ECS::Signature physicsSignature;
	physicsSignature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
	physicsSignature.set(m_coordinator->GetComponentTypeID<RigidBodyComponent>());
	m_coordinator->SetSystemSignature<PhysicsSystem>(physicsSignature);
	m_physicsSystem->Init();

	// --- 4. �f���pEntity�̍쐬 ---
	CreateDemoEntities(m_coordinator.get());

	std::cout << "GameScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;
}

void GameScene::Uninit()
{
	// Coordinator�̔j���iunique_ptr�������I��delete�����s�j
	m_coordinator.reset();
	m_renderSystem.reset();

	// �ÓI�|�C���^���N���A
	s_coordinator = nullptr;

	std::cout << "GameScene::Uninit() - ECS Destroyed." << std::endl;
}

void GameScene::Update(float deltaTime)
{
	// --- 1. �f���p�ϐ��̍X�V���W�b�N (Main.cpp����ڊ�) ---
	SceneDemo::RotationRad += SceneDemo::ROTATION_INCREMENT;
	if (SceneDemo::RotationRad > XM_2PI)
	{
		SceneDemo::RotationRad -= XM_2PI;
	}

	SceneDemo::Time += SceneDemo::TIME_INCREMENT;
	const float CENTER_Y = 1.5f;
	const float AMPLITUDE = 0.5f;
	const float FREQUENCY = 2.0f;
	float newY = CENTER_Y + AMPLITUDE * sin(SceneDemo::Time * FREQUENCY);

	// --- 2. ECS Entity��Component�X�V ---
	// 3�ڂ�Entity�iID: 2�j�͉�]���锠�Ɖ���
	const ECS::EntityID rotatingBoxID = 2;

	if (m_coordinator && m_coordinator->m_entityManager->GetSignature(rotatingBoxID).test(m_coordinator->m_componentManager->GetComponentTypeID<TransformComponent>()))
	{
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(rotatingBoxID);
		transform.Rotation.y = SceneDemo::RotationRad; // Y����]���X�V
		transform.Position.y = newY; // Y���ʒu���X�V
	}

	// TODO: PhysicsSystem�Ȃǂ��������ꂽ��A������UpdateSystem�����s����
	if (m_physicsSystem)
	{
		m_physicsSystem->Update();
	}
}

void GameScene::Draw()
{
	// RenderSystem�͏�ɑ��݂���Ɖ���
	if (m_renderSystem)
	{
		// 1. �J�����ݒ��f�o�b�O�O���b�h�`��
		m_renderSystem->DrawSetup();

		// 2. ECS Entity�̕`��
		m_renderSystem->DrawEntities();
	}
}