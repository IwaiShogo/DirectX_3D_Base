/*****************************************************************//**
 * @file	MapGenerationSystem.cpp
 * @brief	BSP/MSTによる館内レイアウト生成アルゴリズムの実装。
 * 
 * @details	BSPで部屋を配置し、Delaunay/MSTで接続性を保証した通路を生成する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：BSP/MSTの骨格実装
 * 
 * @update	2025/11/10	最終更新日
 * 			作業内容：	- 修正：InstantiateRoom/Corridorに開口部と配置ロジックを実装。
 * 
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/Systems/MapGenerationSystem.h"
#include "ECS/EntityFactory.h" 
#include "ECS/ECS.h"
#include "ProcGen/LevelGenerator.h" // BSP/MSTジェネレーター
#include <algorithm>
#include <random>
#include <cmath>

using namespace DirectX;
using namespace ProcGen;
using namespace ECS;

//--------------------------------------
// 補助定数
//--------------------------------------
const float WALL_THICKNESS = 0.2f;
const float ITEM_HEIGHT = 0.5f;
const float CORRIDOR_TOLERANCE = WALL_THICKNESS + 0.1f; // 通路の開口部判定に使う許容誤差

//--------------------------------------
// ヘルパー関数：通路のEntity生成（InstantiateCorridor）
//--------------------------------------
void InstantiateCorridor(Coordinator* coordinator, const Segment& seg, const GridMapping& map)
{
    const float WALL_H = map.wallHeight; // 2.5f
    const float WIDTH = seg.width * map.scaleXZ; // 通路の幅 (例: 2.0f * 1.0f = 2.0f)
    const float HALF_WIDTH = WIDTH / 2.0f;
    const float HALF_THICKNESS = WALL_THICKNESS / 2.0f;

    // 1. セグメントの長さ、中心、向きを計算

    // ベクトル AB (ワールド座標)
    float dx = seg.b.x - seg.a.x;
    float dz = seg.b.z - seg.a.z;
    float original_length = std::sqrt(dx * dx + dz * dz);
    
    // 通路の長さを両端の部屋の壁の厚み分だけ短くする
    // 通路は部屋の中心から始まり、部屋の中心で終わるため、両端で壁の厚み分短縮が必要
    const float TOTAL_WALL_SKIP = WALL_THICKNESS * map.scaleXZ; // 通路を部屋の壁の厚み分内側から開始/終了
    float length = original_length - 2.0f * TOTAL_WALL_SKIP;

    // 通路長が WALL_THICKNESS 以下の場合は無視（ほぼ点であるため）
    if (length < 1e-3f) return;

    // 短縮されたセグメントの中心座標 (中点)
    XMFLOAT3 centerPos;

    // 通路の方向ベクトルを正規化
    float inv_original_length = 1.0f / original_length;
    float dir_x = dx * inv_original_length; // X方向単位ベクトル
    float dir_z = dz * inv_original_length; // Z方向単位ベクトル

    // 侵食分だけ中心をシフトさせる
    // 始点(seg.a)から (全長 / 2) - (侵食分) だけ離れた位置
    float half_len_original = original_length / 2.0f;
    float shift_amount = half_len_original - (length / 2.0f);

    // 中心点 = seg.a + (方向ベクトル * 距離)
    float original_center_x = seg.a.x + dir_x * half_len_original;
    float original_center_z = seg.a.z + dir_z * half_len_original;

    centerPos.x = original_center_x;
    centerPos.z = original_center_z;
    centerPos.y = map.yFloor; // Y座標は一旦床の高さに設定

    // 元の中心点を使い、スケールと回転で短縮を表現するのがシンプルです。
    float rotationY = 0.0f;

    // 2. 通路の床 Entity の生成
    {
        XMFLOAT3 floorScale;

        if (std::abs(dx) > std::abs(dz)) // 主にX軸方向のセグメント
        {
            // X:短縮後の長さ(length), Y:厚(T), Z:幅(WIDTH)
            floorScale = { length, WALL_THICKNESS, WIDTH }; // length を使用
            rotationY = 0.0f;
        }
        else // 主にZ軸方向のセグメント
        {
            // X:幅(WIDTH), Y:厚(T), Z:短縮後の長さ(length)
            floorScale = { WIDTH, WALL_THICKNESS, length }; // length を使用
            rotationY = 0.0f;
        }

        // Y座標を床Entityの中心に調整
        centerPos.y = map.yFloor - HALF_THICKNESS; // Y軸位置を元に戻す

        // EntityFactory::CreateCorridor は回転Yに対応したGround/Wallファクトリを使用
        EntityFactory::CreateCorridor(coordinator, centerPos, floorScale, rotationY);
    }

    // 3. 通路の壁 Entity の生成

    // Y座標は床から壁の半分の高さに持ち上げる
    float wallY = map.yFloor + WALL_H / 2.0f;

    // 壁の中心座標を再計算
    XMFLOAT3 wallCenter = centerPos;
    wallCenter.y = wallY;

    // 主軸方向を再判定
    if (std::abs(dx) > std::abs(dz)) // X軸セグメント (RotationY = 0)
    {
        // X:短縮後の長さ(length), Y:高(H), Z:厚(T)
        XMFLOAT3 wallScale = { length, WALL_H, WALL_THICKNESS };

        // 壁 1: Z軸 + 側 (右)
        XMFLOAT3 pos1 = wallCenter;
        pos1.z += (HALF_WIDTH + HALF_THICKNESS); // Z軸+方向へ、半幅 + 半厚み分シフト
        EntityFactory::CreateWall(coordinator, pos1, wallScale);

        // 壁 2: Z軸 - 側 (左)
        XMFLOAT3 pos2 = wallCenter;
        pos2.z -= (HALF_WIDTH + HALF_THICKNESS); // Z軸-方向へ、半幅 + 半厚み分シフト
        EntityFactory::CreateWall(coordinator, pos2, wallScale);
    }
    else // Z軸セグメント (RotationY = 90度)
    {
        // X:厚(T), Y:高(H), Z:短縮後の長さ(length)
        XMFLOAT3 wallScale = { WALL_THICKNESS, WALL_H, length };

        // 壁 1: X軸 + 側 (右)
        XMFLOAT3 pos1 = wallCenter;
        // 【修正】: 回転しているため、X軸方向に幅分シフトする（通路がZ軸に沿っているため）
        //           RotationY=90度の場合、X軸スケールが厚み、Z軸スケールが長さになるように定義されている。
        //           壁の位置は、通路のZ軸に垂直なX軸方向に移動させる。
        pos1.x += (HALF_WIDTH + HALF_THICKNESS);
        EntityFactory::CreateWall(coordinator, pos1, wallScale);

        // 壁 2: X軸 - 側 (左)
        XMFLOAT3 pos2 = wallCenter;
        pos2.x -= (HALF_WIDTH + HALF_THICKNESS);
        EntityFactory::CreateWall(coordinator, pos2, wallScale);
    }
}

//--------------------------------------
// ヘルパー関数：BSPの結果に基づき、部屋のEntityを生成
//--------------------------------------
void InstantiateRoom(
    Coordinator* coordinator,
    const Room& room,
    const GridMapping& map,
    uint32_t& totalItems,
    ECS::EntityID& playerSpawnID,
    ECS::EntityID& guardSpawnID)
{
    // 座標系の設定
    // BSP/MSTはXZ平面で動作するため、Y座標は床高さ map.yFloor を使用します。
    const float WALL_H = map.wallHeight;
    const float SCALE = map.scaleXZ;
    const float HALF_THICKNESS = WALL_THICKNESS / 2.0f;
    const float MIN_WALL_SEGMENT = 0.1f; // 壁の最小長
    const float CORRIDOR_W = 2.0f * SCALE; // 通路の標準幅（LevelGenerator.hのSegment::width=2.0fに依存）
    const float INSET_X = CORRIDOR_W / 2.0f; // 接続される通路の中心位置を特定するためのインセット

    const RectF& rc = room.rect;

    // 1. 床 Entity の生成
    {
        // 床のサイズ（rc.w, rc.h をそのまま使用）
        XMFLOAT3 floorScale = { rc.w * SCALE, WALL_THICKNESS, rc.h * SCALE };

        // 床の中心位置
        float centerX = (rc.x + rc.w / 2.0f) * SCALE;
        float centerZ = (rc.y + rc.h / 2.0f) * SCALE;

        // Y座標は map.yFloor に WALL_THICKNESS の半分を加えて、床Entityの中心位置とする
        XMFLOAT3 floorPos = { centerX, map.yFloor - HALF_THICKNESS, centerZ };

        // EntityFactory::CreateGround を使用して床を生成
        EntityFactory::CreateGround(coordinator, floorPos, floorScale);
    }

    // 2. 4つの壁 Entity の生成 (開口部処理を導入)

    // 壁の長さのスケール定義 (Step 4で修正済み)
    // X方向に広がる壁 (奥と手前): {長さ, 高さ, 厚み} -> {rc.w, WALL_H, WALL_THICKNESS}
    XMFLOAT3 wallScaleX_Base = { rc.w * SCALE, WALL_H, WALL_THICKNESS * SCALE };
    // Z方向に広がる壁 (右と左): {厚み, 高さ, 長さ} -> {WALL_THICKNESS, WALL_H, rc.h}
    XMFLOAT3 wallScaleZ_Base = { WALL_THICKNESS * SCALE, WALL_H, rc.h * SCALE };

    float wallY = map.yFloor + WALL_H / 2.0f;

    // 開口部を考慮した壁生成のヘルパー
    auto create_split_walls_x = [&](int side, float posZ) {
        // side 0:南(-Z), 1:北(+Z)

        if (room.connections[side].isConnected) {
            const ConnectionInfo& conn = room.connections[side];

            float full_len = rc.w * SCALE;
            float wall_start_x = rc.x * SCALE;
            float center_of_room_X = (rc.x + rc.w / 2.0f) * SCALE;

            // 1. 左側の壁 (X軸 -)
            float len1 = full_len * conn.start; // conn.start が 0.0 以下になると len1 は負になる
            if (len1 > MIN_WALL_SEGMENT) {
                XMFLOAT3 scale1 = { len1, WALL_H, WALL_THICKNESS * SCALE };
                float posX1 = wall_start_x + len1 / 2.0f;
                EntityFactory::CreateWall(coordinator, { posX1, wallY, posZ }, scale1);
            }

            // 2. 右側の壁 (X軸 +)
            float len2 = full_len * (1.0f - conn.end); // conn.end が 1.0 以上になると len2 は負になる
            if (len2 > MIN_WALL_SEGMENT) {
                XMFLOAT3 scale2 = { len2, WALL_H, WALL_THICKNESS * SCALE };
                float posX2 = wall_start_x + full_len - len2 / 2.0f; // full_len - (len2 / 2.0f)
                EntityFactory::CreateWall(coordinator, { posX2, wallY, posZ }, scale2);
            }
            // 開口部 (conn.end - conn.start) は空いたままになる
        }
        else {
            // 接続がない場合、通常の壁を生成
            float posX = (rc.x + rc.w / 2.0f) * SCALE;
            EntityFactory::CreateWall(coordinator, { posX, wallY, posZ }, wallScaleX_Base);
        }
        };

    // Z方向に広がる壁 (右と左) の分割生成ヘルパー
    auto create_split_walls_z = [&](int side, float posX) {
        // side 2:西(-X), 3:東(+X)

        if (room.connections[side].isConnected) {
            const ConnectionInfo& conn = room.connections[side];

            float full_len = rc.h * SCALE;
            float wall_start_z = rc.y * SCALE;

            // 1. 下側の壁 (Z軸 -)
            float len1 = full_len * conn.start;
            if (len1 > MIN_WALL_SEGMENT) {
                XMFLOAT3 scale1 = { WALL_THICKNESS * SCALE, WALL_H, len1 };
                float posZ1 = wall_start_z + len1 / 2.0f;
                EntityFactory::CreateWall(coordinator, { posX, wallY, posZ1 }, scale1);
            }

            // 2. 上側の壁 (Z軸 +)
            float len2 = full_len * (1.0f - conn.end);
            if (len2 > MIN_WALL_SEGMENT) {
                XMFLOAT3 scale2 = { WALL_THICKNESS * SCALE, WALL_H, len2 };
                float posZ2 = wall_start_z + full_len - len2 / 2.0f;
                EntityFactory::CreateWall(coordinator, { posX, wallY, posZ2 }, scale2);
            }
            // 開口部 (conn.end - conn.start) は空いたままになる
        }
        else {
            // 接続がない場合、通常の壁を生成
            float posZ = (rc.y + rc.h / 2.0f) * SCALE;
            EntityFactory::CreateWall(coordinator, { posX, wallY, posZ }, wallScaleZ_Base);
        }
        };

    // --- 壁 1: 南の壁 (Z軸 - 方向) --- (side=0)
    float posZ_south = rc.y * SCALE - HALF_THICKNESS;
    create_split_walls_x(0, posZ_south);

    // --- 壁 2: 北の壁 (Z軸 + 方向) --- (side=1)
    float posZ_north = (rc.y + rc.h) * SCALE + HALF_THICKNESS;
    create_split_walls_x(1, posZ_north);

    // --- 壁 3: 西の壁 (X軸 - 方向) --- (side=2)
    float posX_west = rc.x * SCALE - HALF_THICKNESS;
    create_split_walls_z(2, posX_west);

    // --- 壁 4: 東の壁 (X軸 + 方向) --- (side=3)
    float posX_east = (rc.x + rc.w) * SCALE + HALF_THICKNESS;
    create_split_walls_z(3, posX_east);

    // --- 3. プレイヤー、警備員、アイテムの配置 ---

    // 現在の部屋の中心座標（スポーン位置として使用）
    XMFLOAT3 roomCenter = { room.center.x * SCALE, ITEM_HEIGHT, room.center.y * SCALE };

    // 3-1. プレイヤーの配置 (最初の部屋にのみ配置)
    if (playerSpawnID == ECS::INVALID_ENTITY_ID)
    {
        playerSpawnID = EntityFactory::CreatePlayer(coordinator, roomCenter);
    }

    // 3-2. 警備員の配置 (ランダムな確率で配置)
    // 警備員はマップ全体で1体のみとし、ランダムに選ばれた部屋に配置するロジックをシミュレート
    // ただし、このInstantiateRoomの呼び出し順が部屋の生成順に依存するため、
    // ここでは「最初の部屋には配置しない」+「まだ配置されていなければ5%の確率で配置」とします。
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    if (playerSpawnID != ECS::INVALID_ENTITY_ID && // 最初の部屋ではない
        guardSpawnID == ECS::INVALID_ENTITY_ID && // まだ警備員が配置されていない
        dis(gen) < 0.05f) // 5% の確率で配置
    {
        guardSpawnID = EntityFactory::CreateGuard(coordinator, roomCenter);
    }

    // 3-3. アイテムの配置 (各部屋にランダムな確率で配置)
    if (dis(gen) < 0.7f) // 70%の確率でアイテムを配置
    {
        EntityFactory::CreateCollectable(coordinator, roomCenter);
        totalItems++;
    }
}


//--------------------------------------
// メイン：マップ生成処理
//--------------------------------------
void MapGenerationSystem::GenerateMap(ECS::EntityID mapEntityID)
{
    if (!m_coordinator) return;

    // 【重要】mapは非const参照で取得 (接続情報を書き込むため)
    MapComponent& map = m_coordinator->GetComponent<MapComponent>(mapEntityID);

    // 1. LevelGeneratorの実行
    LevelGenerator generator;
    map.layout = generator.GenerateMuseum(map.areaW, map.areaH, {}, {}, {});

    // --- 2.5. 部屋と通路の接続点を計算し、Room::connectionsを更新 ---
    // 【修正点】: このブロックを Entity生成ループの前に移動させます。

    // 接続判定の許容誤差
    const float EPSILON = 0.5f;
    const float CORRIDOR_W = 2.0f; // 通路の幅

    // 全ての部屋について、接続する通路を探す (map.layout.roomsを更新)
    for (auto& room : map.layout.rooms)
    {
        // 部屋の境界座標 (BSPローカル座標系)
        const RectF& rc = room.rect;
        float x_min = rc.x;
        float x_max = rc.x + rc.w;
        float z_min = rc.y;
        float z_max = rc.y + rc.h;

        // 部屋の辺の長さ (BSPローカル座標系)
        float len_x = rc.w;
        float len_z = rc.h;

        for (const auto& seg : map.layout.corridors)
        {
            // 接続点の候補は、通路の端点 (seg.a または seg.b)
            XMFLOAT3 candidates[2] = { seg.a, seg.b };

            for (const auto& candidate : candidates)
            {
                // 【修正箇所】: 接続点の座標をBSPローカル座標系に変換
                float conn_x = candidate.x / map.mapping.scaleXZ;
                float conn_z = candidate.z / map.mapping.scaleXZ;

                // ----------------------------------------------------------------------
                // 通路の端点が、部屋の境界線上に存在するかを判定 (BSPローカル座標系で判定)
                // ----------------------------------------------------------------------

                int side = -1; // 0:南(-Z), 1:北(+Z), 2:西(-X), 3:東(+X)

                // 1. X軸に沿った辺（南/北）との接続判定
                // X座標が部屋の幅内に収まっている (BSP座標系で判定)
                if (conn_x > x_min - EPSILON && conn_x < x_max + EPSILON)
                {
                    if (std::abs(conn_z - z_min) < EPSILON) {
                        side = 0; // 南(-Z)の辺
                    }
                    else if (std::abs(conn_z - z_max) < EPSILON) {
                        side = 1; // 北(+Z)の辺
                    }
                }

                // 2. Z軸に沿った辺（西/東）との接続判定
                // Z座標が部屋の高さ内に収まっている (BSP座標系で判定)
                if (side == -1 && conn_z > z_min - EPSILON && conn_z < z_max + EPSILON)
                {
                    if (std::abs(conn_x - x_min) < EPSILON) {
                        side = 2; // 西(-X)の辺
                    }
                    else if (std::abs(conn_x - x_max) < EPSILON) {
                        side = 3; // 東(+X)の辺
                    }
                }

                if (side == -1) continue; // この端点は現在の部屋の境界線に接続していない

                // ----------------------------------------------------------------------
                // 開口部パラメータの計算 (すべてBSPローカル座標系で完結)
                // ----------------------------------------------------------------------

                float wall_len;
                float wall_start;
                float relative_pos;

                if (side == 0 || side == 1) // 南(-Z) または 北(+Z)
                {
                    wall_len = len_x;
                    wall_start = x_min;
                    relative_pos = (conn_x - wall_start) / wall_len; // 辺に沿った相対位置 (0.0 - 1.0)
                }
                else // 西(-X) または 東(+X)
                {
                    wall_len = len_z;
                    wall_start = z_min;
                    relative_pos = (conn_z - wall_start) / wall_len; // 辺に沿った相対位置 (0.0 - 1.0)
                }

                room.connections[side].isConnected = true;

                // 開口部の開始位置と終了位置を計算 (CORRIDOR_Wもスケール未適用)
                float start_pos = relative_pos - (CORRIDOR_W / 2.0f / wall_len);
                float end_pos = relative_pos + (CORRIDOR_W / 2.0f / wall_len);

                // 開始/終了位置を [0.0, 1.0] にクランプする
                room.connections[side].start = std::max(0.0f, start_pos);
                room.connections[side].end = std::min(1.0f, end_pos);
            }
        }
    }
    // -------------------------------------------------------------------


    // 2. Entityのインスタンス化に必要な準備
    uint32_t totalItems = 0;
    ECS::EntityID playerID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID guardID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID gameControllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);


    // 部屋 Entityの生成
    // Room::connectionsが更新された後の map.layout.rooms から取得
    for (const auto& room : map.layout.rooms)
    {
        // InstantiateRoom 内で room.connections が参照される
        InstantiateRoom(m_coordinator, room, map.mapping, totalItems, playerID, guardID);
    }

    // 通路 Entityの生成 
    for (const auto& segment : map.layout.corridors)
    {
        InstantiateCorridor(m_coordinator, segment, map.mapping);
    }

    // 3. ItemTrackerComponentの総アイテム数を設定
    if (gameControllerID != ECS::INVALID_ENTITY_ID)
    {
        if (m_coordinator->m_entityManager->GetSignature(gameControllerID).test(m_coordinator->GetComponentTypeID<ItemTrackerComponent>()))
        {
            m_coordinator->GetComponent<ItemTrackerComponent>(gameControllerID).totalItems = totalItems;
        }
    }

    // 4. 脱出地点の配置 (最後の部屋の中心に配置する)
    // ※ 最後の部屋に配置するのではなく、**警備員が配置されていない部屋**の中からランダムに選ぶ方が良いですが、
    // 既存コードのロジック（最後の部屋）を尊重しつつ、EntityFactory::CreateExitPointのコメントアウトを外します。
    if (!map.layout.rooms.empty())
    {
        // ここで、マップ全体でプレイヤーと警備員が配置された部屋を除外してランダムに選ぶのが理想ですが、
        // 複雑になるため、ここでは「最後の部屋」に仮配置する既存ロジックをそのまま採用します。
        const auto& lastRoom = map.layout.rooms.back();
        XMFLOAT3 exitPos = { lastRoom.center.x * map.mapping.scaleXZ, ITEM_HEIGHT, lastRoom.center.y * map.mapping.scaleXZ };
        //EntityFactory::CreateExitPoint(m_coordinator, exitPos); // TODO: CreateExitPointは未実装の仮コード
    }
}