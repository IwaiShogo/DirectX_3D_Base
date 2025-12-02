#include "ECS/Systems/Core/StageInfoSceneSystem.h"
#include "ECS/Components/Core/StageInfoSceneComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/UIImageComponent.h"
#include "ECS/Components/UI/UIInteractableComponent.h"
#include "ECS/Components/UI/UIAnimationComponent.h"
#include "ECS/Components/ScoreManager.h"
#include "ECS/EntityFactory.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"
#include "Scene/StageSelectScene.h"
#include "Scene/StageinformationScene.h"

#include <iostream>
#include <string>
#include <iomanip>

using namespace DirectX;
using namespace ECS;

const float MAP_END_X = -0.4f;
const float OK_END_Y = -0.55f;
const float BACK_END_Y = -0.85f;

void StageInfoSceneSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& sceneComp = m_coordinator->GetComponent<StageInfoSceneComponent>(entity);

        // --- 生成ロジック ---

        // 背景
        if (sceneComp.BackgroundEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.BackgroundEntity = m_coordinator->CreateEntity(
                TagComponent("InformationBG"),
                TransformComponent(XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 2.0f, 1.0f)),
                UIImageComponent("UI_BG")
            );
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.BackgroundEntity).depth = 0.0f;
        }

        // コルクボード
        if (sceneComp.CorkBoardEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.CorkBoardEntity = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUICORK"),
                TransformComponent(XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.9f, 1.9f, 1.0f)),
                UIImageComponent("UI_CORK")
            );
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.CorkBoardEntity).depth = 0.1f;
        }

        // マップ画像
        if (sceneComp.MapEntity == ECS::INVALID_ENTITY_ID)
        {
            int currentStage = GameScene::s_StageNo;
            std::string mapAssetID = "UI_STAGEMAP_" + std::to_string(currentStage);

            sceneComp.MapEntity = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUIMapImage"),
                TransformComponent(XMFLOAT3(MAP_END_X, 0.35f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
                UIImageComponent(mapAssetID),
                UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.9f, 1.2f, 1.0f), 0.0f, 0.5f }
            );
            m_coordinator->GetComponent<TransformComponent>(sceneComp.MapEntity).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.MapEntity).depth = 0.2f;
        }

        // トレジャーアイコン
        if (sceneComp.TreasureEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.TreasureEntity = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUITREASURE"),
                TransformComponent(XMFLOAT3(0.4f, 0.65f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
                UIImageComponent("UI_PAPER_1"),
                UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.6f, 0.55f, 1.0f), 0.4f, 0.4f }
            );
            m_coordinator->GetComponent<TransformComponent>(sceneComp.TreasureEntity).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.TreasureEntity).depth = 0.3f;
        }

        // 警備員アイコン
        if (sceneComp.SecurityEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.SecurityEntity = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUISECURITY"),
                TransformComponent(XMFLOAT3(0.4f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
                UIImageComponent("UI_PAPER_2"),
                UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.6f, 0.55f, 1.0f), 0.4f, 0.4f }
            );
            m_coordinator->GetComponent<TransformComponent>(sceneComp.SecurityEntity).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.SecurityEntity).depth = 0.3f;
        }

        // OKボタン
        if (sceneComp.ButtonOK == ECS::INVALID_ENTITY_ID)
        {
            float sx = 0.6f, sy = 0.2f;
            sceneComp.ButtonOK = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUIOK"),
                TransformComponent(XMFLOAT3(0.4f, OK_END_Y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
                UIInteractableComponent(-1.0f, -1.0f, true),
                UIImageComponent("UI_PAPER_1"),
                UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(sx, sy, 1.0f), 0.8f, 0.5f }
            );
            m_coordinator->GetComponent<TransformComponent>(sceneComp.ButtonOK).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.ButtonOK).depth = 0.3f;

            auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonOK);
            comp.baseScaleX = sx; comp.baseScaleY = sy;
        }

        // BACKボタン
        if (sceneComp.ButtonBack == ECS::INVALID_ENTITY_ID)
        {
            float sx = 0.6f, sy = 0.2f;
            sceneComp.ButtonBack = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUIBACK"),
                TransformComponent(XMFLOAT3(0.4f, BACK_END_Y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
                UIInteractableComponent(-1.0f, -1.0f, true),
                UIImageComponent("UI_PAPER_2"),
                UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(sx, sy, 1.0f), 0.8f, 0.5f }
            );
            m_coordinator->GetComponent<TransformComponent>(sceneComp.ButtonBack).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.ButtonBack).depth = 0.3f;

            auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonBack);
            comp.baseScaleX = sx; comp.baseScaleY = sy;
        }

        // 数字
        if (sceneComp.TimeDigits[0] == ECS::INVALID_ENTITY_ID)
        {
            float startX = -0.7f;
            float startY = -0.7f;
            float stepX = 0.1f;

            for (int i = 0; i < 5; ++i)
            {
                std::string initAsset = (i == 2) ? "UI_COLON" : "UI_NUM_0";
                sceneComp.TimeDigits[i] = m_coordinator->CreateEntity(
                    TagComponent("TimeDigit" + std::to_string(i)),
                    TransformComponent(XMFLOAT3(startX + (i * stepX), startY, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)),
                    UIImageComponent(initAsset),
                    UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.2f, 1.0f), 0.4f, 0.4f }
                );
                m_coordinator->GetComponent<TransformComponent>(sceneComp.TimeDigits[i]).scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
                m_coordinator->GetComponent<UIImageComponent>(sceneComp.TimeDigits[i]).depth = 0.3f;
            }

            // 生成直後に時間を反映
            float bestTime = ScoreManager::GetBestTime(GameScene::s_StageNo);
            UpdateTimeDisplay(sceneComp, bestTime);
        }

        // カーソル
        if (sceneComp.CursorEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.CursorEntity = m_coordinator->CreateEntity(
                TagComponent("Cursor"),
                TransformComponent(XMFLOAT3(0.0f, 0.0f, -5.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.1f, 0.1f, 1.0f)),
                UIImageComponent("UI_MUSIMEGANE")
            );
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.CursorEntity).depth = 1.0f;
        }

        // --- 遷移・入力ロジック ---

        if (!sceneComp.isSceneChanging)
        {
            // OKボタン
            if (sceneComp.ButtonOK != ECS::INVALID_ENTITY_ID)
            {
                const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonOK);
                if (comp.isClicked)
                {
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST2", 0.5f);
                    sceneComp.isSceneChanging = true;
                    SceneManager::ChangeScene<GameScene>();
                    return;
                }
            }

            // BACKボタン
            if (sceneComp.ButtonBack != ECS::INVALID_ENTITY_ID)
            {
                const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.ButtonBack);
                if (comp.isClicked)
                {
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST3", 0.5f);
                    sceneComp.isSceneChanging = true;
                    SceneManager::ChangeScene<StageSelectScene>();
                    return;
                }
            }
        }
    }
}

void StageInfoSceneSystem::UpdateTimeDisplay(StageInfoSceneComponent& sceneComp, float time)
{
    int minutes = 0;
    int seconds = 0;

    if (time > 0.0f)
    {
        minutes = static_cast<int>(time) / 60;
        seconds = static_cast<int>(time) % 60;
    }

    int m10 = minutes / 10;
    int m1 = minutes % 10;
    int s10 = seconds / 10;
    int s1 = seconds % 10;

    if (sceneComp.TimeDigits[0] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(sceneComp.TimeDigits[0]).assetID = "UI_NUM_" + std::to_string(m10);
    if (sceneComp.TimeDigits[1] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(sceneComp.TimeDigits[1]).assetID = "UI_NUM_" + std::to_string(m1);
    if (sceneComp.TimeDigits[3] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(sceneComp.TimeDigits[3]).assetID = "UI_NUM_" + std::to_string(s10);
    if (sceneComp.TimeDigits[4] != ECS::INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(sceneComp.TimeDigits[4]).assetID = "UI_NUM_" + std::to_string(s1);
}