#ifndef ___ZOOM_TRANSITION_SYSTEM_H___
#define ___ZOOM_TRANSITION_SYSTEM_H___

#include "ECS/SystemManager.h"
#include "ECS/Coordinator.h"

namespace ECS {
    class ZoomTransitionSystem : public System
    {
    public:
        void Init(Coordinator* coordinator) override { m_coordinator = coordinator; }
        void Update(float deltaTime) override;

    private:
        Coordinator* m_coordinator;
    };
}
#endif
