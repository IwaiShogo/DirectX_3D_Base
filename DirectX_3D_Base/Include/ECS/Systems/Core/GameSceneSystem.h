#ifndef ___GAME_SCENE_SYSTEM_H___
#define ___GAME_SCENE_SYSTEM_H___

#include "ECS/ECS.h"

class GameSceneSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

public:
    void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }
    void Update(float deltaTime) override;
};

#endif