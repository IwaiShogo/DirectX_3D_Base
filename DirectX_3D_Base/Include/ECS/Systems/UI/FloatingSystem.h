/*****************************************************************//**
 * @file    FloatingSystem.h
 * @brief   FloatingComponentを持つエンティティを上下動させるシステム
 *********************************************************************/
#pragma once

#include "ECS/Coordinator.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/FloatingComponent.h"
#include <cmath>

class FloatingSystem : public ECS::System
{
public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    void Update(float deltaTime)
    {
        // 管理下の全エンティティ（FloatingComponent + TransformComponent持ち）をループ
        for (auto const& entity : m_entities)
        {
            // コンポーネントの取得
            auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
            auto& floating = m_coordinator->GetComponent<FloatingComponent>(entity);

            // 時間を進める
            floating.time += deltaTime;

            // サイン波でオフセットを計算
            // Y = 基準位置 + (サイン波(-1.0~1.0) * 振幅)
            float offset = std::sin(floating.time * floating.speed) * floating.amplitude;

            // 座標更新
            transform.position.y = floating.initialY + offset;

            // オプション：回転も少し加えるとより宝物っぽくなります
            // transform.rotation.y += 1.0f * deltaTime; 
        }
    }

private:
    ECS::Coordinator* m_coordinator = nullptr;
};