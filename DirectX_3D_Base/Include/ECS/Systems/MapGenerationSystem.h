/*****************************************************************//**
 * @file	MapGenerationSystem.h
 * @brief	マップのグリッドデータにランダムなパターンを書き込むシステム
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

#ifndef ___MAP_GENERATION_SYSTEM_H___
#define ___MAP_GENERATION_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "ProcGen/LevelGenerator.h"	// BSPの構造体定義に依存

// ===== プロトタイプ宣言 =====
void InstantiateRoom(
	ECS::Coordinator* coordinator,
	const ProcGen::Room& room,
	const ProcGen::GridMapping& map,
	uint32_t& totalItems,
	ECS::EntityID& playerSpawnID,
	ECS::EntityID& guardSpawnID);

void InstantiateCorridor(
	ECS::Coordinator* coordinator,
	const ProcGen::Segment& seg,
	const ProcGen::GridMapping& map);

 /**
  * @class MapGenerationSystem
  * @brief MapComponentに基づき、ランダムなグリッド構造を生成する
  */
class MapGenerationSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;

public:
	void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }

	/**
	 * @brief マップ生成ロジックを実行する（Updateループではなく、SceneInit時に一度だけ呼ばれる）
	 */
	void GenerateMap(ECS::EntityID mapEntityID);
};

#endif // !___MAP_GENERATION_SYSTEM_H___