#ifndef ___StageSelectSceneComponent_H___
#define ___StageSelectSceneComponent_H___

#include "ECS/Types.h"
#include "ECS/ComponentRegistry.h"
#include <vector>

struct StageSelectSceneComponent
{
    // UI要素
    ECS::EntityID BackgroundEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID CorkBoardEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID CursorEntity = ECS::INVALID_ENTITY_ID;

    // ステージ選択ボタン (1~6)
    ECS::EntityID StageButtons[6] = {
        ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID,
        ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID
    };

    // 装飾ボタン
    ECS::EntityID ButtonA_Entity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID ButtonB_Entity = ECS::INVALID_ENTITY_ID;

    // 演出中フラグ
    bool isTransitioning = false;
};

REGISTER_COMPONENT_TYPE(StageSelectSceneComponent)

#endif