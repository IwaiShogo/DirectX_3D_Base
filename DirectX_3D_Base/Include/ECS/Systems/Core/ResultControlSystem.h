#ifndef ___RESULT_CONTROL_SYSTEM_H___
#define ___RESULT_CONTROL_SYSTEM_H___

#include "ECS/ECS.h"
#include <ECS/Components/UI/UIButtonComponent.h>

class ResultControlSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;
    float             m_timer = 0.0f;

public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
        m_timer = 0.0f;
    }

    void Update(float deltaTime) override;
};

#endif // !___RESULT_CONTROL_SYSTEM_H___
