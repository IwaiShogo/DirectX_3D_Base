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

#include "ECS/Components/Gameplay/MapComponent.h"
#include "ECS/Components/Gameplay/ItemTrackerComponent.h"

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

	int guardCount = 1;			// 配置する警備員の総数
    int taserCount = 3;         // 配置するテーザーの総数

	int teleportPairCount = 0;    // 配置するテレポーターの総数

    //アイテム順序モードオン/オフ
	float minPathPercentage = 0.25f;
    bool useOrderedCollection = false;

    float timeLimitStar = 180.0f;   // 評価用タイムリミット
    std::vector<std::string> items;

    // ギミック情報
    struct GimmickInfo {
        std::string type;
        int count;
    };
    std::vector<GimmickInfo> gimmicks;
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
			json j;
			file >> j;

			if (j.contains(stageID))
			{
				auto& val = j[stageID];

				// 基本設定
				config.gridSizeX = val.value("gridSizeX", 20);
				config.gridSizeY = val.value("gridSizeY", 20);
				config.tileSize = val.value("tileSize", 5.0f);
				config.wallHeight = val.value("wallHeight", 20.0f);

				config.minRoomSize = val.value("minRoomSize", 3);
				config.maxRoomSize = val.value("maxRoomSize", 6);
				config.maxRoomCount = val.value("maxRoomCount", 5);

				config.guardCount = val.value("guardCount", 1);
				config.teleportPairCount = val.value("teleportPairCount", 0);
				config.minPathPercentage = val.value("minPathPercentage", 0.3f);
				config.useOrderedCollection = val.value("useOrderedCollection", false);

				// ★追加: タイムリミット読み込み
				config.timeLimitStar = val.value("timeLimitStar", 180.0f);

				// ★追加: アイテムリスト読み込み
				if (val.contains("items") && val["items"].is_array())
				{
					for (const auto& item : val["items"]) {
						config.items.push_back(item.get<std::string>());
					}
				}
				// 旧互換: itemsがない場合は itemCount だけ読んでダミーを入れる等の対応も可
				else if (val.contains("itemCount"))
				{
					int count = val.value("itemCount", 3);
					for (int k = 0; k < count; ++k) config.items.push_back("Takara_Ring");
				}

				// ギミック読み込み
				if (val.contains("gimmicks") && val["gimmicks"].is_array())
				{
					for (auto& gim : val["gimmicks"])
					{
						MapStageConfig::GimmickInfo info;
						info.type = gim.value("type", "");
						info.count = gim.value("count", 0);
						config.gimmicks.push_back(info);
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
	int m_itemSpawnIndex = 0;

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