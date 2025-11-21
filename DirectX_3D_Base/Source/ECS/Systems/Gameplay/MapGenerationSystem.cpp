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
#include "Systems/Geometory.h"

#include <algorithm>
#include <random>
#include <cmath>
#include <map>
#include <set>

using namespace DirectX;
using namespace ECS;

namespace DirectX {
    /**
     * @brief XMINT2に対する等価比較演算子（operator==）を定義する
     * @details std::removeなどの標準アルゴリズムがDirectX::XMINT2を比較できるようにするために必要
     */
    inline bool operator==(const XMINT2& lhs, const XMINT2& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    /**
     * @brief XMINT2に対する比較演算子（operator<）を定義する (std::map, std::set用)
     */
    inline bool operator<(const XMINT2& lhs, const XMINT2& rhs)
    {
        if (lhs.y != rhs.y) return lhs.y < rhs.y;
        return lhs.x < rhs.x;
    }
}

// ===================================================================
// MazeGenerator 実装
// ===================================================================

// 静的メンバ変数の初期化
std::mt19937 MazeGenerator::s_generator(std::random_device{}());

/**
 * @brief 迷路生成ロジックの本体。MapComponentのgridを書き換える。
 * @param mapComp - 迷路データを書き込むMapComponentへの参照
 */
void MazeGenerator::Generate(MapComponent& mapComp, ItemTrackerComponent& trackerComp)
{
    // BUG-01修正強化: マップ生成の品質保証のための定数
    constexpr int MAX_GENERATION_ATTEMPTS = 10;
    // 迷路として許容できる最小通路セル割合 (50x50グリッドの場合、25%で625セル)
    constexpr float MIN_PATH_PERCENTAGE = 0.25f;
    constexpr int MIN_PATH_COUNT = (int)(MAP_GRID_SIZE * MAP_GRID_SIZE * MIN_PATH_PERCENTAGE);
    int MAX_INDEX = MAP_GRID_SIZE - 1;
    int directions[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };

    // 成功するまでマップ生成プロセスを繰り返す
    for (int attempt = 0; attempt < MAX_GENERATION_ATTEMPTS; ++attempt)
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

        // --------------------------------------------------------------------------------
        // 【ステップ5-2: 部屋配置ロジックの追加】
        // --------------------------------------------------------------------------------

        // 部屋パラメータ
        constexpr int MIN_ROOM_SIZE = 3;    // 最小部屋サイズ (奇数)
        constexpr int MAX_ROOM_SIZE = 3;    // 最大部屋サイズ (奇数)
        constexpr int MAX_ROOM_COUNT = 5;   // 最大配置部屋数

        // 部屋の配置を試行する
        for (int i = 0; i < MAX_ROOM_COUNT; ++i) // 失敗を見越して多めに試行
        {
            // 部屋のサイズをランダムに決定 (奇数に限定して迷路生成との接続を容易にする)
            std::uniform_int_distribution<int> sizeDist(MIN_ROOM_SIZE / 2, MAX_ROOM_SIZE / 2);
            int halfWidth = sizeDist(s_generator) * 2 + 1; // 3, 5, 7, 9...
            int halfHeight = sizeDist(s_generator) * 2 + 1;

            // 部屋の左上角の座標をランダムに決定
            std::uniform_int_distribution<int> posDist(1, MAP_GRID_SIZE - MAX_ROOM_SIZE - 1);
            int startX = posDist(s_generator);
            int startY = posDist(s_generator);

            // 部屋のサイズを調整 (境界からはみ出さないように)
            int roomWidth = halfWidth;
            int roomHeight = halfHeight;

            if (startX + roomWidth >= MAP_GRID_SIZE - 1) roomWidth = MAP_GRID_SIZE - startX - 2;
            if (startY + roomHeight >= MAP_GRID_SIZE - 1) roomHeight = MAP_GRID_SIZE - startY - 2;

            // 部屋のサイズが奇数で、かつ最小サイズ以上であることを保証
            roomWidth = (roomWidth % 2 == 0) ? roomWidth - 1 : roomWidth;
            roomHeight = (roomHeight % 2 == 0) ? roomHeight - 1 : roomHeight;

            if (roomWidth < MIN_ROOM_SIZE || roomHeight < MIN_ROOM_SIZE) continue;

            // 重複チェックはここでは省略し、シンプルに上書きを許容する

            // 部屋の領域をPathとしてマークし、visited=true (迷路生成の対象外とする)
            for (int y = startY; y < startY + roomHeight; ++y)
            {
                for (int x = startX; x < startX + roomWidth; ++x)
                {
                    mapComp.grid[y][x].type = CellType::Room;
                    mapComp.grid[y][x].visited = true; // 迷路生成アルゴリズムの対象外
                }
            }
        }

        // --------------------------------------------------------------------------------
        // 【ステップ5-3: 迷路生成の実行】
        // --------------------------------------------------------------------------------
        int startX = mapComp.startPos.x;
        int startY = mapComp.startPos.y;

        // 開始セルを確実にPathとしてマークし、visitedをリセット
        mapComp.grid[startY][startX].type = CellType::Path;
        mapComp.grid[startY][startX].visited = false;

        RecursiveBacktracker(mapComp, startX, startY);

        // --------------------------------------------------------------------------------
        // 5. 迷路生成の後に、最外周の壁セルをvisited=trueに固定する
        // --------------------------------------------------------------------------------
        // 迷路生成が完了した後で、外周を確実に固定します。
        int MAX_INDEX = MAP_GRID_SIZE - 1;
        for (int i = 0; i < MAP_GRID_SIZE; ++i)
        {
            // 1. 上下の境界 (y=0 と y=MAX_INDEX)
            mapComp.grid[0][i].type = CellType::Wall;
            mapComp.grid[0][i].visited = true;

            mapComp.grid[MAX_INDEX][i].type = CellType::Wall;
            mapComp.grid[MAX_INDEX][i].visited = true;

            // 2. 左右の境界 (x=0 と x=MAX_INDEX)
            mapComp.grid[i][0].type = CellType::Wall;
            mapComp.grid[i][0].visited = true;

            mapComp.grid[i][MAX_INDEX].type = CellType::Wall;
            mapComp.grid[i][MAX_INDEX].visited = true;
        }

        // --------------------------------------------------------------------------------
        // 【確実な接続保証ロジック（Flood Fillによる連結成分の結合）】
        // --------------------------------------------------------------------------------
        bool mapWasConnected = false;

        // 接続プロセスを最大でMAP_GRID_SIZE分繰り返す (複雑な分断でも接続を保証)
        for (int connection_iter = 0; connection_iter < MAP_GRID_SIZE; ++connection_iter)
        {
            bool wallDestroyed = false;

            // a) Flood Fillのための visitedフラグをリセット
            for (int y = 0; y < MAP_GRID_SIZE; ++y)
            {
                for (int x = 0; x < MAP_GRID_SIZE; ++x)
                {
                    if (mapComp.grid[y][x].type != CellType::Wall)
                    {
                        mapComp.grid[y][x].visited = false;
                    }
                }
            }

            // b) Flood Fill (BFS) を実行: スタートから到達可能なセルをマーク
            std::vector<XMINT2> bfs_queue;
            bfs_queue.push_back(mapComp.startPos);
            mapComp.grid[mapComp.startPos.y][mapComp.startPos.x].visited = true;

            size_t head = 0;
            while (head < bfs_queue.size())
            {
                XMINT2 current = bfs_queue[head++];

                for (int i = 0; i < 4; ++i)
                {
                    int nx = current.x + directions[i][0];
                    int ny = current.y + directions[i][1];

                    if (nx > 0 && nx < MAX_INDEX && ny > 0 && ny < MAX_INDEX &&
                        mapComp.grid[ny][nx].type != CellType::Wall &&
                        !mapComp.grid[ny][nx].visited)
                    {
                        mapComp.grid[ny][nx].visited = true;
                        bfs_queue.push_back({ nx, ny });
                    }
                }
            }

            // c) 接続の試行: 未到達エリアと到達エリアを繋ぐ壁を探して破壊
            for (int y = 1; y < MAX_INDEX; ++y)
            {
                for (int x = 1; x < MAX_INDEX; ++x)
                {
                    if (mapComp.grid[y][x].type == CellType::Wall)
                    {
                        bool bordersReachable = false;
                        bool bordersUnreachable = false;

                        for (int i = 0; i < 4; ++i)
                        {
                            int nx = x + directions[i][0];
                            int ny = y + directions[i][1];

                            if (nx > 0 && nx < MAX_INDEX && ny > 0 && ny < MAX_INDEX &&
                                mapComp.grid[ny][nx].type != CellType::Wall)
                            {
                                if (mapComp.grid[ny][nx].visited) bordersReachable = true;
                                else bordersUnreachable = true;
                            }
                        }

                        if (bordersReachable && bordersUnreachable)
                        {
                            mapComp.grid[y][x].type = CellType::Path;
                            wallDestroyed = true;
                            // 接続が発生したため、内側のループを抜けて Flood Fill の再実行へ
                            goto restart_connection_loop;
                        }
                    }
                }
            }

        restart_connection_loop:;

            if (!wallDestroyed)
            {
                mapWasConnected = true;
                break; // 完全に接続された
            }
        }

        // --------------------------------------------------------------------------------
        // 【BUG-01修正強化: 品質チェックと再試行】
        // --------------------------------------------------------------------------------

        // 1. 通路/部屋セルの総数をカウント
        int totalPathCells = 0;
        for (int y = 0; y < MAP_GRID_SIZE; ++y)
        {
            for (int x = 0; x < MAP_GRID_SIZE; ++x)
            {
                if (mapComp.grid[y][x].type != CellType::Wall && mapComp.grid[y][x].type != CellType::Unvisited)
                {
                    totalPathCells++;
                }
            }
        }

        // 2. 迷路の密度が低すぎる（ほぼ壁）の場合、再試行
        if (totalPathCells < MIN_PATH_COUNT)
        {
            continue; // for ループの先頭に戻り、新しいマップ生成を試みる
        }

        // 3. 接続性の最終チェック (Flood Fillが成功しているか)
        if (!mapWasConnected)
        {
            // 非常に稀なケースだが、接続が保証されない場合は再試行
            continue;
        }

        // 成功: 品質保証をパスした場合、ループを抜けて配置に進む
        goto end_generate_loop;

    } // for (int attempt = 0; ...
    // 最大試行回数に達した場合、最後に生成されたマップで続行する

end_generate_loop:;

