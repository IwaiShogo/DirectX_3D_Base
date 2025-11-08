/*****************************************************************//**
 * @file	DebugDrawSystem.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/DebugDrawSystem.h"
#include "Systems/Input.h"		// IsKeyTrigger
#include "Systems/Geometory.h"	// Geometory::AddLine
#include <DirectXMath.h>

using namespace DirectX;

void DebugDrawSystem::Update()
{
    // 1. GameController Entityを探す
    ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<DebugComponent>(m_coordinator);
    if (controllerID == ECS::INVALID_ENTITY_ID) return;

    DebugComponent& debug = m_coordinator->GetComponent<DebugComponent>(controllerID);

    // 2. 【3-2: トグル機能】F1キーでデバッグモードON/OFFを切り替え
    if (IsKeyTrigger(VK_F1))
    {
        debug.isDebugModeActive = !debug.isDebugModeActive;
    }

    if (!debug.isDebugModeActive) return; // デバッグモードOFFなら描画処理をスキップ

    // 3. 【3-3: コア描画】マップ構造のライン描画
    if (debug.isDrawLinesEnabled)
    {
        // MapComponentを持つEntityを探す
        ECS::EntityID mapID = ECS::FindFirstEntityWithComponent<MapComponent>(m_coordinator);
        if (mapID != ECS::INVALID_ENTITY_ID)
        {
            DrawMapStructure(mapID);
        }
    }

    // TODO: ここに詳細ステータス表示（座標、速度、FPSなど）のロジックを追加
}


/**
 * @brief MapComponentのグリッドデータに基づき、ワイヤーフレームを描画する
 */
void DebugDrawSystem::DrawMapStructure(ECS::EntityID mapEntityID)
{
    // MapComponentを持つEntityを探す
    ECS::EntityID mapID = ECS::FindFirstEntityWithComponent<MapComponent>(m_coordinator);
    if (mapID == ECS::INVALID_ENTITY_ID) return;

    MapComponent& map = m_coordinator->GetComponent<MapComponent>(mapID);

    // LevelGeneratorのインスタンスを作成し、DebugDrawを呼び出す
    ProcGen::LevelGenerator generator; // Generatorはステートレスなので、ここで作成してOK

    generator.DebugDraw(map.layout, map.mapping);
}