/*****************************************************************//**
 * @file	MapGenerationSystem.h
 * @brief	MapComponentの迷路データを生成し、対応する3Dエンティティを生成・配置するシステム。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：MapGenerationSystemの定義。迷路生成とEntity配置の役割を持つ。
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
#include <random>
#include <stack>

class MazeGenerator final
{
public:
	// C++11/14で標準的な乱数生成機
	static std::mt19937 s_generator;

	/**
	 * @brief 迷路生成ロジックの本体。MapComponentのgridを書き換える。
	 * @param mapComp - 迷路データを書き込むMapComponentへの参照
	 */
	static void Generate(MapComponent& mapComp, ItemTrackerComponent& trackerComp);
private:
	// 再帰的バックトラッカーのヘルパー関数
	static void RecursiveBacktracker(MapComponent& mapComp, int x, int y);
};

/**
 * @class MapGenerationSystem
 * @brief MapComponentに基づき、ランダムなグリッド構造を生成する
 */
class MapGenerationSystem
	: public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;

public:
	void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }

	void InitMap();

	void Update() {}

	void DrawDebugLines();
	DirectX::XMFLOAT3 GetWorldPosition(int x, int y);
private:
	void SpawnMapEntities(MapComponent& mapComp);
};

#endif // !___MAP_GENERATION_SYSTEM_H___