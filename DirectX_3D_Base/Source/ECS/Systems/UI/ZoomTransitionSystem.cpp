#include "ECS/Systems/UI/ZoomTransitionSystem.h"
#include "ECS/Components/UI/ZoomTransitionComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "Scene/SceneManager.h"
#include "Scene/StageinformationScene.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include <iostream>

using namespace DirectX;

namespace ECS {

    void ZoomTransitionSystem::Update(float deltaTime)
    {
        for (auto const& entity : m_entities)
        {
            auto& zoom = m_coordinator->GetComponent<ZoomTransitionComponent>(entity);
            auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

            // ★修正点：アクティブじゃない、または完了済みなら無視
            if (!zoom.isActive || zoom.isFinished) continue;

            // 1. ズーム演出計算
            transform.scale.x += zoom.zoomSpeed * deltaTime;
            transform.scale.y += zoom.zoomSpeed * deltaTime;

            transform.position.z = -2.0f;
            transform.position.x *= 0.9f;
            transform.position.y *= 0.9f;

            // 2. 閾値チェック & 遷移実行
            if (transform.scale.x > zoom.triggerScale)
            {
                std::cout << "[System] Zoom Finished." << std::endl;

                zoom.isFinished = true;

                if (zoom.targetStageNo > 0) {
                    //GameScene::SetStageNo(zoom.targetStageNo);
                }

                switch (zoom.nextScene)
                {
                case TransitionTarget::ToInfo:
                    SceneManager::ChangeScene<StageinformationScene>();
                    break;
                case TransitionTarget::ToTitle:
                    SceneManager::ChangeScene<TitleScene>();
                    break;
                case TransitionTarget::ToGame:
                    SceneManager::ChangeScene<GameScene>();
                    break;
                default:
                    break;
                }
                return;
            }
        }
    }
}