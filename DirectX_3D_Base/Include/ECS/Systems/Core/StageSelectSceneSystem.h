#ifndef ___STAGE_SELECT_SCENE_SYSTEM_H___
#define ___STAGE_SELECT_SCENE_SYSTEM_H___

#include "ECS/ECS.h"
// Componentの不完全宣言が可能ならここでstruct宣言、不可ならcppでinclude
struct StageSelectSceneComponent;

class StageSelectSceneSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;
    void CreateStageButton(int index, struct StageSelectSceneComponent& sceneComp, int unlockedStageMax);

public:
    void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }
    void Update(float deltaTime) override;
};

#endif