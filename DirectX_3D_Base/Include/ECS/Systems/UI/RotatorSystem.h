/*****************************************************************//**
 * @file    RotatorSystem.h
 * @brief   RotatorComponentを持つエンティティを回転させるシステム
 *********************************************************************/
#pragma once

#include "ECS/Coordinator.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/RotatorComponent.h"

class RotatorSystem : public ECS::System
{
public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    void Update(float deltaTime) override
    {
        for (auto const& entity : m_entities)
        {
            auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
            auto& rotator = m_coordinator->GetComponent<RotatorComponent>(entity);

            // 回転を加算
            transform.rotation.x += rotator.rotationSpeed.x * deltaTime;
            transform.rotation.y += rotator.rotationSpeed.y * deltaTime;
            transform.rotation.z += rotator.rotationSpeed.z * deltaTime;

            // 角度が大きくなりすぎないようにリセット (2π ≒ 6.28318f)
            constexpr float PI_2 = 6.2831853f;
            if (transform.rotation.x > PI_2) transform.rotation.x -= PI_2;
            if (transform.rotation.y > PI_2) transform.rotation.y -= PI_2;
            if (transform.rotation.z > PI_2) transform.rotation.z -= PI_2;
        }
    }

private:
    ECS::Coordinator* m_coordinator = nullptr;
};