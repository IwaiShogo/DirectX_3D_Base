#ifndef ___RESULT_CONTROL_SYSTEM_H___
#define ___RESULT_CONTROL_SYSTEM_H___

#include "ECS/ECS.h"
#include <ECS/Components/UI/UIButtonComponent.h>

#include <unordered_map>

class ResultControlSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;
    float m_timer = 0.0f;

    std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_starTargetScale; // ’Ç‰Á

public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
        m_timer = 0.0f;
        m_starTargetScale.clear(); // ’Ç‰Á
    }

    void Update(float deltaTime) override;
};

#endif // !___RESULT_CONTROL_SYSTEM_H___
