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
	// GameController Entityを検索
	/*ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
	if (controllerID == ECS::INVALID_ENTITY_ID) return;

	auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);*/
 	auto uiInput = ECS::ECSInitializer::GetSystem<UIInputSystem>();
	/*if (uiInput)
	{
		uiInput->Update(deltaTime);
	}*/
	
	for (auto entity : m_entities)
	{
		// コンポーネント取得
		const auto& interactable = m_coordinator->GetComponent<UIInteractableComponent>(entity);
		const auto& tagComp = m_coordinator->GetComponent<TagComponent>(entity);

		bool isCurrentHover = interactable.isHovered;

		bool isLastHover = m_lastHoverStates[entity];

		if (isCurrentHover && !isLastHover)
		{
			ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");

		}

		m_lastHoverStates[entity] = isCurrentHover;


		// クリックされたらタグを確認して遷移
		if (interactable.isClicked)
		{
			if (tagComp.tag == "ResultTitle")
			{
				ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");
				cout << "Title Clicked -> TitleScene" << endl;
				m_isTransitioning = true;
				m_nextSceneTag = "Title";
				m_transitionTimer = 0.0f;
				//SceneManager::ChangeScene<TitleScene>();
				break;
			}
			else if (tagComp.tag == "ResultRetry")
			{
				ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");

				cout << "Title Clicked -> GameScene" << endl;
				//SceneManager::ChangeScene<GameScene>();

				m_isTransitioning = true;
				m_nextSceneTag = "Game";
				m_transitionTimer = 0.0f;
				break;
			}
			/*else if (tagComp.tag == "ResultSelect")
			{
			ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST");
				cout << "Title Clicked -> SelectScene" << endl;
				m_isTransitioning = true;
				m_nextSceneTag = "Game";
				m_transitionTimer = 0.0f;
				break;
			}*/
		}
	}
	

}
