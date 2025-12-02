#ifndef ___StageInfoSceneComponent_H___
#define ___StageInfoSceneComponent_H___

#include "ECS/Types.h"
#include "ECS/ComponentRegistry.h"

struct StageInfoSceneComponent
{
    ECS::EntityID BackgroundEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID CorkBoardEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID CursorEntity = ECS::INVALID_ENTITY_ID;

    ECS::EntityID MapEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID TreasureEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID SecurityEntity = ECS::INVALID_ENTITY_ID;

    ECS::EntityID ButtonOK = ECS::INVALID_ENTITY_ID;
    ECS::EntityID ButtonBack = ECS::INVALID_ENTITY_ID;

    // éûä‘ï\é¶ópåÖ (00:00) 4åÖ+ÉRÉçÉì
    ECS::EntityID TimeDigits[5] = { ECS::INVALID_ENTITY_ID };

    bool isSceneChanging = false;
};

REGISTER_COMPONENT_TYPE(StageInfoSceneComponent)

#endif