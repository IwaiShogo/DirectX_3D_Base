#include "ECS/Systems/Core/GameSceneSystem.h"
#include "ECS/Components/Core/GameSceneComponent.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "ECS/Components/Gameplay/ItemTrackerComponent.h"
#include "ECS/Components/Core/SoundComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/UIImageComponent.h"
#include "ECS/EntityFactory.h"
#include <iostream>
#include <vector>

using namespace DirectX;
using namespace ECS;

void GameSceneSystem::Update(float deltaTime)
{
    // GameSceneComponentを持つエンティティを探す
    for (auto const& entity : m_entities)
    {
        // ECS::GameSceneComponent ではなく GameSceneComponent (グローバル)
        auto& sceneComp = m_coordinator->GetComponent<GameSceneComponent>(entity);

        // --- 依存データの取得 ---
        ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
        if (controllerID == ECS::INVALID_ENTITY_ID) return;

        const auto& gameState = m_coordinator->GetComponent<GameStateComponent>(controllerID);

        int currentItems = 0;
        // 修正: GetEntityManager() -> m_entityManager
        // 修正: ECS::ItemTrackerComponent -> ItemTrackerComponent
        if (m_coordinator->m_entityManager->GetSignature(controllerID).test(m_coordinator->GetComponentTypeID<ItemTrackerComponent>()))
        {
            currentItems = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID).collectedItems;
        }

        // --- BGM管理ロジック ---
        // エンティティ生成（初回のみ）
        if (sceneComp.bgmScoutID == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.bgmScoutID = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_TEST", 0.5f);
            m_coordinator->GetComponent<SoundComponent>(sceneComp.bgmScoutID).RequestStop();
        }
        if (sceneComp.bgmActionID == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.bgmActionID = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_TEST2", 0.5f);
            m_coordinator->GetComponent<SoundComponent>(sceneComp.bgmActionID).RequestStop();
        }
        if (sceneComp.bgmCompleteID == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.bgmCompleteID = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_TEST3", 0.5f);
            m_coordinator->GetComponent<SoundComponent>(sceneComp.bgmCompleteID).RequestStop();
        }

        // 再生すべきBGMの決定
        ECS::EntityID targetBGM = ECS::INVALID_ENTITY_ID;

        if (gameState.currentMode == GameMode::SCOUTING_MODE)
        {
            targetBGM = sceneComp.bgmScoutID;
        }
        else // ACTION_MODE
        {
            if (currentItems >= 3) targetBGM = sceneComp.bgmCompleteID;
            else targetBGM = sceneComp.bgmActionID;
        }

        // BGM制御 (Scout)
        auto& scoutSound = m_coordinator->GetComponent<SoundComponent>(sceneComp.bgmScoutID);
        if (targetBGM == sceneComp.bgmScoutID) {
            if (!sceneComp.isScoutPlaying) { scoutSound.RequestPlay(0.5f, XAUDIO2_LOOP_INFINITE); sceneComp.isScoutPlaying = true; }
        }
        else {
            if (sceneComp.isScoutPlaying) { scoutSound.RequestStop(); sceneComp.isScoutPlaying = false; }
        }

        // BGM制御 (Action)
        auto& actionSound = m_coordinator->GetComponent<SoundComponent>(sceneComp.bgmActionID);
        if (targetBGM == sceneComp.bgmActionID) {
            if (!sceneComp.isActionPlaying) { actionSound.RequestPlay(0.5f, XAUDIO2_LOOP_INFINITE); sceneComp.isActionPlaying = true; }
        }
        else {
            if (sceneComp.isActionPlaying) { actionSound.RequestStop(); sceneComp.isActionPlaying = false; }
        }

        // BGM制御 (Complete)
        auto& completeSound = m_coordinator->GetComponent<SoundComponent>(sceneComp.bgmCompleteID);
        if (targetBGM == sceneComp.bgmCompleteID) {
            if (!sceneComp.isCompletePlaying) { completeSound.RequestPlay(0.5f, XAUDIO2_LOOP_INFINITE); sceneComp.isCompletePlaying = true; }
        }
        else {
            if (sceneComp.isCompletePlaying) { completeSound.RequestStop(); sceneComp.isCompletePlaying = false; }
        }

        // --- アイテムUI表示ロジック ---
        if (gameState.currentMode == GameMode::ACTION_MODE)
        {
            const std::vector<std::string> onIDs = { "UI_TEST1", "UI_TEST2", "UI_TEST3" };
            const std::vector<std::string> offIDs = { "UI_TEST1_OFF", "UI_TEST2_OFF", "UI_TEST3_OFF" };

            // UI生成
            if (sceneComp.uiEntities.empty())
            {
                for (const auto& assetID : onIDs)
                {
                    ECS::EntityID id = ECS::EntityFactory::CreateUITestEntity(
                        m_coordinator,
                        DirectX::XMFLOAT2(0.0f, 0.0f),
                        DirectX::XMFLOAT2(0.1f, 0.2f),
                        assetID
                    );
                    sceneComp.uiEntities.push_back(id);
                }
                std::cout << "All UIs Created!" << std::endl;
            }

            // UI演出更新
            if (m_coordinator->m_entityManager->GetSignature(controllerID).test(m_coordinator->GetComponentTypeID<ItemTrackerComponent>()))
            {
                int currentIndex = currentItems;
                const float CENTER_X = 0.75f;
                const float SPACING = 0.15f;
                const float BASE_Y = 0.9f;
                bool isCompleted = (currentIndex >= onIDs.size());

                for (int i = 0; i < sceneComp.uiEntities.size(); ++i)
                {
                    ECS::EntityID uiEntity = sceneComp.uiEntities[i];
                    auto& transform = m_coordinator->GetComponent<TransformComponent>(uiEntity);
                    auto& uiImage = m_coordinator->GetComponent<UIImageComponent>(uiEntity);

                    if (i >= onIDs.size()) continue;

                    if (isCompleted)
                    {
                        uiImage.assetID = offIDs[i];
                        float posX = CENTER_X + ((i - 1) * SPACING);
                        transform.position = DirectX::XMFLOAT3(posX, BASE_Y, 0.0f);
                        transform.scale = DirectX::XMFLOAT3(0.1f, 0.2f, 1.0f);
                        uiImage.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                        uiImage.depth = 0.0f;
                        continue;
                    }

                    int offset = i - currentIndex;
                    if (offset < -1 || offset > 1)
                    {
                        transform.scale = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
                        continue;
                    }

                    if (i < currentIndex) uiImage.assetID = offIDs[i];
                    else uiImage.assetID = onIDs[i];

                    float targetX = CENTER_X + (offset * SPACING);
                    transform.position = DirectX::XMFLOAT3(targetX, BASE_Y, 0.0f);

                    if (offset == 0) transform.scale = DirectX::XMFLOAT3(0.15f, 0.3f, 1.0f);
                    else transform.scale = DirectX::XMFLOAT3(0.1f, 0.2f, 1.0f);

                    uiImage.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                }

                // Complete UI
                if (isCompleted)
                {
                    if (sceneComp.completeUIEntity == ECS::INVALID_ENTITY_ID)
                    {
                        sceneComp.completeUIEntity = ECS::EntityFactory::CreateUITestEntity(
                            m_coordinator,
                            DirectX::XMFLOAT2(CENTER_X, BASE_Y),
                            DirectX::XMFLOAT2(0.5f, 0.25f),
                            "UI_TEST4"
                        );
                        auto& completeImage = m_coordinator->GetComponent<UIImageComponent>(sceneComp.completeUIEntity);
                        completeImage.depth = -0.1f;
                        std::cout << "Complete UI Created!" << std::endl;
                    }
                }
                else
                {
                    if (sceneComp.completeUIEntity != ECS::INVALID_ENTITY_ID)
                    {
                        m_coordinator->DestroyEntity(sceneComp.completeUIEntity);
                        sceneComp.completeUIEntity = ECS::INVALID_ENTITY_ID;
                    }
                }
            }
        }
        else // UI消去 (SCOUTING_MODE)
        {
            if (!sceneComp.uiEntities.empty())
            {
                for (ECS::EntityID entityID : sceneComp.uiEntities)
                {
                    m_coordinator->DestroyEntity(entityID);
                }
                sceneComp.uiEntities.clear();
                std::cout << "All UIs Destroyed." << std::endl;
            }
        }
    }
}