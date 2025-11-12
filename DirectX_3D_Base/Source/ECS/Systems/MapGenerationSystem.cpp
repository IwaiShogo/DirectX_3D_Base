/*****************************************************************//**
 * @file	MapGenerationSystem.cpp
 * @brief	ランダム迷路のデータ生成ロジックと、そのデータに基づいたEntity生成ロジックの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：MapGenerationSystem.cppを作成。迷路生成アルゴリズム (再帰的バックトラッカー) を実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- xx：
 * 
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/EntityFactory.h" 
#include "ECS/ECS.h"

#include <algorithm>
#include <random>
#include <cmath>

using namespace DirectX;
using namespace ECS;

// ===================================================================
// MazeGenerator 実装
// ===================================================================

// 静的メンバ変数の初期化
std::mt19937 MazeGenerator::s_generator(std::random_device{}());

/**
 * @brief 迷路生成ロジックの本体。MapComponentのgridを書き換える。
 * @param mapComp - 迷路データを書き込むMapComponentへの参照
 */
void MazeGenerator::Generate(MapComponent& mapComp)
{
    // 全てのセルを未訪問(Unvisited)にリセット
    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            mapComp.grid[y][x].visited = false;
            mapComp.grid[y][x].type = CellType::Wall; // 初期状態はすべて壁
            mapComp.grid[y][x].hasWallNorth = true;
            mapComp.grid[y][x].hasWallSouth = true;
            mapComp.grid[y][x].hasWallEast = true;
            mapComp.grid[y][x].hasWallWest = true;
        }
    }

    // プレイヤーのスタート位置を決定 (左上)
    mapComp.startPos = { 0, 0 };
    // ゴール位置を決定 (右下)
    mapComp.goalPos = { MAP_GRID_SIZE - 1, MAP_GRID_SIZE - 1 };

    // 再帰的バックトラッカーを開始
    RecursiveBacktracker(mapComp, mapComp.startPos.x, mapComp.startPos.y);

    // スタートとゴールをPathとしてマークし、ゲーム要素として指定
    mapComp.grid[mapComp.startPos.y][mapComp.startPos.x].type = CellType::Start;
    mapComp.grid[mapComp.goalPos.y][mapComp.goalPos.x].type = CellType::Goal;

    // --------------------------------------------------------------------------------
    // 【ステップ2-3: アイテムと警備員の配置ロジック】
    // --------------------------------------------------------------------------------

    // 配置可能なパス座標のリストを収集
    std::vector<XMINT2> availablePathPositions;
    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            // 通路であり、かつスタート・ゴールではない位置を候補とする
            if (mapComp.grid[y][x].type == CellType::Path)
            {
                availablePathPositions.push_back({ x, y });
            }
        }
    }

    // ランダムな位置を選択できるようにシャッフル
    std::shuffle(availablePathPositions.begin(), availablePathPositions.end(), s_generator);

    // 1. アイテム (3〜5個) の配置
    constexpr int MIN_ITEMS = 3;
    constexpr int MAX_ITEMS = 5;
    // 乱数でアイテム数を決定 (std::uniform_int_distributionを使用)
    std::uniform_int_distribution<int> itemDist(MIN_ITEMS, MAX_ITEMS);
    int itemsToPlace = itemDist(s_generator);

    // 配置可能な数が足りない場合は、リストのサイズを上限とする
    itemsToPlace = std::min((int)availablePathPositions.size(), itemsToPlace);

    for (int i = 0; i < itemsToPlace; ++i)
    {
        XMINT2 pos = availablePathPositions.back();
        availablePathPositions.pop_back(); // 使用した座標はリストから削除

        mapComp.grid[pos.y][pos.x].type = CellType::Item;
        mapComp.itemPositions.push_back(pos); // ItemComponentで管理するために位置情報を保存
    }

    // 2. 警備員（敵）の配置 (ここでは1体とする)
    constexpr int GUARDS_TO_PLACE = 1;

    if (availablePathPositions.size() >= GUARDS_TO_PLACE)
    {
        XMINT2 pos = availablePathPositions.back();
        availablePathPositions.pop_back();

        mapComp.grid[pos.y][pos.x].type = CellType::Guard;
        // 警備員の位置はMapComponentに専用の変数を用意することもできるが、ここではグリッドに直接マークする
    }
}

/**
 * @brief 再帰的バックトラッカーの主要ロジック
 * @param mapComp - マップデータ
 * @param x - 現在のセルのX座標
 * @param y - 現在のセルのY座標
 */
