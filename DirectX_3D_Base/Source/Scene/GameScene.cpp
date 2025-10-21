#include "Scene/GameScene.h"
#include <iostream>

void GameScene::Initialize(SceneManager& manager)
{
    // �� ������Main.cpp�Ȃǂ���A�ˑ��I�u�W�F�N�g�iDirectX, Camera, MeshBuffer�Ȃǁj���擾/�������郍�W�b�N�������ɈڐA���܂��B
    // ��: gfx_ = &DirectX::GetInstance();

    // 1. World�̍쐬�Ə�����
    world_ = std::make_unique<World>();

    // 2. RenderSystem�̍쐬�Ə�����
    // RenderSystem�͈ˑ��I�u�W�F�N�g�iDirectX, Camera�Ȃǁj���󂯎��
    renderer_ = std::make_unique<RenderSystem>(*gfx_, *cam_, *meshMgr_, *shaderList_);

    // 3. �e�X�g�pEntity�̐����i�X�e�b�v2-3�Œ�Ă������W�b�N�j

    // --------------------------------------------------
    // Test Entity: Rotating Cube (Rotator Behaviour�̃e�X�g)
    // --------------------------------------------------
    world_->Create()
        .With<Transform>(
            DirectX::XMFLOAT3{ 5, 5, 5 },
            DirectX::XMFLOAT3{ 0, 0, 0 },
            DirectX::XMFLOAT3{ 2, 2, 2 }
        )
        .With<MeshRenderer>(
            MeshBuffer::CUBE_MESH_ID,
            DirectX::XMFLOAT3{ 1, 0, 0 }, // ��
            Texture::INVALID_TEXTURE
        )
        .With<Rotator>(90.0f); // ���b90�x��]

    std::cout << "GameScene Initialized. Entities created." << std::endl;
}

void GameScene::Update(float dt)
{
    // World���̑SBehaviour�R���|�[�l���g���X�V
    world_->Update(dt);

    // �� ������InputSystem�Ȃǂ̓Ɨ�����System���Ăяo�����Ƃ��\�ł��B
    // ��: inputSystem_->Update(dt);

    // �V�[���J�ڂ̃��W�b�N�������ɋL�q
    // ��: if (isGameOver) { manager.ChangeScene("Result"); }
}

void GameScene::Draw()
{
    // World���̕`��\��Entity��RenderSystem�ŕ`��
    if (renderer_ && world_) {
        renderer_->Draw(*world_);
    }
}

void GameScene::Finalize()
{
    // World��RenderSystem��std::unique_ptr�ł��邽�߁A�����Ń������͎����������܂��B
    world_.reset();
    renderer_.reset();
}