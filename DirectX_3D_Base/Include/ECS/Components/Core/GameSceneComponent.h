#ifndef ___GAME_SCENE_COMPONENT_H___
#define ___GAME_SCENE_COMPONENT_H___
#include "ECS/Types.h"
#include "ECS/ComponentRegistry.h"
#include <vector>
#include <string>

struct GameSceneComponent
{
    // UI用変数
    ECS::EntityID completeUIEntity = ECS::INVALID_ENTITY_ID;
    std::vector<ECS::EntityID> uiEntities;

    // BGM用変数
    ECS::EntityID bgmScoutID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID bgmActionID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID bgmCompleteID = ECS::INVALID_ENTITY_ID;

    bool isScoutPlaying = false;
    bool isActionPlaying = false;
    bool isCompletePlaying = false;
};

#include "ECS/ComponentRegistry.h"
// 既存のマクロを使用
REGISTER_COMPONENT_TYPE(GameSceneComponent)

#endif // !___GAME_SCENE_COMPONENT_H___