void MazeGenerator::RecursiveBacktracker(MapComponent& mapComp, int x, int y)
{
    // 現在のセルを訪問済みとしてマーク
    mapComp.grid[y][x].visited = true;
    mapComp.grid[y][x].type = CellType::Path; // 通路を掘る

    // 進行方向の候補 (dx, dy) を定義 (北, 東, 南, 西)
    // 迷路のセルは2x2の壁と壁の間に1マスの通路が掘られるイメージ
    std::vector<std::pair<int, int>> directions = {
        { 0, -2 },	// 北 (-2ステップ)
        { 2, 0 },	// 東 (+2ステップ)
        { 0, 2 },	// 南 (+2ステップ)
        { -2, 0 }	// 西 (-2ステップ)
    };

    // 進行方向をシャッフル
    std::shuffle(directions.begin(), directions.end(), s_generator);

    for (const auto& dir : directions)
    {
        int dx = dir.first;
        int dy = dir.second;

        // 次のセル座標
        int nx = x + dx;
        int ny = y + dy;

        // 間の壁のセル座標
        int wallX = x + dx / 2;
        int wallY = y + dy / 2;

        // 境界チェック (nx, ny がグリッド内か)
        if (nx >= 0 && nx < MAP_GRID_SIZE && ny >= 0 && ny < MAP_GRID_SIZE)
        {
            if (!mapComp.grid[ny][nx].visited)
            {
                // ★ 修正点：間の壁セルを通路にする ★
                mapComp.grid[wallY][wallX].type = CellType::Path; // 間のセルを通路として掘る

                // 次のセルへ進む
                RecursiveBacktracker(mapComp, nx, ny);
            }
        }
    }
}

// ===================================================================
// MapGenerationSystem 実装
// ===================================================================

/**
 * @brief マップ生成システムを初期化し、迷路を生成する。
 */
void MapGenerationSystem::InitMap()
{
    // MapComponentを持つエンティティは一つだけとする (ゲーム全体を司るController Entity)
    EntityID mapEntity = FindFirstEntityWithComponent<MapComponent>(m_coordinator);

    // 見つからなかった場合は、専用のEntityを生成してアタッチ
    if (mapEntity == INVALID_ENTITY_ID)
    {
        //mapEntity = m_coordinator->CreateEntity();
        //m_coordinator->AddComponent(mapEntity, MapComponent{});
        // MapComponentがEntityFactoryで生成されたGameControllerにアタッチされる可能性もあるため、
        // 必ずしもここでTransformなどを持たせる必要はない。ここではデータのみとする。
        return;
    }

    // MapComponentを取得して迷路を生成
    MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);

    // 1. 迷路データの生成
    MazeGenerator::Generate(mapComp);

    // 2. 3D空間へのEntity配置
    SpawnMapEntities(mapComp);
}

/**
 * @brief グリッド座標をワールド座標に変換するヘルパー関数
 */
XMFLOAT3 MapGenerationSystem::GetWorldPosition(int x, int y)
{
    // マップの中心をワールド原点(0, 0, 0)とするためのオフセット計算
    // 10x10グリッドの場合、中心は (5, 5) のセルとセルの間
    constexpr float MAP_CENTER_OFFSET = (MAP_GRID_SIZE / 2.0f) * TILE_SIZE;

    XMFLOAT3 pos;
    // X座標: (グリッドX * タイルサイズ) - オフセット + (タイル半分のオフセット)
    pos.x = (float)x * TILE_SIZE - MAP_CENTER_OFFSET + (TILE_SIZE / 2.0f);
    // Y座標: 地面なので0 (高さの概念は後で導入可能)
    pos.y = 0.0f;
    // Z座標: (グリッドY * タイルサイズ) - オフセット + (タイル半分のオフセット)
    // DirectXのZ軸はUnity/Unrealとは異なり「奥がマイナス」のことが多いが、ここでは「グリッドY=0が手前」で進める
    pos.z = (float)y * TILE_SIZE - MAP_CENTER_OFFSET + (TILE_SIZE / 2.0f);

    return pos;
}


/**
 * @brief 生成されたマップデータに基づき、3D空間に壁、床、オブジェクトを配置する
 */