    // --------------------------------------------------------------------------------
    // 【ステップ2-1: 行き止まり（袋小路）除去ロジックの導入】
    // --------------------------------------------------------------------------------
    // マップの回遊性を高め、一本道の通路を完全に削減するまで繰り返す

    // 4方向 (2マス先)
    std::vector<std::pair<int, int>> directions2x2 = { { 0, -2 }, { 2, 0 }, { 0, 2 }, { -2, 0 } };
    // 4方向 (1マス先)
    int directions_1step[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
    bool deadEndConnected = false;

    // デバッグ用カウンタの宣言
    int totalDeadEndsResolved = 0; // 解消された総数のみを追跡する

    // ★ 修正：do-whileループの構造を根本的に変更 ★
    // 変更がなくなるまでループし、デッドエンドのリストを更新する
    bool deadEndsWereProcessed;
    do
    {
        deadEndsWereProcessed = false;
        // 検出された全てのデッドエンドを保持するコンテナ
        std::vector<XMINT2> deadEndCandidates;

        // 1. 全てのPathセルを走査し、真の「行き止まり」セル（openDirections <= 2）をコンテナに収集
        for (int y = 1; y < MAX_INDEX; ++y)
        {
            for (int x = 1; x < MAX_INDEX; ++x)
            {
                if (mapComp.grid[y][x].type != CellType::Path) continue;

                int openDirections = 0;
                // 1マス先の隣接セルをチェック (前回修正されたロジック)
                for (int i = 0; i < 4; ++i)
                {
                    int nx = x + directions_1step[i][0];
                    int ny = y + directions_1step[i][1];

                    if (nx > 0 && nx < MAX_INDEX && ny > 0 && ny < MAX_INDEX)
                    {
                        CellType type = mapComp.grid[ny][nx].type;
                        if (type == CellType::Path || type == CellType::Room || type == CellType::Start || type == CellType::Goal)
                        {
                            openDirections++;
                        }
                    }
                }

                if (openDirections <= 1)
                {
                    deadEndCandidates.push_back({ x, y });
                }
            }
        }

        // 処理が行われる前に総数を報告することで、検出数を確認する
        printf("DeadEnd Removal Pass: Detected %zu dead ends.\n", deadEndCandidates.size());

        // 2. 検出された全てのデッドエンドを順番に処理する
        for (const auto& targetDeadEnd : deadEndCandidates)
        {
            if (mapComp.grid[targetDeadEnd.y][targetDeadEnd.x].type != CellType::Path) continue;

            // ----------------------------------------------------------------------------------
            // 3. 行き止まりセルから、最も近い Path/Room への接続経路を BFS で検索
            // ----------------------------------------------------------------------------------

            std::vector<XMINT2> bfs_queue;
            std::map<XMINT2, XMINT2> parentMap;
            std::set<XMINT2> visitedSet;

            // BFSの初期化ロジック (根元の通路を除外) はそのまま利用
            for (int i = 0; i < 4; ++i)
            {
                int nx = targetDeadEnd.x + directions_1step[i][0];
                int ny = targetDeadEnd.y + directions_1step[i][1];

                if (nx > 0 && nx < MAX_INDEX && ny > 0 && ny < MAX_INDEX)
                {
                    XMINT2 neighbor = { nx, ny };
                    CellType neighborType = mapComp.grid[ny][nx].type;

                    // 【修正】もし隣接セルがPath/Roomの場合、それがデッドエンドの真の根元である
                    if (neighborType == CellType::Path || neighborType == CellType::Room || neighborType == CellType::Start)
                    {
                        // 既に開通している通路方向（根元）を探索対象から除外
                        visitedSet.insert(neighbor);
                    }
                    else if (neighborType == CellType::Wall)
                    {
                        // 壁の場合は、初期キューに追加し、壁破壊経路から探索を開始
                        if (visitedSet.find(neighbor) == visitedSet.end())
                        {
                            visitedSet.insert(neighbor);
                            parentMap[neighbor] = targetDeadEnd;
                            bfs_queue.push_back(neighbor);
                        }
                    }
                }
            }
            visitedSet.insert(targetDeadEnd);

            XMINT2 foundTarget = { -1, -1 };
            size_t head = 0;
            while (head < bfs_queue.size())
            {
                XMINT2 current = bfs_queue[head++];

                // 接続ターゲットの発見チェック (自身は除く)
                if (current.x != targetDeadEnd.x || current.y != targetDeadEnd.y)
                {
                    CellType type = mapComp.grid[current.y][current.x].type;
                    if (type == CellType::Path || type == CellType::Room)
                    {
                        foundTarget = current;
                        break; // 最短の接続先を発見
                    }
                }

                // 4方向の隣接セル（1マス先）を探索 (Wallも通過可能)
                for (int i = 0; i < 4; ++i)
                {
                    int nx = current.x + directions_1step[i][0]; // 1マスステップ
                    int ny = current.y + directions_1step[i][1]; // 1マスステップ

                    if (nx > 0 && nx < MAX_INDEX && ny > 0 && ny < MAX_INDEX)
                    {
                        XMINT2 neighbor = { nx, ny };
                        if (visitedSet.find(neighbor) == visitedSet.end())
                        {
                            CellType type = mapComp.grid[ny][nx].type;

                            if (type != CellType::Unvisited)
                            {
                                visitedSet.insert(neighbor);
                                parentMap[neighbor] = current;
                                bfs_queue.push_back(neighbor);
                            }
                        }
                    }
                }
            } // end while (BFS)


            // ----------------------------------------------------------------------------------
            // 4. 接続経路の逆追跡と、破壊する壁の特定
            // ----------------------------------------------------------------------------------
            if (foundTarget.x != -1)
            {
                XMINT2 current = foundTarget;
                XMINT2 wallToDestroy = { -1, -1 };

                // ★ 修正：デッドエンドからターゲットまでの最短経路を逆追跡し、最初に見つけた壁を破壊する ★

                // ターゲットからデッドエンドまでの最短経路を逆追跡し、経路上の壁を探す
                while (parentMap.count(current))
                {
                    XMINT2 parent = parentMap[current];

                    // currentが壁セルで、かつ親がデッドエンドではない場合（デッドエンド側から見て最初の壁を探す）
                    if (mapComp.grid[current.y][current.x].type == CellType::Wall)
                    {
                        wallToDestroy = current; // ターゲットに最も近い壁を一旦保持
                    }

                    // 親がデッドエンドである、または current がデッドエンドであれば終了
                    if (parent.x == targetDeadEnd.x && parent.y == targetDeadEnd.y)
                    {
                        // デッドエンドに隣接する最後のセルが壁であればそれを破壊
                        if (mapComp.grid[current.y][current.x].type == CellType::Wall)
                        {
                            wallToDestroy = current;
                        }
                        break;
                    }

                    current = parent;
                }

                // 5. 壁の破壊
                if (wallToDestroy.x != -1)
                {
                    mapComp.grid[wallToDestroy.y][wallToDestroy.x].type = CellType::Path;
                    deadEndsWereProcessed = true;
                    totalDeadEndsResolved++;
                    printf("Resolved dead end at (%d, %d) by destroying wall at (%d, %d). Total resolved: %d\n",
                        targetDeadEnd.x, targetDeadEnd.y, wallToDestroy.x, wallToDestroy.y, totalDeadEndsResolved);
                }
            }
        }

    } while (deadEndConnected);

    // --------------------------------------------------------------------------------
    // 【ステップ4-1/3: スタート/ゴール位置の決定とランダム化】
    // --------------------------------------------------------------------------------

    std::vector<XMINT2> spawnablePositions;     // 全ての有効な通路セル (内側 1 to MAX-2)
    std::vector<XMINT2> perimeterSpawnPositions;// 外周より一つ内側の境界セル (スポーン優先)

    // 最外周の壁より一つ内側の領域 (1 to MAX_INDEX - 1) の Pathセルを収集する
    for (int y = 1; y < MAX_INDEX; ++y)
    {
        for (int x = 1; x < MAX_INDEX; ++x)
        {
            // WallでもUnvisitedでもない (Path/Room) セルを候補とする
            if (mapComp.grid[y][x].type != CellType::Wall && mapComp.grid[y][x].type != CellType::Unvisited) {

                // 全ての有効なセルを候補に追加
                spawnablePositions.push_back({ x, y });

                // ★ プレイヤーの要件: 外周より一つ内側のセル (x=1, MAX-1, y=1, MAX-1) を抽出 ★
                if (x == 1 || x == MAX_INDEX - 1 || y == 1 || y == MAX_INDEX - 1) {
                    perimeterSpawnPositions.push_back({ x, y });
                }
            }
        }
    }

    // プレイヤーとゴールの位置をランダムに決定
    if (spawnablePositions.size() >= 2)
    {
        // 1. スタート位置の決定
        if (!perimeterSpawnPositions.empty()) {
            // 要件に従い、内側境界からランダムに選択
            std::shuffle(perimeterSpawnPositions.begin(), perimeterSpawnPositions.end(), s_generator);
            mapComp.startPos = perimeterSpawnPositions.back();
            // 選択されたセルを全体リストから削除し、ゴールと重複しないようにする
            spawnablePositions.erase(std::remove(spawnablePositions.begin(), spawnablePositions.end(), mapComp.startPos), spawnablePositions.end());
        }
        else {
            // 内側境界パスがない場合のフォールバック: 全体からランダムに選択
            std::shuffle(spawnablePositions.begin(), spawnablePositions.end(), s_generator);
            mapComp.startPos = spawnablePositions.back();
            spawnablePositions.pop_back();
        }

        // 2. ゴール位置の決定
        // 残りのセル全体からランダムに選択
        if (!spawnablePositions.empty()) {
            std::shuffle(spawnablePositions.begin(), spawnablePositions.end(), s_generator);
            mapComp.goalPos = spawnablePositions.back();
            spawnablePositions.pop_back();
        }
        else {
            // ゴールを配置する場所がない場合は、スタート位置をPathに戻すなどのエラー処理が必要だが、ここではログのみ
        }

    }
    else {
        // パスセルが足りない場合のデバッグ処理
        mapComp.startPos = { 1, 1 };
        mapComp.goalPos = { MAX_INDEX - 1, MAX_INDEX - 1 };
    }

    // 3. セルタイプを更新
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
    trackerComp.totalItems = itemsToPlace;

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
                // 間の壁セルを通路にする
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
    ItemTrackerComponent& trackerComp = m_coordinator->GetComponent<ItemTrackerComponent>(mapEntity);

    // 1. 迷路データの生成
    // ★ 修正点：50x50グリッドの奇数座標から開始 (1, 1)
    mapComp.startPos = { 1, 1 };
    // ゴール位置を決定 (右下に近い奇数座標)
    mapComp.goalPos = { MAP_GRID_SIZE - 2, MAP_GRID_SIZE - 2 }; // (48, 48)

    MazeGenerator::Generate(mapComp, trackerComp);

    // 2. 3D空間へのEntity配置
    SpawnMapEntities(mapComp);
}

/**
 * @brief グリッド座標をワールド座標に変換するヘルパー関数
 */
XMFLOAT3 MapGenerationSystem::GetWorldPosition(int x, int y)
{
    // MAP_GRID_SIZE = 50, TILE_SIZE = 1.0f
    // マップの中心をワールド原点(0, 0, 0)とするためのオフセット計算 (25.0f)
    constexpr float MAP_CENTER_OFFSET = (MAP_GRID_SIZE / 2.0f) * TILE_SIZE;

    // ★ 修正点: ズレ補正用の定数オフセット ★
    // 左に 0.5m ズレているのを修正するため、+X方向に 0.5m オフセット
    constexpr float X_ADJUSTMENT = 0.5f * TILE_SIZE;
    // 下に 1.0m ズレているのを修正するため、+Z方向に 1.0m オフセット
    constexpr float Z_ADJUSTMENT = 1.0f * TILE_SIZE;

    XMFLOAT3 pos;
    // X座標: (グリッドX * TILE_SIZE - MAP_CENTER_OFFSET) + X_ADJUSTMENT
    pos.x = (float)x * TILE_SIZE - MAP_CENTER_OFFSET + X_ADJUSTMENT;
    // Y座標: 地面なので0
    pos.y = 0.0f;
    // Z座標: (グリッドY * TILE_SIZE - MAP_CENTER_OFFSET) + Z_ADJUSTMENT
    pos.z = (float)y * TILE_SIZE - MAP_CENTER_OFFSET + Z_ADJUSTMENT;

    return pos;
}


void MapGenerationSystem::DrawDebugLines()
{
    using namespace DirectX;
    // マップの中心をワールド原点(0, 0, 0)とするためのオフセット計算
    // NOTE: これらの定数は MapComponent.h または MapGenerationSystem.cpp のグローバル定数から取得します。

    // MapComponentを持つエンティティを取得
    ECS::EntityID mapEntity = FindFirstEntityWithComponent<MapComponent>(m_coordinator);
    if (mapEntity == INVALID_ENTITY_ID) return;

    MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);

