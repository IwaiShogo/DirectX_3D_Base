#pragma once
#include "ECS/ECS.h"

class ItemProximityEffectSystem : public ECS::System
{
public:
    void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }
    void Update(float deltaTime) override;

private:
    ECS::Coordinator* m_coordinator = nullptr;
};
