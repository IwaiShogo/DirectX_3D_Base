/*****************************************************************//**
 * @file    AnimationSystem.h
 * @brief   Updates animation playback for entities that have AnimationComponent.
 *
 * Notes:
 * - Advances Model::Step(deltaTime)
 * - Applies node-animation (non-skeletal) transform results to TransformComponent
 *   with root-motion delta so that "start position" can be controlled externally.
 *********************************************************************/

#ifndef ___ANIMATION_SYSTEM_H___
#define ___ANIMATION_SYSTEM_H___

#include "ECS/ECS.h"
#include <DirectXMath.h>
#include <unordered_map>
#include <unordered_set>

class AnimationSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

    // For node-animation (models without bones): apply translation as delta (root motion).
    std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_prevNodeAnimPos;
    std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_prevNodeAnimRot;
    std::unordered_set<ECS::EntityID> m_resetNodeAnimPos;

public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    void Update(float deltaTime) override;
};

#endif // !___ANIMATION_SYSTEM_H___
