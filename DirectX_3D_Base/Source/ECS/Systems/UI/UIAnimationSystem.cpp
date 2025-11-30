#include "ECS/Systems/UI/UIAnimationSystem.h"
#include <iostream>

using namespace DirectX;

namespace ECS {

    void UIAnimationSystem::Init(Coordinator* coordinator)
    {
        m_coordinator = coordinator;
    }

    void UIAnimationSystem::Update(float deltaTime)
    {
        for (auto const& entity : m_entities)
        {
            auto& anim = m_coordinator->GetComponent<UIAnimationComponent>(entity);
            auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

            if (!anim.isFinished)
            {
                anim.currentTime += deltaTime;

                // 待ち時間を過ぎた場合 -> アニメーション計算
                if (anim.currentTime >= anim.delayTime)
                {
                    float timeSinceStart = anim.currentTime - anim.delayTime;
                    float t = timeSinceStart / anim.duration;

                    t = Clamp01(t);
                    float ease = t * (2.0f - t);

                    if (anim.type == UIAnimationComponent::AnimType::Move)
                    {
                        transform.position = Lerp(anim.startValue, anim.endValue, ease);
                    }
                    else if (anim.type == UIAnimationComponent::AnimType::Scale)
                    {
                        transform.scale = Lerp(anim.startValue, anim.endValue, ease);
                    }

                    if (t >= 1.0f)
                    {
                        anim.isFinished = true;
                    }
                }
                else
                {
                    if (anim.type == UIAnimationComponent::AnimType::Move)
                    {
                        transform.position = anim.startValue;
                    }
                    else if (anim.type == UIAnimationComponent::AnimType::Scale)
                    {
                        transform.scale = anim.startValue;
                    }
                }
            }
        }
    }
    // --- ヘルパー関数の実装 ---

    float UIAnimationSystem::Clamp01(float value)
    {
        if (value < 0.0f) return 0.0f;
        if (value > 1.0f) return 1.0f;
        return value;
    }

    float UIAnimationSystem::Lerp(float start, float end, float t)
    {
        return start + (end - start) * t;
    }

    XMFLOAT3 UIAnimationSystem::Lerp(const XMFLOAT3& start, const XMFLOAT3& end, float t)
    {
        return XMFLOAT3(
            start.x + (end.x - start.x) * t,
            start.y + (end.y - start.y) * t,
            start.z + (end.z - start.z) * t
        );
    }
}