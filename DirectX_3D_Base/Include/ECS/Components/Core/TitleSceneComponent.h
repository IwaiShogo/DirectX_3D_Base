#ifndef ___TitleSceneComponent_H___
#define ___TitleSceneComponent_H___
#include "ECS/Types.h"
#include "ECS/ComponentRegistry.h"

struct TitleSceneComponent
{
	ECS::EntityID TitleBackGroundEntity = ECS::INVALID_ENTITY_ID;
	ECS::EntityID TitleLogoEntity = ECS::INVALID_ENTITY_ID;			// タイトルロゴエンティティ

	ECS::EntityID ButtonTitleStartEntity = ECS::INVALID_ENTITY_ID;	// スタートボタンエンティティ
	ECS::EntityID ButtonTitleExitEntity = ECS::INVALID_ENTITY_ID;	// 終了ボタンエンティティ
	
	ECS::EntityID Button_NewGameEntity = ECS::INVALID_ENTITY_ID;	//	はじめから	//未実装
	ECS::EntityID Button_ContinueEntity = ECS::INVALID_ENTITY_ID;	//　続きから	//未実装

	ECS::EntityID interactableEntity = ECS::INVALID_ENTITY_ID;
	std::vector<ECS::EntityID> uiEntities;

	bool showGameStartOptions = false;
};

#include "ECS/ComponentRegistry.h"
//既存のマクロを使用
REGISTER_COMPONENT_TYPE(TitleSceneComponent)

#endif //!___TitleSceneComponent_H___