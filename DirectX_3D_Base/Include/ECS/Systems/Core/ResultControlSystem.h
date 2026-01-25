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
    bool m_playedStampEffect = false; // エフェクトを1回だけ出すフラグ

    std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_starTargetScale; // 追加

    float m_starEffectTimer = 0.0f;
    int   m_starEffectStep = 0;

public:

    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
        m_timer = 0.0f;
        m_playedStampEffect = false;
        m_starTargetScale.clear(); 

        m_starEffectTimer = 0.0f;
        m_starEffectStep = 0;
    }

    void Update(float deltaTime) override;
};

#endif // !___RESULT_CONTROL_SYSTEM_H___
