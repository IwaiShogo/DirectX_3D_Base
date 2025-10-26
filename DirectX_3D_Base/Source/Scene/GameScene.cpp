/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "Scene/GameScene.h"
#include "ECS/Coordinator.h"
// Component
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"
#include "ECS/Component/Input.h"
#include "ECS/Component/Collider.h"
#include "ECS/Component/Camera.h"
// System
#include "ECS/System/MovementSystem.h"
#include "ECS/System/RenderSystem.h"
#include "ECS/System/InputSystem.h"
#include "ECS/System/CollisionSystem.h"
#include "ECS/System/CameraSystem.h"

/* ���\�[�X�}�l�[�W���[ */
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"

// --------------------------------------------------
// �O���[�o����Coordinator�̒�` (RenderSystem.h��extern�錾��������)
// --------------------------------------------------
// �� ���ۂ̃v���W�F�N�g�ł́A�����S��DI (�ˑ�������)��V���O���g���p�^�[���𐄏����܂����A
//    �����ł�ECS�̃R�A�\�����V���v���ɓ��삳���邽�߁AGameScene���ێ�����C���X�^���X�ւ�
//    �O���[�o���A�N�Z�X��������`�Œ�`���܂��B
Coordinator* g_pCoordinator = nullptr;

// --------------------------------------------------
// �����֐�: ECS�̏����ݒ�
// --------------------------------------------------

/**
 * @brief ECS�R�A�AComponent�ASystem�̏������Ɠo�^���s��
 */
void GameScene::SetupECS()
{
    // 1. Coordinator�̏����� (�����̊e��Manager��������)
    coordinator_.Init();

    // 2. Component�̓o�^
    // �g�p����S�Ă�Component�^��o�^���AComponentType ID�����蓖�Ă�
    coordinator_.RegisterComponent<Transform>();
    coordinator_.RegisterComponent<MeshRenderer>();
    coordinator_.RegisterComponent<Input>();
    coordinator_.RegisterComponent<Collider>();
    coordinator_.RegisterComponent<CameraSetting>();
    // ... ����ǉ����� Component (��: Input, Health, Collider) �������ɒǉ�

    // 3. System�̓o�^��Signature�̐ݒ�

    // MovementSystem�̓o�^�ƃV�O�l�`���ݒ�
    movementSystem_ = coordinator_.RegisterSystem<MovementSystem>();
    Signature movementSignature;
    movementSignature.set(coordinator_.GetComponentType<Transform>(), true);
    // movementSignature.set(coordinator_.GetComponentType<Velocity>(), true); // Velocity Component�������
    coordinator_.SetSystemSignature<MovementSystem>(movementSignature);
    movementSystem_->Initialize();

    // RenderSystem�̓o�^�ƃV�O�l�`���ݒ�
    renderSystem_ = coordinator_.RegisterSystem<RenderSystem>();
    Signature renderSignature;
    renderSignature.set(coordinator_.GetComponentType<Transform>(), true);
    renderSignature.set(coordinator_.GetComponentType<MeshRenderer>(), true);
    coordinator_.SetSystemSignature<RenderSystem>(renderSignature);
    renderSystem_->Initialize();

    // InputSystem�̓o�^�ƃV�O�l�`���ݒ�
    inputSystem_ = coordinator_.RegisterSystem<InputSystem>();
    Signature inputSignature;
    inputSignature.set(coordinator_.GetComponentType<Input>(), true); // Input Component�̂�
    coordinator_.SetSystemSignature<InputSystem>(inputSignature);
    inputSystem_->Initialize();

    // CollisionSystem�̓o�^�ƃV�O�l�`���ݒ�
    collisionSystem_ = coordinator_.RegisterSystem<CollisionSystem>();
    Signature collisionSignature;
    collisionSignature.set(coordinator_.GetComponentType<Transform>(), true);
    collisionSignature.set(coordinator_.GetComponentType<Collider>(), true); // ��Collider Component��v��
    coordinator_.SetSystemSignature<CollisionSystem>(collisionSignature);
    //collisionSystem_->Initialize();

    // CameraSystem�̓o�^�ƃV�O�l�`���ݒ�
    cameraSystem_ = coordinator_.RegisterSystem<CameraSystem>(); // ���ǉ�
    Signature cameraSignature;
    cameraSignature.set(coordinator_.GetComponentType<Transform>(), true);
    cameraSignature.set(coordinator_.GetComponentType<CameraSetting>(), true);
    coordinator_.SetSystemSignature<CameraSystem>(cameraSignature);
    cameraSystem_->Initialize();

    // 4. �O���[�o����Coordinator�ւ̎Q�Ƃ�ݒ�
    // GameScene�����C���X�^���X���O���[�o���Q�Ƃɐݒ�
    // �����RenderSystem::Draw()����g_Coordinator�A�N�Z�X��L���ɂ��邽�߂ɕK�{
    g_pCoordinator = &coordinator_;
}

