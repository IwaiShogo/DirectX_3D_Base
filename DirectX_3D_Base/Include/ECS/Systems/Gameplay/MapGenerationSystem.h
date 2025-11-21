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
#include <fstream>
#include <iostream>
#include "External/JSON/json.hpp"

using json = nlohmann::json;

/**
 * @struct	MapStageConfig
 * @brief	ステージ毎のマップ生成パラメータを定義する構造体
 */
struct MapStageConfig
{
	int gridSizeX = 20;			// グリッドサイズ（横幅）
	int gridSizeY = 20;			// グリッドサイズ（縦幅）
	float tileSize = 5.0f;		// 1セル当たりのワールドサイズ
	float wallHeight = 20.0f;	// 壁の高さ

	int minRoomSize = 3;		// 最小部屋サイズ（奇数）
	int maxRoomSize = 3;		// 最大部屋サイズ（奇数）
	int maxRoomCount = 5;		// 最大配置部屋数

	int itemCount = 3;			// 配置するアイテムの総数
	int guardCount = 1;			// 配置する警備員の総数

	std::map<CellType, int> gimmickCounts;

	float minPathPercentage = 0.25f;
};

class MapConfigLoader final
{
public:

	/**
	 * [MapStageConfig - Load]
	 * @brief	指定されたステージIDの設定をJSONファイルから読み込む
	 * 
	 * @param	[in] stageID 読み込みたいステージのID
	 * @return	成功した場合のMapStageConfig、失敗した場合はデフォルト値
	 */
	static MapStageConfig Load(const std::string& stageID)
	{
        MapStageConfig config; // デフォルト値で初期化

        // JSONファイルのパス (実行ファイルからの相対パス)
        const std::string filePath = "Assets/Config/map_config.json";

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            printf("[Error] Failed to open stage config file: %s\n", filePath.c_str());
            return config; // ファイルが開けない場合はデフォルト設定を返す
        }

        try
        {
            json jsonData;
            file >> jsonData; // パース実行

            // 指定されたStageIDが存在するかチェック
            if (jsonData.contains(stageID))
            {
                const auto& stageData = jsonData[stageID];

                // 各パラメータを読み込む (存在しない場合はデフォルト値のまま)
                if (stageData.contains("gridSizeX")) config.gridSizeX = stageData["gridSizeX"];
                if (stageData.contains("gridSizeY")) config.gridSizeY = stageData["gridSizeY"];
                if (stageData.contains("tileSize")) config.tileSize = stageData["tileSize"];
                if (stageData.contains("wallHeight")) config.wallHeight = stageData["wallHeight"];

                if (stageData.contains("minRoomSize")) config.minRoomSize = stageData["minRoomSize"];
                if (stageData.contains("maxRoomSize")) config.maxRoomSize = stageData["maxRoomSize"];
                if (stageData.contains("maxRoomCount")) config.maxRoomCount = stageData["maxRoomCount"];

                if (stageData.contains("itemCount")) config.itemCount = stageData["itemCount"];
                if (stageData.contains("guardCount")) config.guardCount = stageData["guardCount"];
                if (stageData.contains("minPathPercentage")) config.minPathPercentage = stageData["minPathPercentage"];

                // ギミック情報の読み込み (拡張用)
                if (stageData.contains("gimmicks"))
                {
                    for (const auto& gimmick : stageData["gimmicks"])
                    {
                        std::string typeName = gimmick["type"];
                        int count = gimmick["count"];

                        // 文字列からCellTypeへの変換が必要だが、
                        // 現状は対応表がないため例としてコメントアウト
                        // if (typeName == "Teleporter") config.gimmickCounts[CellType::Teleporter] = count;
                    }
                }

                printf("[Info] Loaded stage config for: %s\n", stageID.c_str());
            }
            else
            {
                printf("[Warning] Stage ID '%s' not found in config. Using default.\n", stageID.c_str());
            }
        }
        catch (const json::parse_error& e)
        {
            printf("[Error] JSON parse error: %s\n", e.what());
        }

        return config;
	}
};

class MazeGenerator final
{
public:
	// C++11/14で標準的な乱数生成機
	static std::mt19937 s_generator;

	/**
	 * @brief 迷路生成ロジックの本体。MapComponentのgridを書き換える。
	 * @param mapComp - 迷路データを書き込むMapComponentへの参照
	 */
	static void Generate(MapComponent& mapComp, ItemTrackerComponent& trackerComp, const MapStageConfig& config);
private:
	// 再帰的バックトラッカーのヘルパー関数
	static void RecursiveBacktracker(MapComponent& mapComp, const MapStageConfig& config, int x, int y);
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

	void CreateMap(const std::string& stageID);

	void Update(float deltaTime) override {}

	void DrawDebugLines();
	DirectX::XMFLOAT3 GetWorldPosition(int x, int y, const MapStageConfig& config);
private:
	void SpawnMapEntities(MapComponent& mapComp, const MapStageConfig& config);
};

#endif // !___MAP_GENERATION_SYSTEM_H___