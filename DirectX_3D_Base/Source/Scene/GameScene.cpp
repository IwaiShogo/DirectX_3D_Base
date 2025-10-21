#include "Scene/GameScene.h"
#include <iostream>

void GameScene::Initialize(SceneManager& manager)
{
    // ※ 既存のMain.cppなどから、依存オブジェクト（DirectX, Camera, MeshBufferなど）を取得/生成するロジックをここに移植します。
    // 例: gfx_ = &DirectX::GetInstance();

    // 1. Worldの作成と初期化
    world_ = std::make_unique<World>();

    // 2. RenderSystemの作成と初期化
    // RenderSystemは依存オブジェクト（DirectX, Cameraなど）を受け取る
    renderer_ = std::make_unique<RenderSystem>(*gfx_, *cam_, *meshMgr_, *shaderList_);

    // 3. テスト用Entityの生成（ステップ2-3で提案したロジック）

    // --------------------------------------------------
    // Test Entity: Rotating Cube (Rotator Behaviourのテスト)
    // --------------------------------------------------
    world_->Create()
        .With<Transform>(
            DirectX::XMFLOAT3{ 5, 5, 5 },
            DirectX::XMFLOAT3{ 0, 0, 0 },
            DirectX::XMFLOAT3{ 2, 2, 2 }
        )
        .With<MeshRenderer>(
            MeshBuffer::CUBE_MESH_ID,
            DirectX::XMFLOAT3{ 1, 0, 0 }, // 赤
            Texture::INVALID_TEXTURE
        )
        .With<Rotator>(90.0f); // 毎秒90度回転

    std::cout << "GameScene Initialized. Entities created." << std::endl;
}

void GameScene::Update(float dt)
{
    // World内の全Behaviourコンポーネントを更新
    world_->Update(dt);

    // ※ ここでInputSystemなどの独立したSystemを呼び出すことも可能です。
    // 例: inputSystem_->Update(dt);

    // シーン遷移のロジックをここに記述
    // 例: if (isGameOver) { manager.ChangeScene("Result"); }
}

void GameScene::Draw()
{
    // World内の描画可能なEntityをRenderSystemで描画
    if (renderer_ && world_) {
        renderer_->Draw(*world_);
    }
}

void GameScene::Finalize()
{
    // WorldとRenderSystemがstd::unique_ptrであるため、ここでメモリは自動解放されます。
    world_.reset();
    renderer_.reset();
}