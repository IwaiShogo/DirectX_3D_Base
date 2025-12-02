#ifndef ___STAGE_INFO_SCENE_SYSTEM_H___
#define ___STAGE_INFO_SCENE_SYSTEM_H___

#include "ECS/ECS.h"

struct StageInfoSceneComponent;

class StageInfoSceneSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;
    void UpdateTimeDisplay(StageInfoSceneComponent& sceneComp, float time);

public:
    void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }
    void Update(float deltaTime) override;
};

#endif