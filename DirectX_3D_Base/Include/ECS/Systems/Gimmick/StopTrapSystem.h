#ifndef ___STOPTRAP_SYSTEM_H___
#define ___STOPTRAP_SYSTEM_H___

#include "ECS/ECS.h"

class StopTrapSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;
public:
    void Init(ECS::Coordinator* coordinator) override;
    void Update(float deltaTime) override;


};

#endif // !___STOPTRAP_SYSTEM_H___