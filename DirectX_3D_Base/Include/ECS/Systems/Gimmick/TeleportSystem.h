#ifndef ___TELEPORT_SYSTEM_H___
#define ___TELEPORT_SYSTEM_H___

#include "ECS/ECS.h"

class TeleportSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

public:
    void Init(ECS::Coordinator* coordinator) override;
    void Update(float deltaTime) override;
};

#endif