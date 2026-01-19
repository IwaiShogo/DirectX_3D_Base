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
void MazeGenerator::Generate(MapComponent& mapComp, ItemTrackerComponent& trackerComp, const MapStageConfig& config)
{
    // configから動的な定数を取得
    const int GRID_SIZE_X = config.gridSizeX;
    const int GRID_SIZE_Y = config.gridSizeY;
    const int MAX_INDEX_X = GRID_SIZE_X - 1;
    const int MAX_INDEX_Y = GRID_SIZE_Y - 1;

    // マップ生成の品質保証のための定数
    constexpr int MAX_GENERATION_ATTEMPTS = 10;

    // 迷路として許容できる最小通路セル割合 (50x50グリッドの場合、25%で625セル)
    const float MIN_PATH_PERCENTAGE = config.minPathPercentage;
    // 最小通路セル数を動的に計算
    const int MIN_PATH_COUNT = (int)(GRID_SIZE_X * GRID_SIZE_Y * MIN_PATH_PERCENTAGE);

    int directions[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };

    // 成功するまでマップ生成プロセスを繰り返す
    for (int attempt = 0; attempt < MAX_GENERATION_ATTEMPTS; ++attempt)
    {
        // 可変サイズに対応するため、gridをリサイズ
        mapComp.grid.assign(GRID_SIZE_Y, std::vector<Cell>(GRID_SIZE_X));

        // 全てのセルを未訪問(Unvisited)にリセット
        for (int y = 0; y < GRID_SIZE_Y; ++y)
        {
            for (int x = 0; x < GRID_SIZE_X; ++x)
            {
                mapComp.grid[y][x].visited = false;
                mapComp.grid[y][x].type = CellType::Wall; // 初期状態はすべて壁
            }
        }

        // --------------------------------------------------------------------------------
        // 【ステップ5-2: 部屋配置ロジックの追加】
        // --------------------------------------------------------------------------------

        // 部屋パラメータをconfigから取得
        const int MIN_ROOM_SIZE = config.minRoomSize;
        const int MAX_ROOM_SIZE = config.maxRoomSize;
        const int MAX_ROOM_COUNT = config.maxRoomCount;

        // 部屋の配置を試行する
        for (int i = 0; i < MAX_ROOM_COUNT; ++i) // 失敗を見越して多めに試行
        {
            // 部屋のサイズをランダムに決定 (奇数に限定して迷路生成との接続を容易にする)
            std::uniform_int_distribution<int> sizeDist(MIN_ROOM_SIZE / 2, MAX_ROOM_SIZE / 2);
            int halfSize = sizeDist(s_generator);
            int roomWidth = halfSize * 2 + 1;
            int roomHeight = halfSize * 2 + 1;

            // 部屋の左上角の座標をランダムに決定
            // 配置可能範囲を (1, 1) から (GRID_SIZE - MAX_ROOM_SIZE - 2) までに限定
            std::uniform_int_distribution<int> posXDist(1, GRID_SIZE_X - roomWidth - 1);
            std::uniform_int_distribution<int> posYDist(1, GRID_SIZE_Y - roomHeight - 1);

            int startX = posXDist(s_generator);
            int startY = posYDist(s_generator);

            // 部屋のサイズを調整 (境界からはみ出さないように)
            if (startX + roomWidth >= MAX_INDEX_X) roomWidth = MAX_INDEX_X - startX - 1;
            if (startY + roomHeight >= MAX_INDEX_Y) roomHeight = MAX_INDEX_Y - startY - 1;

            // 部屋のサイズが奇数で、かつ最小サイズ以上であることを保証
            roomWidth = (roomWidth % 2 == 0) ? roomWidth - 1 : roomWidth;
            roomHeight = (roomHeight % 2 == 0) ? roomHeight - 1 : roomHeight;

            if (roomWidth < MIN_ROOM_SIZE || roomHeight < MIN_ROOM_SIZE) continue;

            // 部屋の領域をRoomとしてマークし、visited=true (迷路生成の対象外)
            for (int y = startY; y < startY + roomHeight; ++y)
            {
                for (int x = startX; x < startX + roomWidth; ++x)
                {
                    // 境界からはみ出さないか最終チェック
                    if (x >= 0 && x < GRID_SIZE_X && y >= 0 && y < GRID_SIZE_Y)
                    {
                        mapComp.grid[y][x].type = CellType::Room;
                        mapComp.grid[y][x].visited = true; // 迷路生成アルゴリズムの対象外
                    }
                }
            }
        }

        // --------------------------------------------------------------------------------
        // 【ステップ5-3: 迷路生成の実行】
        // --------------------------------------------------------------------------------
        // スタート位置をグリッドの中心に近い奇数座標の範囲で決定
        std::uniform_int_distribution<int> startXDist(1, GRID_SIZE_X - 2);
        std::uniform_int_distribution<int> startYDist(1, GRID_SIZE_Y - 2);

        int startX = startXDist(s_generator);
        int startY = startYDist(s_generator);

        startX = 1;
        startY = 1;

        // 開始セルをPathとしてマークし、visitedをリセット
        mapComp.grid[startY][startX].type = CellType::Path;
        mapComp.grid[startY][startX].visited = false;

        RecursiveBacktracker(mapComp, config, startX, startY);

        // --------------------------------------------------------------------------------
        // 5. 迷路生成の後に、最外周の壁セルをvisited=trueに固定する
        // --------------------------------------------------------------------------------
        // 迷路生成が完了した後で、外周を確実に固定します。
        for (int i = 0; i < GRID_SIZE_X; ++i)
        {
            // 1. 上下の境界 (y=0 と y=MAX_INDEX_Y)
            mapComp.grid[0][i].type = CellType::Wall;
            mapComp.grid[0][i].visited = true;

            mapComp.grid[MAX_INDEX_Y][i].type = CellType::Wall;
            mapComp.grid[MAX_INDEX_Y][i].visited = true;
        }
        for (int i = 0; i < GRID_SIZE_Y; ++i)
        {
            // 2. 左右の境界 (x=0 と x=MAX_INDEX_X)
            mapComp.grid[i][0].type = CellType::Wall;
            mapComp.grid[i][0].visited = true;

            mapComp.grid[i][MAX_INDEX_X].type = CellType::Wall;
            mapComp.grid[i][MAX_INDEX_X].visited = true;
        }

        // --------------------------------------------------------------------------------
        // 【確実な接続保証ロジック（Flood Fillによる連結成分の結合）】
        // --------------------------------------------------------------------------------
        bool mapWasConnected = false;

        // 接続プロセスを最大でGRID_SIZE分繰り返す
        for (int connection_iter = 0; connection_iter < GRID_SIZE_X + GRID_SIZE_Y; ++connection_iter)
        {
            bool wallDestroyed = false;

            // a) Flood Fillのための visitedフラグをリセット
            for (int y = 0; y < GRID_SIZE_Y; ++y)
            {
                for (int x = 0; x < GRID_SIZE_X; ++x)
                {
                    if (mapComp.grid[y][x].type != CellType::Wall)
                    {
                        mapComp.grid[y][x].visited = false;
                    }
                }
            }

            // スタート位置を一時的に設定（スタート/ゴール配置前の暫定処理）
            XMINT2 tempStartPos = { startX, startY };

            // b) Flood Fill (BFS) を実行: スタートから到達可能なセルをマーク
            std::vector<XMINT2> bfs_queue;
            bfs_queue.push_back(tempStartPos);
            mapComp.grid[tempStartPos.y][tempStartPos.x].visited = true;

            size_t head = 0;
            while (head < bfs_queue.size())
            {
                XMINT2 current = bfs_queue[head++];

                for (int i = 0; i < 4; ++i)
                {
                    int nx = current.x + directions[i][0];
                    int ny = current.y + directions[i][1];

                    // 境界チェックにMAX_INDEX_X, MAX_INDEX_Yを使用
                    if (nx > 0 && nx < MAX_INDEX_X && ny > 0 && ny < MAX_INDEX_Y &&
                        mapComp.grid[ny][nx].type != CellType::Wall &&
                        !mapComp.grid[ny][nx].visited)
                    {
                        mapComp.grid[ny][nx].visited = true;
                        bfs_queue.push_back({ nx, ny });
                    }
                }
            }

            // c) 接続の試行: 未到達エリアと到達エリアを繋ぐ壁を探して破壊
            // 境界チェックにMAX_INDEX_X, MAX_INDEX_Yを使用
            for (int y = 1; y < MAX_INDEX_Y; ++y)
            {
                for (int x = 1; x < MAX_INDEX_X; ++x)
                {
                    if (mapComp.grid[y][x].type == CellType::Wall)
                    {
                        bool bordersReachable = false;
                        bool bordersUnreachable = false;

                        for (int i = 0; i < 4; ++i)
                        {
                            int nx = x + directions[i][0];
                            int ny = y + directions[i][1];

                            // 境界チェックにMAX_INDEX_X, MAX_INDEX_Yを使用
                            if (nx > 0 && nx < MAX_INDEX_X && ny > 0 && ny < MAX_INDEX_Y &&
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
        for (int y = 0; y < GRID_SIZE_Y; ++y)
        {
            for (int x = 0; x < GRID_SIZE_X; ++x)
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
    // 4方向 (1マス先)
    int directions_1step[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
    bool deadEndConnected = false;

    // デバッグ用カウンタの宣言
    int totalDeadEndsResolved = 0; // 解消された総数のみを追跡する

    // 変更がなくなるまでループし、デッドエンドのリストを更新する
    bool deadEndsWereProcessed;
    do
    {
        deadEndsWereProcessed = false;
        // 検出された全てのデッドエンドを保持するコンテナ
        std::vector<XMINT2> deadEndCandidates;

        // 1. 全てのPathセルを走査し、真の「行き止まり」セル（openDirections <= 2）をコンテナに収集
        for (int y = 1; y < MAX_INDEX_Y; ++y)
        {
            for (int x = 1; x < MAX_INDEX_X; ++x)
            {
                // Roomも開通していると見なすため、Pathのみを対象とする
                if (mapComp.grid[y][x].type != CellType::Path) continue;

                int openDirections = 0;
                // 1マス先の隣接セルをチェック (前回修正されたロジック)
                for (int i = 0; i < 4; ++i)
                {
                    int nx = x + directions_1step[i][0];
                    int ny = y + directions_1step[i][1];

                    if (nx > 0 && nx < MAX_INDEX_X && ny > 0 && ny < MAX_INDEX_Y)
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

                if (nx > 0 && nx < MAX_INDEX_X && ny > 0 && ny < MAX_INDEX_Y)
                {
                    XMINT2 neighbor = { nx, ny };
                    CellType neighborType = mapComp.grid[ny][nx].type;

                    // もし隣接セルがPath/Roomの場合、それがデッドエンドの真の根元である
                    if (neighborType == CellType::Path || neighborType == CellType::Room || neighborType == CellType::Start || neighborType == CellType::Goal)
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

                    if (nx > 0 && nx < MAX_INDEX_X && ny > 0 && ny < MAX_INDEX_Y)
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
    for (int y = 1; y < MAX_INDEX_Y; ++y)
    {
        for (int x = 1; x < MAX_INDEX_X; ++x)
        {
            // WallでもUnvisitedでもない (Path/Room) セルを候補とする
            if (mapComp.grid[y][x].type != CellType::Wall && mapComp.grid[y][x].type != CellType::Unvisited) {

                // 全ての有効なセルを候補に追加
                spawnablePositions.push_back({ x, y });

                // プレイヤーの要件: 外周より一つ内側のセル (x=1, MAX-1, y=1, MAX-1) を抽出
                if (x == 1 || x == MAX_INDEX_X - 1 || y == 1 || y == MAX_INDEX_Y - 1) {
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
        mapComp.goalPos = { MAX_INDEX_X - 1, MAX_INDEX_Y - 1 };
    }

    // 3. セルタイプを更新
    mapComp.grid[mapComp.startPos.y][mapComp.startPos.x].type = CellType::Start;
    mapComp.grid[mapComp.goalPos.y][mapComp.goalPos.x].type = CellType::Goal;

    // --------------------------------------------------------------------------------
    // 【ステップ2-3: アイテムと警備員の配置ロジック】
    // --------------------------------------------------------------------------------

    // 配置可能なパス座標のリストを収集
    std::vector<XMINT2> availablePathPositions;
    for (int y = 0; y < GRID_SIZE_Y; ++y)
    {
        for (int x = 0; x < GRID_SIZE_X; ++x)
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

    // 1. アイテム (config.itemCount) の配置
    const int ITEMS_TO_PLACE = config.items.size();
    trackerComp.totalItems = ITEMS_TO_PLACE;

    // 配置可能な数が足りない場合は、リストのサイズを上限とする
    int itemsToPlace = std::min((int)availablePathPositions.size(), ITEMS_TO_PLACE);

    for (int i = 0; i < itemsToPlace; ++i)
    {
        XMINT2 pos = availablePathPositions.back();
        availablePathPositions.pop_back(); // 使用した座標はリストから削除

        mapComp.grid[pos.y][pos.x].type = CellType::Item;
        mapComp.itemPositions.push_back(pos); // ItemComponentで管理するために位置情報を保存
    }

    // 2. 警備員（敵）の配置 (config.guardCount)
    const int GUARDS_TO_PLACE = config.guardCount;

    int guardsPlaced = 0;
    while (guardsPlaced < GUARDS_TO_PLACE && !availablePathPositions.empty())
    {
        XMINT2 pos = availablePathPositions.back();
        availablePathPositions.pop_back();

        mapComp.grid[pos.y][pos.x].type = CellType::Guard;
        guardsPlaced++;
    }

    // 3. 汎用ギミック配置ロジック
    for (const auto& gimmick : config.gimmicks)
    {
        int placedCount = 0;

        // 指定された個数分配置を試みる
        while (placedCount < gimmick.count && !availablePathPositions.empty())
        {
            // --- [Case: Taser] ---
            if (gimmick.type == "Taser")
            {
                XMINT2 pos = availablePathPositions.back();
                availablePathPositions.pop_back();

                mapComp.grid[pos.y][pos.x].type = CellType::Taser;
                placedCount++;
            }
            // --- [Case: Teleporter] ---
            else if (gimmick.type == "Teleporter")
            {
                // テレポーターはペア(2箇所)が必要なので、空きが2つ以上あるか確認
                if (availablePathPositions.size() < 2)
                {
                    printf("[Warning] Not enough space for Teleporter pair.\n");
                    break;
                }

                // 入口と出口の2マスをリストから取得
                XMINT2 posA = availablePathPositions.back();
                availablePathPositions.pop_back();
                XMINT2 posB = availablePathPositions.back();
                availablePathPositions.pop_back();

                // グリッドにテレポート属性を設定
                mapComp.grid[posA.y][posA.x].type = CellType::Teleporter;
                mapComp.grid[posB.y][posB.x].type = CellType::Teleporter;

                mapComp.teleportPairs.push_back({ posA, posB });
                placedCount++;
            }
            // --- [Case: その他 (将来的な拡張)] ---
            else
            {
                // 未定義のタイプが指定された場合の警告
                printf("[Warning] Unknown gimmick type in config: %s\n", gimmick.type.c_str());
                break; // 無限ループ防止のためbreak
            }
        }
    }
}

/**
 * @brief 再帰的バックトラッカーの主要ロジック
 * @param mapComp - マップデータ
 * @param x - 現在のセルのX座標
 * @param y - 現在のセルのY座標
 */
void MazeGenerator::RecursiveBacktracker(MapComponent& mapComp, const MapStageConfig& config, int x, int y)
{
    const int GRID_SIZE_X = config.gridSizeX;
    const int GRID_SIZE_Y = config.gridSizeY;

    // 現在のセルを訪問済みとしてマーク
    mapComp.grid[y][x].visited = true;
    if (mapComp.grid[y][x].type != CellType::Room)
    {
        mapComp.grid[y][x].type = CellType::Path; // 通路を掘る
    }

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
        if (nx >= 0 && nx < GRID_SIZE_X && ny >= 0 && ny < GRID_SIZE_Y)
        {
            if (!mapComp.grid[ny][nx].visited)
            {
                // 間の壁セルを通路にする
                mapComp.grid[wallY][wallX].type = CellType::Path; // 間のセルを通路として掘る

                // 次のセルへ進む
                RecursiveBacktracker(mapComp, config, nx, ny); // <--- configを渡す
            }
        }
    }
}

// ===================================================================
// MapGenerationSystem 実装
// ===================================================================

/**
 * [void - CreateMap]
 * @brief	マップ生成システムを初期化し、迷路を生成する。
 * 
 * @param	[in] stageID 読み込むステージ設定のID
 */
void MapGenerationSystem::CreateMap(const std::string& stageID)
{
    // 1. 設定の読み込み
    MapStageConfig config = MapConfigLoader::Load(stageID);

    // MapComponentを持つエンティティは一つだけとする
    EntityID mapEntity = FindFirstEntityWithComponent<MapComponent>(m_coordinator);

    // 見つからなかった場合は、専用のエンティティを生成してアタッチ
    if (mapEntity == INVALID_ENTITY_ID) return;

    auto& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);
    auto& trackerComp = m_coordinator->GetComponent<ItemTrackerComponent>(mapEntity);
    auto& stateComp = m_coordinator->GetComponent<GameStateComponent>(mapEntity);

    stateComp.timeLimitStar = config.timeLimitStar;
    trackerComp.targetItemIDs.clear();
    mapComp.teleportPairs.clear();
    mapComp.itemPositions.clear();

    // 2. 迷路データの生成
    // MapComponent内に設定情報がコピーされる
    mapComp.gridSizeX = config.gridSizeX;
    mapComp.gridSizeY = config.gridSizeY;
    mapComp.tileSize = config.tileSize;
    mapComp.wallHeight = config.wallHeight;
    mapComp.startPos = { 1, 1 };
    mapComp.goalPos = { config.gridSizeX - 2, config.gridSizeY - 2 };
    trackerComp.useOrderedCollection = config.useOrderedCollection;
    trackerComp.currentTargetOrder = 1;
    trackerComp.collectedItems = 0;
    trackerComp.totalItems = 0;

    MazeGenerator::Generate(mapComp, trackerComp, config);

    m_itemSpawnIndex = 0;

    // 3. 3D空間へのEntity配置
    SpawnMapEntities(mapComp, config);
}

/**
 * @brief グリッド座標をワールド座標に変換するヘルパー関数
 */
XMFLOAT3 MapGenerationSystem::GetWorldPosition(int x, int y, const MapStageConfig& config)
{
    // configから動的な定数を取得
    const float TILE_SIZE = config.tileSize;
    const int GRID_SIZE_X = config.gridSizeX;
    const int GRID_SIZE_Y = config.gridSizeY;

    // マップの中心をワールド原点(0, 0, 0)とするためのオフセット計算
    // X軸の中心オフセットを動的に計算
    const float MAP_CENTER_OFFSET_X = (GRID_SIZE_X / 2.0f) * TILE_SIZE;
    // Z軸の中心オフセットを動的に計算
    const float MAP_CENTER_OFFSET_Z = (GRID_SIZE_Y / 2.0f) * TILE_SIZE;

    // ハードコードされた定数からconfigのtileSizeを使用するように変更
    constexpr float X_ADJUSTMENT = 0.5f;
    constexpr float Z_ADJUSTMENT = 1.0f;

    XMFLOAT3 pos;
    // X座標: (グリッドX * TILE_SIZE - MAP_CENTER_OFFSET_X) + X_ADJUSTMENT * TILE_SIZE
    pos.x = (float)x * TILE_SIZE - MAP_CENTER_OFFSET_X + X_ADJUSTMENT * TILE_SIZE;
    // Y座標: 地面なので0
    pos.y = 0.0f;
    // Z座標: (グリッドY * TILE_SIZE - MAP_CENTER_OFFSET_Z) + Z_ADJUSTMENT * TILE_SIZE
    pos.z = (float)y * TILE_SIZE - MAP_CENTER_OFFSET_Z + Z_ADJUSTMENT * TILE_SIZE;

    return pos;
}


void MapGenerationSystem::DrawDebugLines()
{
    using namespace DirectX;
    // MapComponentを持つエンティティを取得
    ECS::EntityID mapEntity = FindFirstEntityWithComponent<MapComponent>(m_coordinator);
    if (mapEntity == INVALID_ENTITY_ID) return;

    MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);

    // MapComponentから動的な定数を取得
    const int GRID_SIZE_X = mapComp.gridSizeX;
    const int GRID_SIZE_Y = mapComp.gridSizeY;
    const float TILE_SIZE = mapComp.tileSize;
    const float WALL_HEIGHT = mapComp.wallHeight;

    XMFLOAT4 wallColor = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f); // 緑色

    MapStageConfig tempConfig;
    tempConfig.gridSizeX = mapComp.gridSizeX;
    tempConfig.gridSizeY = mapComp.gridSizeY;
    tempConfig.tileSize = mapComp.tileSize;
    tempConfig.wallHeight = mapComp.wallHeight;

    // 描画の高さを床の上(Y=0.1f)に設定して、Zファイティングを避ける
    const float LINE_HEIGHT_ADJUST = tempConfig.wallHeight + 1.0f; // WALL_HEIGHTは20.0fなので、LINE_HEIGHTは21.0f
    const float LINE_HEIGHT = WALL_HEIGHT + LINE_HEIGHT_ADJUST;

    // グリッド全体を走査し、壁と通路の境界線を描画
    for (int y = 0; y < GRID_SIZE_Y; ++y)
    {
        for (int x = 0; x < GRID_SIZE_X; ++x)
        {
            // 現在のセルタイプ
            CellType currentType = mapComp.grid[y][x].type;

            // 壁または未訪問のセルは描画対象外
            if (currentType == CellType::Wall || currentType == CellType::Unvisited)
            {
                continue;
            }

            // 1. 北側の境界線 (y-1)
            if (y > 0)
            {
                CellType neighborType = mapComp.grid[y - 1][x].type;
                if (neighborType == CellType::Wall)
                {
                    XMFLOAT3 start = GetWorldPosition(x, y, tempConfig); // <--- tempConfigを渡す
                    start.y = LINE_HEIGHT;

                    XMFLOAT3 end = start;
                    end.x += TILE_SIZE;

                    Geometory::AddLine(start, end, wallColor);
                }
            }

            // 2. 東側の境界線 (x+1)
            if (x < GRID_SIZE_X - 1)
            {
                CellType neighborType = mapComp.grid[y][x + 1].type;
                if (neighborType == CellType::Wall)
                {
                    XMFLOAT3 start = GetWorldPosition(x, y, tempConfig); // <--- tempConfigを渡す
                    start.x += TILE_SIZE;
                    start.y = LINE_HEIGHT;

                    XMFLOAT3 end = start;
                    end.z += TILE_SIZE;

                    Geometory::AddLine(start, end, wallColor);
                }
            }

            // 3. 南側の境界線 (y+1)
            if (y < GRID_SIZE_Y - 1)
            {
                CellType neighborType = mapComp.grid[y + 1][x].type;
                if (neighborType == CellType::Wall)
                {
                    XMFLOAT3 start = GetWorldPosition(x, y, tempConfig); // <--- tempConfigを渡す
                    start.z += TILE_SIZE;
                    start.y = LINE_HEIGHT;

                    XMFLOAT3 end = start;
                    end.x += TILE_SIZE;

                    Geometory::AddLine(start, end, wallColor);
                }
            }

            // 4. 西側の境界線 (x-1)
            if (x > 0)
            {
                CellType neighborType = mapComp.grid[y][x - 1].type;
                if (neighborType == CellType::Wall)
                {
                    XMFLOAT3 start = GetWorldPosition(x, y, tempConfig); // <--- tempConfigを渡す
                    start.y = LINE_HEIGHT;

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
void MapGenerationSystem::SpawnMapEntities(MapComponent& mapComp, const MapStageConfig& config)
{
    EntityID tracker = FindFirstEntityWithComponent<ItemTrackerComponent>(m_coordinator);
    auto& trackerComp = m_coordinator->GetComponent<ItemTrackerComponent>(tracker);
    std::vector<EntityID> teleportEntities;

    // configから動的な定数を取得
    const int GRID_SIZE_X = config.gridSizeX;
    const int GRID_SIZE_Y = config.gridSizeY;
    const float TILE_SIZE = config.tileSize;
    const float WALL_HEIGHT = config.wallHeight;

    // --------------------------------------------------------------------------------
    // 【最適化】Greedy Meshingのための処理済みフラグ用グリッド
    // --------------------------------------------------------------------------------
    // グリッドサイズを動的に変更
    std::vector<std::vector<bool>> processed(GRID_SIZE_Y, std::vector<bool>(GRID_SIZE_X, false));
    XMINT2 doorGridPos = { -1, -1 };

    // ====================================================================
    // 0. ドアの生成 (壁より先に配置し、壁生成をブロックする)
    // ====================================================================
    auto SpawnDoorAt = [&](XMINT2 targetPos) {
        int dx[] = { -1, 1, 0, 0 };
        int dy[] = { 0, 0, -1, 1 };
        float angles[] = { 90.0f, -90.0f, 0.0f, 180.0f };

        for (int i = 0; i < 4; ++i)
        {
            int nx = targetPos.x + dx[i];
            int ny = targetPos.y + dy[i];

            // グリッド範囲内チェック
            if (nx >= 0 && nx < GRID_SIZE_X && ny >= 0 && ny < GRID_SIZE_Y)
            {
                // 外周かつ壁である場所を探す
                bool isOuter = (nx == 0 || nx == GRID_SIZE_X - 1 || ny == 0 || ny == GRID_SIZE_Y - 1);

                // ★修正: 外周でなくても、スタート地点の隣接壁ならOKにする（条件緩和）
                // ただし、部屋の壁ではなく「迷路の外壁」であることを確認したい場合は isOuter を維持
                // ここではデバッグのため「壁ならOK」としつつ、外周優先にするロジックも考えられるが、
                // まずは isOuter && Wall で見つかるはず。見つからないなら nx, ny の計算か Wall 判定が怪しい。

                if (isOuter && mapComp.grid[ny][nx].type == CellType::Wall)
                {
                    // 座標計算
                    XMFLOAT3 basePos = GetWorldPosition(nx, ny, config);
                    XMFLOAT3 centerPos = basePos;
                    centerPos.x += TILE_SIZE / 2.0f;
                    centerPos.z += TILE_SIZE / 2.0f;

                    // 1. ドア生成
                    XMFLOAT3 doorPos = centerPos;
                    doorPos.y = TILE_SIZE / 2.0f;

                    float radianAngle = DirectX::XMConvertToRadians(angles[i]);
                    EntityFactory::CreateDoor(m_coordinator, doorPos, radianAngle, true);

                    // 2. ドア直下の床
                    XMFLOAT3 groundPos = centerPos;
                    groundPos.y = -0.01f;
                    EntityFactory::CreateGround(m_coordinator, groundPos, XMFLOAT3(TILE_SIZE, TILE_SIZE, TILE_SIZE));

                    // 3. 外側の花道 (進入路)
                    for (int k = 1; k <= 3; ++k)
                    {
                        XMFLOAT3 outPos = centerPos;
                        // 方向ベクトルを加算 (dx, dy は -1, 0, 1 なので、TILE_SIZE を掛ける)
                        outPos.x += (dx[i] * TILE_SIZE) * k;
                        outPos.z += (dy[i] * TILE_SIZE) * k;
                        outPos.y = -0.01f;
                        EntityFactory::CreateGround(m_coordinator, outPos, XMFLOAT3(TILE_SIZE, TILE_SIZE, TILE_SIZE));
                    }

                    // フラグ更新
                    processed[ny][nx] = true;
                    doorGridPos = { nx, ny }; // ★重要: 座標を記憶

                    return; // 生成完了
                }
            }
        }
        // ループを抜けてもここに来る＝ドア生成失敗
        printf("Error: Failed to spawn door adjacent to Start(%d, %d)\n", targetPos.x, targetPos.y);
        };

    // スタート地点の隣接する「外周」にドアを作る
    // このドアを入場・脱出兼用とする
    SpawnDoorAt(mapComp.startPos);

    // ゴール地点のドア生成は削除 (兼用するため)
    // SpawnDoorAt(mapComp.goalPos, false);

    // --------------------------------------------------------------------
    // 1. 床Entityの生成
    // --------------------------------------------------------------------
    for (int y = 0; y < GRID_SIZE_Y; ++y)
    {
        for (int x = 0; x < GRID_SIZE_X; ++x)
        {
            if (mapComp.grid[y][x].type == CellType::Wall ||
                mapComp.grid[y][x].type == CellType::Unvisited ||
                processed[y][x])
            {
                continue;
            }

            // 座標計算
            XMFLOAT3 basePos = GetWorldPosition(x, y, config);
            XMFLOAT3 centerPos = basePos;
            centerPos.x += TILE_SIZE / 2.0f; // マスの中心へ
            centerPos.z += TILE_SIZE / 2.0f;
            centerPos.y = -0.01f;

            // 床 Entityの生成 (サイズは常に 1x1タイル分)
            EntityFactory::CreateGround(
                m_coordinator,
                centerPos,
                XMFLOAT3(TILE_SIZE, TILE_SIZE, TILE_SIZE) // X, Z ともに TILE_SIZE 固定
            );

            // このマスを処理済みにする
            processed[y][x] = true;
        }
    }


    // --------------------------------------------------------------------
    // 2. 壁Entityの生成
    // --------------------------------------------------------------------
    // 処理済みフラグをリセットして壁の結合に再利用
    processed.assign(GRID_SIZE_Y, std::vector<bool>(GRID_SIZE_X, false));

    if (doorGridPos.x != -1 && doorGridPos.y != -1)
    {
        processed[doorGridPos.y][doorGridPos.x] = true;
    }

    for (int y = 0; y < GRID_SIZE_Y; ++y)
    {
        for (int x = 0; x < GRID_SIZE_X; ++x)
        {
            if (mapComp.grid[y][x].type != CellType::Wall || processed[y][x]) continue;

            // 座標計算
            XMFLOAT3 basePos = GetWorldPosition(x, y, config);
            XMFLOAT3 centerPos = basePos;
            centerPos.x += TILE_SIZE / 2.0f;
            centerPos.z += TILE_SIZE / 2.0f;
            centerPos.y = WALL_HEIGHT / 2.0f;

            // 壁 Entityの生成
            EntityFactory::CreateWall(
                m_coordinator,
                centerPos,
                XMFLOAT3(TILE_SIZE, WALL_HEIGHT, TILE_SIZE), // X, Z ともに TILE_SIZE 固定
                0.0f
            );

            // このマスを処理済みにする
            processed[y][x] = true;
        }
    }

    // --------------------------------------------------------------------
    // 3. 特殊 Entityの配置（単独セル）
    // --------------------------------------------------------------------

    for (int y = 0; y < GRID_SIZE_Y; ++y) // GRID_SIZE_Y を使用
    {
        for (int x = 0; x < GRID_SIZE_X; ++x) // GRID_SIZE_X を使用
        {
            Cell& cell = mapComp.grid[y][x];

            switch (cell.type)
            {
            case CellType::Start:
            case CellType::Goal:
            case CellType::Item:
            case CellType::Guard:
            case CellType::Taser:
                // TODO: CellType::Room に配置するギミックがある場合はここに追加
                // case CellType::Teleporter: 
            {
                // 単独セルの中心座標を取得
                XMFLOAT3 cellCenter = GetWorldPosition(x, y, config); // <--- configを渡す
                // GetWorldPositionは角座標を返すので、単独セルの中心オフセットを追加
                cellCenter.x += TILE_SIZE / 2.0f;
                cellCenter.z += TILE_SIZE / 2.0f;

                // 特殊オブジェクトのY座標を床の表面に合わせる (TILE_SIZE / 2.0f = プレイヤー/アイテムの中心)
                cellCenter.y = TILE_SIZE / 1.3f;

                if (cell.type == CellType::Start) {
                    EntityFactory::CreatePlayer(m_coordinator, cellCenter);
                    EntityFactory::CreateEnemySpawner(m_coordinator, cellCenter, 7.0f);
                }
                else if (cell.type == CellType::Goal) {
                }
                else if (cell.type == CellType::Item) {

                    // configからアイテムリストを取得
                    std::string itemID = "Unknown";
                    if (m_itemSpawnIndex < config.items.size()) {
                        itemID = config.items[m_itemSpawnIndex];
                    }
                    else
                    {
                        if (!config.items.empty()) itemID = config.items.back();
                    }

                    trackerComp.targetItemIDs.push_back(itemID);

                    //配列itemPositions内でインデックスを探して順序番号を決める
                    int orderIndex = 0;
                    //設定で順序モードが有効な時番号を振る
                    if (config.useOrderedCollection)
                    {
                        orderIndex = m_itemSpawnIndex + 1;
                    }

                    EntityFactory::CreateCollectable(m_coordinator, cellCenter, orderIndex, itemID);

                    m_itemSpawnIndex++;
                }
                else if (cell.type == CellType::Guard) {
                    //EntityFactory::CreateGuard(m_coordinator, cellCenter);
                }
                else if (cell.type == CellType::Taser) {
                    XMFLOAT3 taserPos = cellCenter;
                    taserPos.y += 0.0f;
                    EntityFactory::CreateTaser(m_coordinator, taserPos);
                }
            }
            break;
            default:
                break;
            }
        }
    }
    for (const auto& pair : mapComp.teleportPairs)
    {

        XMFLOAT3 posA = GetWorldPosition(pair.posA.x, pair.posA.y, config);
        posA.x += config.tileSize / 2.0f; posA.z += config.tileSize / 2.0f;
        // ★修正: 床の表面（2.5f付近）に出す。少し浮かせて 2.6f に設定
        posA.y = config.tileSize / 2.0f + 0.2f;

        // B地点も同様に修正
        XMFLOAT3 posB = GetWorldPosition(pair.posB.x, pair.posB.y, config);
        posB.x += config.tileSize / 2.0f; posB.z += config.tileSize / 2.0f;
        posB.y = config.tileSize / 2.0f + 0.2f;

        // エンティティ生成
        ECS::EntityID entA = EntityFactory::CreateTeleporter(m_coordinator, posA);
        ECS::EntityID entB = EntityFactory::CreateTeleporter(m_coordinator, posB);

        // ★ここで相互にお互いのIDを targetEntity にセットする
        auto& compA = m_coordinator->GetComponent<TeleportComponent>(entA);
        auto& compB = m_coordinator->GetComponent<TeleportComponent>(entB);

        compA.targetEntity = entB;
        compB.targetEntity = entA;

        printf("[Debug] Teleporter Linked: %d <-> %d\n", entA, entB);
    }
}