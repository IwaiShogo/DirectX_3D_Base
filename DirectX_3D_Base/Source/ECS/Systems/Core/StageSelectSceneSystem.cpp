#include "ECS/Systems/Core/StageSelectSceneSystem.h"
#include "ECS/Components/Core/StageSelectSceneComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/UIImageComponent.h"
#include "ECS/Components/UI/UIInteractableComponent.h"
#include "ECS/Components/UI/UIAnimationComponent.h"
#include "ECS/Components/UI/ZoomTransitionComponent.h"
#include "ECS/Components/ScoreManager.h"
#include "ECS/EntityFactory.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "Scene/StageinformationScene.h"
#include "Systems/Input.h"

#include <iostream>
#include <string>

using namespace DirectX;
using namespace ECS;

void StageSelectSceneSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& sceneComp = m_coordinator->GetComponent<StageSelectSceneComponent>(entity);

        // --- 生成ロジック ---

        // 背景
        if (sceneComp.BackgroundEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.BackgroundEntity = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUIBG"),
                TransformComponent(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(2.0f, 2.0f, 1.0f)),
                UIImageComponent("UI_BG")
            );
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.BackgroundEntity).depth = 0.0f;
        }

        // コルクボード
        if (sceneComp.CorkBoardEntity == ECS::INVALID_ENTITY_ID)
        {
            sceneComp.CorkBoardEntity = m_coordinator->CreateEntity(
                TagComponent("SelectSceneUICORK"),
                TransformComponent(XMFLOAT3(0.0f, 0.0f, 0.05f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.9f, 1.9f, 1.0f)),
                UIImageComponent("UI_CORK")
            );
            m_coordinator->GetComponent<UIImageComponent>(sceneComp.CorkBoardEntity).depth = 0.1f;
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

        // ステージ開放状況の確認
        static int unlockedStageMax = 1;
        bool needCreateButtons = false;
        for (int i = 0; i < 6; ++i) {
            if (sceneComp.StageButtons[i] == ECS::INVALID_ENTITY_ID) {
                needCreateButtons = true;
                break;
            }
        }

        if (needCreateButtons)
        {
            unlockedStageMax = 1;
            for (int i = 1; i <= 5; ++i)
            {
                if (ScoreManager::GetBestTime(i) > 0.0f) unlockedStageMax = i + 1;
                else break;
            }
        }

        // ボタン生成
        for (int i = 0; i < 6; ++i)
        {
            if (sceneComp.StageButtons[i] == ECS::INVALID_ENTITY_ID)
            {
                CreateStageButton(i, sceneComp, unlockedStageMax);
            }
        }

        // --- 入力・遷移ロジック ---

        if (IsKeyTrigger('Q'))
        {
            SceneManager::ChangeScene<TitleScene>();
            return;
        }

        if (!sceneComp.isTransitioning)
        {
            for (int i = 0; i < 6; ++i)
            {
                auto entity = sceneComp.StageButtons[i];
                if (entity != ECS::INVALID_ENTITY_ID && m_coordinator->HasComponent<UIInteractableComponent>(entity))
                {
                    const auto& comp = m_coordinator->GetComponent<UIInteractableComponent>(entity);
                    if (comp.isClicked)
                    {
                        int stageNo = i + 1;
                        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST", 0.5f);

                        GameScene::SetStageNo(stageNo);
                        GameScene::s_StageNo = stageNo;

                        if (m_coordinator->HasComponent<ZoomTransitionComponent>(entity)) {
                            auto& zoom = m_coordinator->GetComponent<ZoomTransitionComponent>(entity);
                            zoom.isActive = true;
                            zoom.targetStageNo = stageNo;
                            zoom.nextScene = TransitionTarget::ToInfo;
                        }

                        m_coordinator->RemoveComponent<UIInteractableComponent>(entity);
                        sceneComp.isTransitioning = true;
                    }
                }
            }
        }
    }
}

void StageSelectSceneSystem::CreateStageButton(int index, StageSelectSceneComponent& sceneComp, int unlockedStageMax)
{
    struct BtnParam { float x, y, sx, sy, delay; std::string img; };
    BtnParam params[] = {
        { -0.6f,  0.45f, 0.7f, 0.7f, 0.0f, "UI_PAPER_1" },
        {  0.0f,  0.6f,  0.4f, 0.3f, 0.1f, "UI_PAPER_2" },
        {  0.0f,  0.0f,  0.5f, 0.35f, 0.2f, "UI_PAPER_3" },
        {  0.6f,  0.4f,  0.55f, 0.7f, 0.3f, "UI_PAPER_1" },
        { -0.55f, -0.45f, 0.6f, 0.6f, 0.4f, "UI_PAPER_2" },
        {  0.5f, -0.5f,  0.8f, 0.8f, 0.5f, "UI_PAPER_3" }
    };

    int stageNo = index + 1;
    bool isLocked = (stageNo > unlockedStageMax);
    auto& p = params[index];
    float depth = 0.1f + (index * 0.1f);

    if (!isLocked)
    {
        sceneComp.StageButtons[index] = m_coordinator->CreateEntity(
            TagComponent("SelectSceneUI" + std::to_string(stageNo)),
            TransformComponent(XMFLOAT3(p.x, p.y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(p.sx, p.sy, 1.0f)),
            UIImageComponent(p.img),
            UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f,0.0f,0.0f), XMFLOAT3(p.sx, p.sy, 1.0f), p.delay, 0.5f },
            ZoomTransitionComponent{ false, 150.0f, 20.0f, TransitionTarget::ToInfo, 0, false },
            UIInteractableComponent(-1.0f, -1.0f, true)
        );
        auto& interact = m_coordinator->GetComponent<UIInteractableComponent>(sceneComp.StageButtons[index]);
        interact.baseScaleX = p.sx; interact.baseScaleY = p.sy;
    }
    else
    {
        sceneComp.StageButtons[index] = m_coordinator->CreateEntity(
            TagComponent("SelectSceneUI" + std::to_string(stageNo)),
            TransformComponent(XMFLOAT3(p.x, p.y, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(p.sx, p.sy, 1.0f)),
            UIImageComponent(p.img),
            UIAnimationComponent{ UIAnimationComponent::AnimType::Scale, XMFLOAT3(0.0f,0.0f,0.0f), XMFLOAT3(p.sx, p.sy, 1.0f), p.delay, 0.5f },
            ZoomTransitionComponent{ false, 150.0f, 20.0f, TransitionTarget::ToInfo, 0, false }
        );
    }
    m_coordinator->GetComponent<UIImageComponent>(sceneComp.StageButtons[index]).depth = depth;
    m_coordinator->GetComponent<TransformComponent>(sceneComp.StageButtons[index]).scale = XMFLOAT3(p.sx, p.sy, 1.0f);
}