    XMFLOAT4 wallColor = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f); // 赤色

    // グリッド全体を走査し、壁と通路の境界線を描画
    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            // 現在のセルタイプ
            CellType currentType = mapComp.grid[y][x].type; //

            // 壁または未訪問のセルは描画対象外（通路の境界のみを描画したい）
            // ※ ここでは、通路(Path)または特殊セル(Start/Goal/Item/Guard)を「床」とみなす
            if (currentType == CellType::Wall || currentType == CellType::Unvisited) //
            {
                continue;
            }

            // セルの角のワールド座標を取得（床の厚さを無視してY=0として描画する）
            // GetWorldPosition(x, y) はセグメントの左上隅を返す仕様（z軸方向が反転している可能性を考慮）
            XMFLOAT3 cellCorner = GetWorldPosition(x, y);
            // GetWorldPositionはセルの中心ではなく、グリッドセルの角（または左下隅）の座標を返す想定

            // 描画の高さを床の上(Y=0.1f)に設定して、Zファイティングを避ける
            constexpr float LINE_HEIGHT = WALL_HEIGHT + 1.0f;

            // 1. 北側の境界線 (y-1)
            if (y > 0)
            {
                CellType neighborType = mapComp.grid[y - 1][x].type; //
                // 隣が壁で、自身が通路の場合、境界線を描画
                if (neighborType == CellType::Wall) //
                {
                    // 始点: (x, y) の左上角
                    XMFLOAT3 start = GetWorldPosition(x, y); // GetWorldPosition(x, y) は壁Entityの左下隅を返す仕様
                    start.y = LINE_HEIGHT;

                    // 終点: (x, y) の右上角
                    XMFLOAT3 end = start;
                    end.x += TILE_SIZE;

                    // Geometory::AddLineは外部システムのため、ここでインクルードの確認が必要だが、ここでは仮に存在するものとする
                    Geometory::AddLine(start, end, wallColor);
                }
            }

            // 2. 東側の境界線 (x+1)
            if (x < MAP_GRID_SIZE - 1)
            {
                CellType neighborType = mapComp.grid[y][x + 1].type; //
                // 隣が壁で、自身が通路の場合、境界線を描画
                if (neighborType == CellType::Wall) //
                {
                    // 始点: (x, y) の右上角
                    XMFLOAT3 start = GetWorldPosition(x, y);
                    start.x += TILE_SIZE;
                    start.y = LINE_HEIGHT;

                    // 終点: (x, y) の右下角
                    XMFLOAT3 end = start;
                    end.z += TILE_SIZE;

                    Geometory::AddLine(start, end, wallColor);
                }
            }

            // 3. 南側の境界線 (y+1)
            if (y < MAP_GRID_SIZE - 1)
            {
                CellType neighborType = mapComp.grid[y + 1][x].type; //
                // 隣が壁で、自身が通路の場合、境界線を描画
                if (neighborType == CellType::Wall) //
                {
                    // 始点: (x, y) の左下角
                    XMFLOAT3 start = GetWorldPosition(x, y);
                    start.z += TILE_SIZE;
                    start.y = LINE_HEIGHT;

                    // 終点: (x, y) の右下角
                    XMFLOAT3 end = start;
                    end.x += TILE_SIZE;

                    Geometory::AddLine(start, end, wallColor);
                }
            }

            // 4. 西側の境界線 (x-1)
            if (x > 0)
            {
                CellType neighborType = mapComp.grid[y][x - 1].type; //
                // 隣が壁で、自身が通路の場合、境界線を描画
                if (neighborType == CellType::Wall) //
                {
                    // 始点: (x, y) の左上角
                    XMFLOAT3 start = GetWorldPosition(x, y);
                    start.y = LINE_HEIGHT;

                    // 終点: (x, y) の左下角
                    XMFLOAT3 end = start;
                    end.z += TILE_SIZE;

                    Geometory::AddLine(start, end, wallColor);
                }
            }
        }
    }
}