/**
 * @brief ECS Entity�̏����z�u�i�e�X�g�pEntity�̐����Ȃǁj���s��
 */
void GameScene::SetupEntities()
{
    // --------------------------------------------------
    // ���C���J���� Entity�̐���
    // --------------------------------------------------
    mainCameraEntity_ = coordinator_.CreateEntity();

    // Transform
    Transform camT;
    camT.position = { 0.0f, 10.0f, -20.0f };
    camT.rotation = { 20.0f, 0.0f, 0.0f };
    coordinator_.AddComponent(mainCameraEntity_, camT);

    // CameraSetting
    CameraSetting camS;
    camS.aspectRatio = 1280.0f / 720.0f;
    coordinator_.AddComponent(mainCameraEntity_, camS);





    // �e�X�g�p��Entity�𐶐�
    Entity cube = coordinator_.CreateEntity();

    // Transform Component��ǉ�
    Transform t;
    t.position = { 10.0f, 5.0f, 0.0f };
    t.rotation = { 0.0f, 45.0f, 0.0f };
    coordinator_.AddComponent(cube, t);

    // MeshRenderer Component��ǉ�
    MeshRenderer mr;
    mr.meshId = 1;      // ���ۂ̃��b�V��ID
    mr.textureId = 1;   // ���ۂ̃e�N�X�`��ID
    mr.isLoaded = true;
    mr.color = { 0.8f, 0.2f, 0.2f, 1.0f }; // �Ԃ��L���[�u
    coordinator_.AddComponent(cube, mr);

    // RenderSystem��MovementSystem�̗�����Signature�𖞂������߁A������System�ɏ����ΏۂƂ��Ēǉ������B
}

// --------------------------------------------------
// Scene�C���^�[�t�F�[�X�̎���
// --------------------------------------------------

void GameScene::Initialize()
{
    // ECS�̃Z�b�g�A�b�v
    SetupECS();

    // Entity�̏����z�u
    SetupEntities();

    // ... �J������DirectX���\�[�X�̏������Ȃ�
}

void GameScene::Update(float deltaTime)
{
    // --------------------------------------------------
    // ECS�̍X�V���� (�f�[�^�쓮�^�̃��W�b�N���s)
    // --------------------------------------------------

    // 1. InputSystem�̍X�V�i�O���̏�Ԃ�ECS�f�[�^�ɏ������ށj
    inputSystem_->Update(deltaTime);

    // 2. �J����System�̍X�V
    cameraSystem_->Update(deltaTime);

    // 2. ���W�b�NSystem�̍X�V
    // MovementSystem��Transform Component���X�V���A�ʒu��ω�������
    movementSystem_->Update(deltaTime);

    // 3. �Փ�System�̍X�V (�Փ˃t���O�̍X�V)
    // �ړ����Transform�f�[�^���g�p���ďՓ˔�������s
    collisionSystem_->Update(deltaTime);

    // ... ����System (��: InputSystem, PhysicsSystem, AnimationSystem) �������Ŏ��s

    // --------------------------------------------------
    // ����: RenderSystem��Update�ł͕`�悹���ADraw�Ŏ��s���邱�Ƃ���ʓI�ł��B
    // --------------------------------------------------
}

void GameScene::Draw()
{
    // DirectX�̕`��J�n�����i��: ClearRenderTargetView, BeginScene�Ȃǁj

    // 2. �`��System�̎��s
    // RenderSystem��Transform��MeshRenderer��ǂݎ��ADirectX��DrawCall�𔭍s����
    // �� Camera��MeshManager�͊O���V�X�e���Ƃ���Draw���\�b�h�ɓn���K�v������܂��B
    //   �����ł�Camera��MeshManager���O���Œ�`����Ă��邱�Ƃ�O��ɋ[���I�ɌĂяo���܂��B
    //   (��: Camera* g_Camera; MeshManager* g_MeshManager; )

    // Camera& cam = GetCurrentCamera(); 
    // MeshManager& meshMgr = GetMeshManager();

    // renderSystem_->Draw(cam, meshMgr); 

    // DirectX�̕`��I�������i��: Present�Ȃǁj

    // 1. �J����System����ŐV��ViewProjection�s����擾
    // CameraSystem�͊���Update�Ōv�Z�ς�
    DirectX::XMMATRIX viewProjMatrix = cameraSystem_->GetViewProjectionMatrix(); // ���C��

    // 2. RenderSystem�̎��s
    // RenderSystem.h��Draw���\�b�h�̃V�O�l�`���ɍ��킹�āA�_�~�[��MeshManager���g�p
    class DummyMeshManager {};
    DummyMeshManager meshMgr;

    renderSystem_->Draw(viewProjMatrix, meshMgr); // ���C��: ViewProj�s���n��

}

void GameScene::Finalize()
{
    // ... �V�[���ŗL�̃��\�[�X����Ȃ�
}