/*****************************************************************//**
 * @file	EnemySpawnComponent.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/12/11	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ENEMY_SPAWN_COMPONENT_H___
#define ___ENEMY_SPAWN_COMPONENT_H___

#include <string>

 // 何をスポーンさせるかの種類
enum class EnemyType
{
    Guard,  // 警備員
    Taser   // テーザー（もしあれば）
};

struct EnemySpawnComponent
{
    EnemyType type;       // 生成する敵の種類
    float timer;          // 出現までの残り時間
    float effectTiming;   // 予兆エフェクトを出すタイミング（残り何秒で出すか）
    bool effectPlayed;    // 予兆再生済みフラグ

    EnemySpawnComponent(EnemyType t = EnemyType::Guard, float delay = 3.0f, float preEffect = 1.5f)
        : type(t)
        , timer(delay)
        , effectTiming(preEffect)
        , effectPlayed(false)
    {
    }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(EnemySpawnComponent)

#endif