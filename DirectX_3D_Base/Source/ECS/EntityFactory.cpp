/*****************************************************************//**
 * @file	EntityFactory.cpp
 * @brief	����̃G���e�B�e�B�i�v���C���[�A�n�ʂȂǁj�̐������W�b�N���W�񂷂�w���p�[�N���X�̎����B
 * 
 * @details	
 * Component�̋�̓I�Ȓl�ݒ�������ɏW�񂵁A�V�[���R�[�h���V���v���ɂ���B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�G���e�B�e�B�����̐ÓI�������쐬�B�n�ʃG���e�B�e�B�̐������W�b�N���ړ��B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

 // ===== �C���N���[�h =====
#include "ECS/EntityFactory.h"
#include "ECS/ECS.h" // ���ׂẴR���|�[�l���g��Coordinator�ɃA�N�Z�X���邽��
#include "Main.h" // METER�Ȃǂ̒萔�ɃA�N�Z�X

using namespace ECS;
using namespace DirectX;

// �ÓI�����o�ϐ��̒�` (�K�v�ɉ����Ēǉ�)
// const static ECS::EntityID EntityFactory::s_playerID = 2; // �G���e�B�e�BID��Coordinator���Ǘ����邽�ߕs�v

/**
 * @brief �Q�[�����[���h�̐ÓI�Ȓn�ʃG���e�B�e�B�𐶐�����
 * @param coordinator - �G���e�B�e�B�̐����Ɠo�^���s��Coordinator
 * @param position - �ʒu
 * @param scale - �X�P�[��
 * @return EntityID - �������ꂽ�n�ʃG���e�B�e�BID
 */
EntityID EntityFactory::CreateGround(Coordinator* coordinator, const XMFLOAT3& position, const XMFLOAT3& scale)
{
	// GameScene::CreateDemoEntities()����n�ʂ̃��W�b�N���ړ�
	ECS::EntityID ground = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	position,
			/* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Scale	*/	scale
		),
		RenderComponent(
			/* MeshType	*/	MESH_BOX,
			/* Color	*/	XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
		),
		RigidBodyComponent(
			/* Velocity		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Acceleration	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* Mass			*/	0.0f, // �ÓI�I�u�W�F�N�g
			/* Friction		*/	0.8f,
			/* Restitution	*/	0.2f
		),
		CollisionComponent(
			/* Size			*/	XMFLOAT3(scale.x / 2.0f, scale.y / 2.0f, scale.z / 2.0f),
			/* Offset		*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
			/* ColliderType	*/	COLLIDER_STATIC
		)
	);
	return ground;
}

/**
 * @brief �v���C���[�G���e�B�e�B�𐶐�����
 * @param coordinator - �G���e�B�e�B�̐����Ɠo�^���s��Coordinator
 * @param position - �����ʒu
 * @return EntityID - �������ꂽ�v���C���[�G���e�B�e�BID
 */
ECS::EntityID EntityFactory::CreatePlayer(Coordinator * coordinator, const XMFLOAT3 & position)
{
	// GameScene::CreateDemoEntities()����v���C���[�̃��W�b�N���ړ�
	ECS::EntityID player = coordinator->CreateEntity(
		TransformComponent(
			/* Position	*/	position,
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
			/* ColliderType	*/	COLLIDER_DYNAMIC
		),
		PlayerControlComponent(
			/* MoveSpeed	*/	4.0f,
			/* JumpPower	*/	3.0f
		)
	);
	return player;
}

/**
 * @brief �J�����G���e�B�e�B�𐶐�����
 * @param coordinator - �G���e�B�e�B�̐����Ɠo�^���s��Coordinator
 * @param focusID - �Ǐ]�Ώۂ̃G���e�B�e�BID
 * @return EntityID - �������ꂽ�J�����G���e�B�e�BID
 */
ECS::EntityID EntityFactory::CreateCamera(Coordinator* coordinator, EntityID focusID)
{
	// GameScene::CreateDemoEntities()����J�����̃��W�b�N���ړ�
	ECS::EntityID mainCamera = coordinator->CreateEntity(
		CameraComponent(
			/* FocusID		*/	focusID,
			/* Offset		*/	XMFLOAT3(0.0f, METER(3.0f), METER(-5.0f)),
			/* FollowSpeed	*/	0.1f
		),
		// RenderSystem������T�����߂̃_�~�[�R���|�[�l���g (RenderComponent��TransformComponent�͕K�{)
		RenderComponent(), TransformComponent()
	);
	return mainCamera;
}


/**
 * @brief �S�Ẵf���p�G���e�B�e�B�𐶐����AECS�ɓo�^���� (GameScene::Init()����Ă΂��)
 * @param coordinator - �G���e�B�e�B�̐����Ɠo�^���s��Coordinator
 */
void EntityFactory::CreateAllDemoEntities(Coordinator* coordinator)
{
	// --- 1. 1�ڂ̒n�ʁi�ÓI�I�u�W�F�N�g�j ---
	CreateGround(coordinator,
		XMFLOAT3(0.0f, -0.5f, 0.0f),
		XMFLOAT3(10.0f, 0.2f, 10.0f));

	// --- 2. 2�ڂ̒n�ʁi���j (ModelComponent���g�p) ---
	// CreateGround�̃��W�b�N�Ɏ��Ă��邪�AModelComponent���K�v�Ȃ��ߒ��ڋL�q
	coordinator->CreateEntity(
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
			/* Path		*/	"Assets/Model/Rizu/dousakakuninn11.fbx",
			/* Scale	*/	0.1f,
			/* Flip		*/	Model::ZFlip
		)
	);

	// --- 3. �v���C���[�ƃJ�����̐��� ---
	ECS::EntityID playerID = CreatePlayer(coordinator, XMFLOAT3(1.0f, 1.5f, 0.0f));
	CreateCamera(coordinator, playerID);
}