/**
 * @brief 生成されたマップデータに基づき、3D空間に壁、床、オブジェクトを配置する
 */
void MapGenerationSystem::SpawnMapEntities(MapComponent& mapComp)
{
    // --------------------------------------------------------------------------------
    // 【最適化】Greedy Meshingのための処理済みフラグ用グリッド
    // --------------------------------------------------------------------------------
    bool processed[MAP_GRID_SIZE][MAP_GRID_SIZE] = { false };

    // --------------------------------------------------------------------
    // 1. 床Entityの生成（Path, Start, Goal, Item, Guardセルを全てPathとして結合）
    // --------------------------------------------------------------------
    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            if (mapComp.grid[y][x].type == CellType::Wall ||
                mapComp.grid[y][x].type == CellType::Unvisited ||
                processed[y][x])
            {
                continue;
            }

            // X軸方向への結合を試みる (Path/特殊セルは全て床として結合)
            int endX = x;
            while (endX + 1 < MAP_GRID_SIZE &&
                !processed[y][endX + 1] &&
                mapComp.grid[y][endX + 1].type != CellType::Wall &&
                mapComp.grid[y][endX + 1].type != CellType::Unvisited)
            {
                endX++;
            }

            // 結合 Entityの生成
            int segmentLength = endX - x + 1;
            float worldLength = (float)segmentLength * TILE_SIZE;

            // ★ 修正1: グリッドの左下隅の座標を取得 (BUG-06対応)
            XMFLOAT3 segmentStartPos = GetWorldPosition(x, y);

            // ★ 修正2: Entityの中心座標を計算 (BUG-06対応)
            XMFLOAT3 segmentCenterPos = segmentStartPos;
            segmentCenterPos.x += worldLength / 2.0f; // Entityの長さの半分をオフセット

            segmentCenterPos.z += TILE_SIZE / 2.0f;

            // 修正3: Y座標のオフセット (BUG-05対応)
            segmentCenterPos.y = -0.01f; // Zファイティング回避のためわずかに下げる

            // 床 Entityの生成
            EntityFactory::CreateGround(
                m_coordinator,
                segmentCenterPos,
                XMFLOAT3(worldLength, TILE_SIZE, TILE_SIZE) // X: 長さ, Y: 1m(厚さ), Z: 1m幅
            );

            // 処理済みフラグの更新
            for (int i = x; i <= endX; ++i)
            {
                processed[y][i] = true;
            }
            x = endX;
        }
    }


    // --------------------------------------------------------------------
    // 2. 壁Entityの生成（Wallセルのみを結合）
    // --------------------------------------------------------------------
    // 処理済みフラグをリセットして壁の結合に再利用
    for (int y = 0; y < MAP_GRID_SIZE; ++y) {
        for (int x = 0; x < MAP_GRID_SIZE; ++x) {
            processed[y][x] = false;
        }
    }

    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            if (mapComp.grid[y][x].type != CellType::Wall || processed[y][x]) continue;

            // X軸方向への結合を試みる (Wallセルのみ)
            int endX = x;
            while (endX + 1 < MAP_GRID_SIZE &&
                !processed[y][endX + 1] &&
                mapComp.grid[y][endX + 1].type == CellType::Wall)
            {
                endX++;
            }

            // 壁 Entityの生成
            int segmentLength = endX - x + 1;
            float worldLength = (float)segmentLength * TILE_SIZE;

            // ★ 修正1: グリッドの左下隅の座標を取得 (BUG-06対応)
            XMFLOAT3 segmentStartPos = GetWorldPosition(x, y);

            // ★ 修正2: Entityの中心座標を計算 (BUG-06対応)
            XMFLOAT3 segmentCenterPos = segmentStartPos;
            segmentCenterPos.x += worldLength / 2.0f; // Entityの長さの半分をオフセット

            segmentCenterPos.z += TILE_SIZE / 2.0f;

            // 修正3: Y座標のオフセット (BUG-05対応)
            segmentCenterPos.y = WALL_HEIGHT / 2.0f; // 2.5f (壁の中心を地面より上に持ち上げる)

            // 壁 Entityの生成
            EntityFactory::CreateWall(
                m_coordinator,
                segmentCenterPos,
                XMFLOAT3(worldLength, WALL_HEIGHT, TILE_SIZE), // X: 長さ, Y: 高さ, Z: 厚さ (1m)
                0.0f // 回転なし
            );

            // 処理済みフラグの更新
            for (int i = x; i <= endX; ++i)
            {
                processed[y][i] = true;
            }
            x = endX;
        }
    }

    // --------------------------------------------------------------------
    // 3. 特殊 Entityの配置（単独セル）
    // --------------------------------------------------------------------
    for (int y = 0; y < MAP_GRID_SIZE; ++y)
    {
        for (int x = 0; x < MAP_GRID_SIZE; ++x)
        {
            Cell& cell = mapComp.grid[y][x];

            switch (cell.type)
            {
            case CellType::Start:
            case CellType::Goal:
            case CellType::Item:
            case CellType::Guard:
            {
                // 単独セルの中心座標を取得
                XMFLOAT3 cellCenter = GetWorldPosition(x, y);
                // GetWorldPositionは角座標を返すので、単独セルの中心オフセットを追加
                cellCenter.x += TILE_SIZE / 2.0f;
                cellCenter.z += TILE_SIZE / 2.0f;

                // 特殊オブジェクトのY座標を床の表面に合わせる (0.5f = プレイヤー/アイテムの中心)
                cellCenter.y = TILE_SIZE / 2.0f;

                if (cell.type == CellType::Start) {
                    EntityFactory::CreatePlayer(m_coordinator, cellCenter);
                    EntityFactory::CreateGoal(m_coordinator, cellCenter);
                    EntityFactory::CreateGuard(m_coordinator, cellCenter);
                }
                else if (cell.type == CellType::Goal) {
                    //EntityFactory::CreateGoal(m_coordinator, cellCenter);
                }
                else if (cell.type == CellType::Item) {
                    EntityFactory::CreateCollectable(m_coordinator, cellCenter);
                }
                else if (cell.type == CellType::Guard) {
                    //EntityFactory::CreateGuard(m_coordinator, cellCenter);
                }
            }
            break;
            default:
                break;
            }
        }
    }
}