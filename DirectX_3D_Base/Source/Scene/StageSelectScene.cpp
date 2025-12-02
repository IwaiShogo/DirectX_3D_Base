#include "Scene/StageSelectScene.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Core/StageSelectSceneSystem.h"
#include "ECS/Components/Core/StageSelectSceneComponent.h"
#include "ECS/Systems/UI/CursorSystem.h"

using namespace ECS;

ECS::Coordinator* StageSelectScene::s_coordinator = nullptr;

void StageSelectScene::Init()
{
    // 1. ECS初期化
    m_coordinator = std::make_shared<Coordinator>();
    s_coordinator = m_coordinator.get();
    ECSInitializer::InitECS(m_coordinator);

    // 2. System登録
    // StageSelectSceneSystem
    {
        auto system = m_coordinator->RegisterSystem<StageSelectSceneSystem>();
        ECS::Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<StageSelectSceneComponent>());
        m_coordinator->SetSystemSignature<StageSelectSceneSystem>(signature);
        system->Init(m_coordinator.get());
    }

    // CursorSystem
    {
        auto system = m_coordinator->RegisterSystem<CursorSystem>();
        ECS::Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(m_coordinator->GetComponentTypeID<TagComponent>());
        m_coordinator->SetSystemSignature<CursorSystem>(signature);
        system->Init(m_coordinator.get());
    }

    // 3. 管理用エンティティの生成
    m_coordinator->CreateEntity(StageSelectSceneComponent{});

    // BGM
    EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST", 0.3f);
}

void StageSelectScene::Uninit()
{
    ECSInitializer::UninitECS();
    m_coordinator.reset();
    s_coordinator = nullptr;
}

void StageSelectScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);
}

void StageSelectScene::Draw()
{
    if (auto system = ECSInitializer::GetSystem<RenderSystem>()) {
        system->DrawSetup();
        system->DrawEntities();
    }
    if (auto system = ECSInitializer::GetSystem<UIRenderSystem>()) {
        system->Render();
    }
}