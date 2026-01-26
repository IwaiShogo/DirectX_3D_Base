/*****************************************************************//**
 * @file	MapComponent.h
 * @brief	ランダム生成される迷路マップの構造を保持するコンポーネント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：MapComponentを定義。迷路生成アルゴリズムのデータ構造を定義。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MAP_COMPONENT_H___
#define ___MAP_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include <vector>

/**
 * @enum    CellType
 * @brief   マップ要素の列挙体
 */
enum class CellType
    : std::uint8_t
{
    // 迷路生成時に使用
    Unvisited = 0,	// 未訪問（生成アルゴリズム用）
    Wall = 1,		// 壁
    Path = 2,		// 通路 (床)
    Room = 3,       // 部屋

    // ゲーム要素
    Start = 4,      // プレイヤーの開始位置
    Goal = 5,       // ゴール
    Item = 6,       // 回収アイテム
    Guard = 7,      // 警備員（敵）の初期位置
	Taser = 8,      // テーザーの初期位置
	Teleporter = 9, // テレポーターの位置
	StopTrap = 10,   // 停止トラップの位置
};

/**
 * @struct  Cell
 * @brief   迷路のセル情報を保持する構造体
 */
struct Cell
{
    CellType type = CellType::Wall; // 初期状態は全て壁
    bool visited = false;           // 迷路生成フラグ

    // 周囲の壁の有無（迷路生成で通路を掘り進めた後の壁の残存情報を保持）
    bool hasWallNorth = true;
    bool hasWallSouth = true;
    bool hasWallEast = true;
    bool hasWallWest = true;
};

/**
 * @struct MapComponent
 * @brief BSP/MSTによるマップのレイアウトデータとワールド座標へのマッピングを保持する
 */
struct MapComponent
{
    // 実行時に設定されるマップの寸法とスケール
    int gridSizeX = 0;
    int gridSizeY = 0;
    float tileSize = 0.0f;
    float wallHeight = 0.0f;

    // マップのグリッドデータ
    std::vector<std::vector<Cell>> grid;

    // マップ生成に必要な座標情報
    DirectX::XMINT2 startPos = { 0, 0 }; // プレイヤー初期位置
    DirectX::XMINT2 goalPos = { 0, 0 }; // ゴール位置
    std::vector<DirectX::XMINT2> itemPositions; // アイテムの配置位置
    struct TeleportPair { DirectX::XMINT2 posA; DirectX::XMINT2 posB; };
    std::vector<TeleportPair> teleportPairs;

    MapComponent()
    {

    }
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(MapComponent)

#endif // !___MAP_COMPONENT_H___