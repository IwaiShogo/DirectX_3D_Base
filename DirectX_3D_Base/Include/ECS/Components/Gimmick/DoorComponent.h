/*****************************************************************//**
 * @file	DoorComponent.h
 * @brief	ドアの状態の管理
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/08	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___DOOR_COMPONENT_H___
#define ___DOOR_COMPONENT_H___

enum class DoorState
{
    Closed,
    Opening,
    Open,
    Closing
};

struct DoorComponent
{
    DoorState state;
    bool isLocked;          // 鍵がかかっているか（ゴール用ならアイテムコンプまでtrue）
    bool isEntrance;        // 入口か出口か

    // アニメーション用タイマーなどはAnimationComponentに任せるか、ここで簡易管理
    float timer = 0.0f;

    DoorComponent(bool entrance = false, bool locked = true)
        : state(DoorState::Closed)
        , isEntrance(entrance)
        , isLocked(locked)
        , timer(0.0f)
    {
    }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(DoorComponent)

#endif // !___DOOR_COMPONENT_H___