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

// ===== 定数・マクロ定義 =====
// マップのグリッドサイズ（10 x 10セル = 50m x 50m）
constexpr int MAP_GRID_SIZE = 20;
// 1セルあたりのワールドサイズ（5m）
constexpr float TILE_SIZE = 2.0f;
// 壁の高さ（モジュールアセットに合わせる想定で仮に5mとしておく）
constexpr float WALL_HEIGHT = 5.0f;

constexpr float MAP_CENTER_OFFSET = (MAP_GRID_SIZE / 2.0f) * TILE_SIZE; // 20.0f
constexpr float X_ADJUSTMENT = 0.5f * TILE_SIZE; // 1.0f
constexpr float Z_ADJUSTMENT = 1.0f * TILE_SIZE; // 2.0f

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

    // ゲーム要素
    Start = 3,      // プレイヤーの開始位置
    Goal = 4,       // ゴール
    Item = 5,       // 回収アイテム
    Guard = 6       // 警備員（敵）の初期位置
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
    //// 迷路のグリッドデータ (10x10)
    Cell grid[MAP_GRID_SIZE][MAP_GRID_SIZE];

    //// マップ生成に必要な座標情報
    DirectX::XMINT2 startPos = { 0, 0 }; // プレイヤー初期位置
    DirectX::XMINT2 goalPos = { MAP_GRID_SIZE - 1, MAP_GRID_SIZE - 1 }; // ゴール位置
    std::vector<DirectX::XMINT2> itemPositions; // アイテムの配置位置

    MapComponent()
    {
        // 全てのセルを初期状態(Wall)に設定
        for (int y = 0; y < MAP_GRID_SIZE; ++y)
        {
            for (int x = 0; x < MAP_GRID_SIZE; ++x)
            {
                // Cellのデフォルトコンストラクタが呼ばれ、Wall/hasWall=trueで初期化される
            }
        }
    }
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(MapComponent)

#endif // !___MAP_COMPONENT_H___