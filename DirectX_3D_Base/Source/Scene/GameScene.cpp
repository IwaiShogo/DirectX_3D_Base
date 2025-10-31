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

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManager�����RenderSystem�擾�Ɏg�p
 
// ===== �ÓI�����o�[�ϐ��̒�` =====u
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
	// --- 1. 1�ڂ̒n�ʁi�ÓI�I�u�W�F�N�g�j ---
	ECS::EntityID ground1 = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(0.0f, -0.5f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(10.0f, 0.2f, 10.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	0.0f,
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(5.0f, 0.1f, 5.0f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_STATIC
		)
	);

	// --- 2. 2�ڂ̒n�ʁi���j (Transform + Render) ---
	ECS::EntityID ground2 = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(2.0f, 0.0f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_MODEL,
			/* Color	*/	XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	0.0f,
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_STATIC
		),
		ModelComponent(
			/* Path		*/	"Assets/Model/Rizu/dousakakuninn11 1.fbx",
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::ZFlip
		)
	);

	// --- 3. ��]���锠 (Transform + Render) ---
	ECS::EntityID player = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	XMFLOAT3(1.0f, 1.5f, 0.0f),
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	XMFLOAT3(1.0f, 1.0f, 1.0f)
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	1.0f,
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(0.5f, 0.5f, 0.5f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_DYNAMIC // ���I
		),
		PlayerControlComponent(
			/* MoveSpeed	*/	4.0f,
			/* JumpPower	*/	3.0f
		)
	);

	const static ECS::EntityID s_playerID = player;

	// �J����Entity�̐����i�v���C���[ EntityID ��Ǐ]�Ώۂɐݒ�j
	ECS::EntityID mainCamera = coordinator->CreateEntity(
		CameraComponent(
			/* FocusID		*/	s_playerID,
			/* Offset		*/	XMFLOAT3(0.0f, METER(3.0f), METER(-5.0f)),
			/* FollowSpeed	*/	0.1f
		),
		RenderComponent(),TransformComponent()	// RenderSystem������T������
	);
}


// ===== GameScene �����o�[�֐��̎��� =====

void GameScene::Init()
{
	// --- 1. ECS Coordinator�̏����� ---
	m_coordinator = std::make_unique<ECS::Coordinator>();

	// �ÓI�|�C���^�Ɍ��݂�Coordinator��ݒ�
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 4. �f���pEntity�̍쐬 ---
	CreateDemoEntities(m_coordinator.get());

	std::cout << "GameScene::Init() - ECS Initialized and Demo Entities Created." << std::endl;
}

void GameScene::Uninit()
{
	// 1. ECS System�̐ÓI���\�[�X�����
	ECS::ECSInitializer::UninitECS();

	// Coordinator�̔j���iunique_ptr�������I��delete�����s�j
	m_coordinator.reset();

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

	// --- 2. ECS System�̍X�V
	// 1. ����
	// if (m_playerControlSystem) // �폜
	if (auto system = ECS::ECSInitializer::GetSystem<PlayerControlSystem>())
	{
		system->Update();
	}

	// 2. �����v�Z�i�ʒu�̍X�V�j
	// if (m_physicsSystem) // �폜
	if (auto system = ECS::ECSInitializer::GetSystem<PhysicsSystem>())
	{
		system->Update();
	}

	// 3. �Փˌ��o�Ɖ����i�ʒu�̏C���j
	// if (m_collisionSystem) // �폜
	if (auto system = ECS::ECSInitializer::GetSystem<CollisionSystem>())
	{
		system->Update();
	}

	// 4. �J��������i�r���[�E�v���W�F�N�V�����s��̍X�V�j
	// if (m_cameraControlSystem) // �폜
	if (auto system = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
	{
		system->Update();
	}

	// --- 3. ECS Entity��Component�X�V ---
	// 3�ڂ�Entity�iID: 2�j�͉�]���锠�Ɖ���
	const ECS::EntityID rotatingBoxID = 2;

	if (m_coordinator && m_coordinator->m_entityManager->GetSignature(rotatingBoxID).test(m_coordinator->m_componentManager->GetComponentTypeID<TransformComponent>()))
	{
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(rotatingBoxID);
		//transform.Rotation.y = SceneDemo::RotationRad; // Y����]���X�V
		//transform.Position.y = newY; // Y���ʒu���X�V
	}
}

void GameScene::Draw()
{
	// RenderSystem�͏�ɑ��݂���Ɖ���
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		// 1. �J�����ݒ��f�o�b�O�O���b�h�`��
		system->DrawSetup();

		// 2. ECS Entity�̕`��
		system->DrawEntities();
	}
}