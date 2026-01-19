#include "include/scene/OpeningScene.h"
#include "ECS/Systems/Core/OpeningControlSystem.h"

#include <ECS/ECSInitializer.h>
#include <ECS/Systems/UI/UIRenderSystem.h>

using namespace ECS;

void OpeningScene::Init()
{
    m_coordinator = std::make_shared<ECS::Coordinator>();
    ECS::ECSInitializer::InitECS(m_coordinator);

    if (auto system = ECS::ECSInitializer::GetSystem<OpeningControlSystem>())
    {
        system->Init(m_coordinator.get());

        system->StartOpening();
    }
}

void OpeningScene::Uninit()
{
    ECS::ECSInitializer::UninitECS();
    m_coordinator.reset();
}

void OpeningScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);
}

void OpeningScene::Draw()
{
    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(true);
        system->Render(false);
    }
}