void MapGenerationSystem::SpawnMapEntities(MapComponent& mapComp)
{
    // **注意**: このシステムでEntityFactoryを直接呼び出すことで、
    // Entityの作成とコンポーネントの付与を共通化します。

    // 各セルを走査して壁を配置する
    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            Cell& cell = mapComp.grid[y][x];
            XMFLOAT3 cellCenter = GetWorldPosition(x, y);

            // 1. 床 Entityの配置とコリジョン付与
            // 通路、スタート、ゴール、アイテム、警備員のセルには必ず床を置く
            if (cell.type != CellType::Wall && cell.type != CellType::Unvisited)
            {
                // グリッド一つ分の床 Entityを生成
                // TILE_SIZE (5.0f) と同じ大きさの床タイルを、セルの中心位置に配置
                //EntityFactory::CreateGround(
                //    m_coordinator,
                //    cellCenter,
                //    XMFLOAT3(5.0f, 0.1f, 5.0f) // 1.0fはEntityFactory内で5mスケールに変換される前提
                //);

                // 【注意】EntityFactory::CreateGround のシグネチャが不明確なため、
                // 以前のCreateGround (position, scale) の代わりに、
                // マップタイルとして使う EntityFactory::CreateCorridor/CreateTile などがあると想定し、
                // CreateGroundを修正して利用します。

                // 補足: EntityFactory::CreateGroundがコリジョンを持っている必要があります。
                // EntityFactory::CreateGroundの実装を確認・修正します。
            }

            // セルが通路（PathまたはStart/Goal）の場合のみ、周囲の壁をチェックして配置する
            if (cell.type != CellType::Wall)
            {
                // 1-3. 既存アセット（壁・床モデル）の選定
                // アセットリストから `barrierMedium.fbx` を仮の壁モデルとして選択します。

                // 北の壁 (Z軸 - 方向)
                if (cell.hasWallNorth)
                {
                    XMFLOAT3 wallPos = cellCenter;
                    wallPos.z -= TILE_SIZE / 2.0f; // セルの境界に移動
                    // CreateWallのシグネチャに合わせてEntityを生成
                    EntityFactory::CreateWall(
                        m_coordinator,
                        wallPos,
                        XMFLOAT3(TILE_SIZE, WALL_HEIGHT, 0.5f), // X: 5m幅, Y: 5m高さ, Z: 薄さ
                        0.0f); // 回転なし (0度)
                }

                // 南の壁 (Z軸 + 方向)
                if (cell.hasWallSouth)
                {
                    XMFLOAT3 wallPos = cellCenter;
                    wallPos.z += TILE_SIZE / 2.0f;
                    EntityFactory::CreateWall(
                        m_coordinator,
                        wallPos,
                        XMFLOAT3(TILE_SIZE, WALL_HEIGHT, 0.5f),
                        0.0f);
                }

                // 東の壁 (X軸 + 方向)
                if (cell.hasWallEast)
                {
                    XMFLOAT3 wallPos = cellCenter;
                    wallPos.x += TILE_SIZE / 2.0f; // セルの境界に移動
                    // 90度Y軸回転でX軸方向に壁を配置
                    EntityFactory::CreateWall(
                        m_coordinator,
                        wallPos,
                        XMFLOAT3(TILE_SIZE, WALL_HEIGHT, 0.5f),
                        XM_PIDIV2); // 90度 (π/2)
                }

                // 西の壁 (X軸 - 方向)
                if (cell.hasWallWest)
                {
                    XMFLOAT3 wallPos = cellCenter;
                    wallPos.x -= TILE_SIZE / 2.0f;
                    // 90度Y軸回転
                    EntityFactory::CreateWall(
                        m_coordinator,
                        wallPos,
                        XMFLOAT3(TILE_SIZE, WALL_HEIGHT, 0.5f),
                        XM_PIDIV2);
                }
            }

            // --------------------------------------------------------------------------------
            // 【特殊オブジェクト（スタート/ゴール/アイテム/警備員）の配置】
            // --------------------------------------------------------------------------------
            switch (cell.type)
            {
            case CellType::Start:
                // プレイヤーをスタート位置に配置
                cellCenter.y += 3.0f;
                EntityFactory::CreatePlayer(m_coordinator, cellCenter);
                break;
            case CellType::Goal:
                // ゴールオブジェクトを配置
                // KayKitアセットから、ここでは `gateLarge_teamRed.fbx` を仮のゴールモデルとして選択
                //EntityFactory::CreateGate(m_coordinator, cellCenter);
                break;
            case CellType::Item:
                // アイテムを配置
                // KayKitアセットから、ここでは `diamond_teamYellow.fbx` を仮のアイテムモデルとして選択
                EntityFactory::CreateCollectable(m_coordinator, cellCenter);
                break;
            case CellType::Guard:
                // 警備員（敵）を配置
                // KayKitアセットから、ここでは `character_dog.fbx` を仮の警備員モデルとして選択
                EntityFactory::CreateGuard(m_coordinator, cellCenter);
                break;
            default:
                // Path (通路) や Wall はここでは何もしない
                break;
            }
        }
    }
}