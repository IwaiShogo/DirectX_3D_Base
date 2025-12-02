#include "Scene/StageinformationScene.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Core/StageInfoSceneSystem.h"
#include "ECS/Components/Core/StageInfoSceneComponent.h"
#include "ECS/Systems/UI/CursorSystem.h"
#include "ECS/Systems/Rendering/RenderSystem.h"
#include "ECS/Systems/UI/UIRenderSystem.h"

using namespace DirectX;
using namespace ECS;

ECS::Coordinator* StageinformationScene::s_coordinator = nullptr;

void StageinformationScene::Init()
{
    // 1. ECS初期化
    m_coordinator = std::make_shared<ECS::Coordinator>();
    s_coordinator = m_coordinator.get();
    ECS::ECSInitializer::InitECS(m_coordinator);

    // 2. System登録
    // StageInfoSceneSystem
    {
        auto system = m_coordinator->RegisterSystem<StageInfoSceneSystem>();
        ECS::Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<StageInfoSceneComponent>());
        m_coordinator->SetSystemSignature<StageInfoSceneSystem>(signature);
        system->Init(m_coordinator.get());
    }

    // CursorSystem
    {
        auto system = m_coordinator->RegisterSystem<CursorSystem>();
        Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<TransformComponent>());
        signature.set(m_coordinator->GetComponentTypeID<TagComponent>());
        m_coordinator->SetSystemSignature<CursorSystem>(signature);
        system->Init(m_coordinator.get());
    }

    // 3. 管理用エンティティの生成
    m_coordinator->CreateEntity(StageInfoSceneComponent{});

    // BGM
    ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST2", 0.3f);
}

void StageinformationScene::Uninit()
{
    ECS::ECSInitializer::UninitECS();
    m_coordinator.reset();
    s_coordinator = nullptr;
}

void StageinformationScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);
}

void StageinformationScene::Draw()
{
    if (auto renderSystem = ECS::ECSInitializer::GetSystem<RenderSystem>())
    {
        renderSystem->DrawSetup();
        renderSystem->DrawEntities();
    }

    if (auto uiSystem = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        ::SetDepthTest(false);
        ::SetBlendMode(BLEND_ALPHA);
        uiSystem->Render();
        ::SetDepthTest(true);
    }
}