#include "ECS/Systems/Core/ResultSceneSystem.h"
#include "ECS/Systems/Core/GameFlowSystem.h"
#include "ECS//Systems/Core/ResultSceneSystem.h"
#include "Scene/SceneManager.h"
#include "ECS/ECSInitializer.h"
#include "ECS/Systems/UI/UIInputSystem.h"
#include "ECS//EntityFactory.h"

using namespace std;


void ResultSceneSystem::Update(float deltaTime)
{
    if (m_isTransitioning)
    {
        m_transitionTimer += deltaTime;
        if (m_transitionTimer > 0.5f)
        {
            if (m_nextSceneTag == "Title")
            {
                cout << "Timer Complete -> TitleScene" << endl;
                SceneManager::ChangeScene<TitleScene>();
            }
            else if (m_nextSceneTag == "Game")
            {
                cout << "Timer Complete -> GameScene" << endl;
                SceneManager::ChangeScene<GameScene>();
            }
            return;
        }
        return;
    }

    // ボタンの状態確認
    for (auto const& entity : m_entities)
    {
        const auto& interactable = m_coordinator->GetComponent<UIButtonComponent>(entity);
        const auto& tagComp = m_coordinator->GetComponent<TagComponent>(entity);

        // --- 修正: isHovered を state == Hover に変更 ---
        bool isCurrentHover = (interactable.state == ButtonState::Hover);
        bool isLastHover = m_lastHoverStates[entity];

        if (isCurrentHover && !isLastHover)
        {
            ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");
        }
        m_lastHoverStates[entity] = isCurrentHover;

        // --- 修正: isClicked を state == Pressed に変更 ---
        if (interactable.state == ButtonState::Pressed)
        {
            if (tagComp.tag == "ResultTitle")
            {
                ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");
                cout << "Title Clicked -> TitleScene" << endl;
                m_isTransitioning = true;
                m_nextSceneTag = "Title";
                m_transitionTimer = 0.0f;
                break;
            }
            else if (tagComp.tag == "ResultRetry")
            {
                ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");
                cout << "Retry Clicked -> GameScene" << endl;
                m_isTransitioning = true;
                m_nextSceneTag = "Game";
                m_transitionTimer = 0.0f;
                break;
            }
        }
    }
}
