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
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
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

//--------------------------------------
// ヘルパー関数：通路のEntity生成 (簡略版)
//--------------------------------------
void InstantiateCorridor(Coordinator* coordinator, const Segment& seg, const GridMapping& map)
{
    // ... (1. 通路の Entity のサイズと位置の計算) ...
    // 通路の中心位置
    XMFLOAT3 center_pos;
    center_pos.x = (seg.a.x + seg.b.x) / 2.0f;
    center_pos.y = map.yFloor + 0.1f; // 床の厚みを考慮して少し持ち上げる
    center_pos.z = (seg.a.z + seg.b.z) / 2.0f;

    // 通路の長さと幅を計算
    XMVECTOR vec = XMLoadFloat3(&seg.b) - XMLoadFloat3(&seg.a);
    vec = XMVectorSetY(vec, 0.0f); // XZ平面のみ
    float length = XMVectorGetX(XMVector3Length(vec));
    float width = seg.width * map.scaleXZ; // 通路の幅

    // 通路の回転角度 (YAW)
    float rotationY = std::atan2(XMVectorGetX(vec), XMVectorGetZ(vec)); // Z軸からの角度

    // ... (2. 床のEntity生成) ...

    // XZ平面でのスケールを決定
    XMFLOAT3 floor_scale;
    XMFLOAT3 wall_scale;
    if (std::abs(seg.a.x - seg.b.x) > std::abs(seg.a.z - seg.b.z)) // X方向に長い通路 (横向き)
    {
        floor_scale = { length, 0.2f, width };
        wall_scale = { length, map.wallHeight, WALL_THICKNESS };
    }
    else // Z方向に長い通路 (縦向き)
    {
        floor_scale = { width, 0.2f, length };
        wall_scale = { WALL_THICKNESS, map.wallHeight, length };
    }

    // ... (省略: 3. Entity Factoryの呼び出し) ...

    // 床の配置
    EntityFactory::CreateGround(coordinator, center_pos, floor_scale);

    // 通路の両側の壁を配置 (TODOを解消)
    float half_width_offset = (width / 2.0f) + (WALL_THICKNESS / 2.0f);
    float wall_half_height = map.wallHeight / 2.0f;

    XMVECTOR center_vec = XMLoadFloat3(&center_pos);
    
    // forward_vec（進行方向）のXZ成分を取得
    float forward_x = std::sin(rotationY);
    float forward_z = std::cos(rotationY);

    // 進行方向に垂直な右方向ベクトル (通路の幅方向のオフセット用)
    // forward_vec をY軸周りに 90度 (π/2) 回転させる
    XMVECTOR right_vec = XMVectorSet(forward_z, 0.0f, -forward_x, 0.0f);

    // オフセット計算
    XMVECTOR offset_vec = right_vec * half_width_offset;

    // 壁1 (右側) の配置
    XMFLOAT3 pos1;
    XMStoreFloat3(&pos1, center_vec + offset_vec);
    pos1.y = wall_half_height; // 高さの中央に移動

    // 壁2 (左側) の配置
    XMFLOAT3 pos2;
    XMStoreFloat3(&pos2, center_vec - offset_vec);
    pos2.y = wall_half_height;

    // TODO: 壁の配置ロジックを実装し、CreateWallを呼び出す。
    // NOTE: EntityFactory::CreateWallは回転を考慮しないため、TransformComponentに回転を明示的に設定する必要がありますが、
    // EntityFactory::CreateWallのプロトタイプはXMFLOAT3 positionとXMFLOAT3 scaleのみを受け取ります。
    // ここでは、一時的に長辺/短辺の壁スケールを使用し、回転をEntityFactory内で設定すると仮定します。

    // 簡易対応: 軸に沿った壁配置 (回転は考慮しない簡略化された壁スケールを使用)
    // 厳密には、通路の回転角度 `rotationY` を考慮した EntityFactory::CreateWall のオーバーロードが必要です。
    // 現状の EntityFactory::CreateWall では回転ができないため、一旦、CreateGroundと同様に回転を無視した軸に沿った壁のスケールを渡します。
    // 長辺/短辺の判定ロジックを再利用
    if (std::abs(seg.a.x - seg.b.x) > std::abs(seg.a.z - seg.b.z))
    {
        // X方向に長い通路
        wall_scale = { length, map.wallHeight, WALL_THICKNESS };
    }
    else
    {
        // Z方向に長い通路
        wall_scale = { WALL_THICKNESS, map.wallHeight, length };
    }

    // 既存の CreateWall を使用し、回転を無視して壁を配置（要改良ポイント）
    EntityFactory::CreateWall(coordinator, pos1, wall_scale);
    EntityFactory::CreateWall(coordinator, pos2, wall_scale);
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
    // ワールド座標の計算
    float centerX = room.rect.x + room.rect.w / 2.0f;
    float centerZ = room.rect.y + room.rect.h / 2.0f;

    // pos_centerの定義
    XMFLOAT3 pos_center = { centerX * map.scaleXZ, map.yFloor, centerZ * map.scaleXZ };
    XMFLOAT3 scale_floor = { room.rect.w * map.scaleXZ, 0.2f, room.rect.h * map.scaleXZ };

    XMFLOAT4 wall_color(0.8f, 0.7f, 0.1f, 1.0f); // Backroomsの黄色

    // 1. 床のEntityを生成 (部屋全体)
    EntityFactory::CreateGround(coordinator, pos_center, scale_floor);

    // 2. 部屋の外周に壁 Entityを生成

    float wall_half_height = map.wallHeight / 2.0f;
    float room_w = room.rect.w * map.scaleXZ;
    float room_h = room.rect.h * map.scaleXZ;

    // 長辺の壁 (X方向) - 座標の調整: 中心から幅の半分だけ離し、壁の厚み半分を足す
    // 奥側の壁 (Z-方向)
    EntityFactory::CreateWall(coordinator,
        { pos_center.x, wall_half_height, pos_center.z - (room_h / 2.0f) + (WALL_THICKNESS / 2.0f) },
        { room_w, map.wallHeight, WALL_THICKNESS });
    // 手前側の壁 (Z+方向)
    EntityFactory::CreateWall(coordinator,
        { pos_center.x, wall_half_height, pos_center.z + (room_h / 2.0f) - (WALL_THICKNESS / 2.0f) },
        { room_w, map.wallHeight, WALL_THICKNESS });

    // 短辺の壁 (Z方向) - Xオフセット (角の重複を避けるため、長さを調整するか、角を個別に生成する必要がありますが、ここでは簡略化)
    EntityFactory::CreateWall(coordinator,
        { pos_center.x - (room_w / 2.0f) + (WALL_THICKNESS / 2.0f), wall_half_height, pos_center.z }, // 左側の壁
        { WALL_THICKNESS, map.wallHeight, room_h });
    EntityFactory::CreateWall(coordinator,
        { pos_center.x + (room_w / 2.0f) - (WALL_THICKNESS / 2.0f), wall_half_height, pos_center.z }, // 右側の壁
        { WALL_THICKNESS, map.wallHeight, room_h });

    // 3. スポーン/アイテムの配置

    // アイテムの配置 (部屋の中心付近)
    if (totalItems < 5) // アイテム数制限
    {
        EntityFactory::CreateCollectable(coordinator, { pos_center.x + 2.0f, ITEM_HEIGHT, pos_center.z + 2.0f });
        totalItems++;
    }

    // プレイヤーのスポーン位置を決定 (最初の部屋の中心)
    if (playerSpawnID == ECS::INVALID_ENTITY_ID)
    {
        playerSpawnID = EntityFactory::CreatePlayer(coordinator, { pos_center.x, map.wallHeight / 2.0f, pos_center.z });
        //guardSpawnID = EntityFactory::CreateGuard(coordinator, { pos_center.x + 5.0f, map.wallHeight / 2.0f, pos_center.z + 5.0f }, playerSpawnID);
    }
}


//--------------------------------------
// メイン：マップ生成処理
//--------------------------------------
void MapGenerationSystem::GenerateMap(ECS::EntityID mapEntityID)
{
    if (!m_coordinator) return;

    MapComponent& map = m_coordinator->GetComponent<MapComponent>(mapEntityID);

    // 1. LevelGeneratorの実行
    LevelGenerator generator;

    // BSP/MSTパラメータはデフォルト値を使用 (BSPParams{}など)
    map.layout = generator.GenerateMuseum(map.areaW, map.areaH, {}, {}, {}); // LevelGeneratorのロジックが実行される

    // 2. Entityのインスタンス化
    uint32_t totalItems = 0;
    ECS::EntityID playerID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID guardID = ECS::INVALID_ENTITY_ID;

    // GameController Entityを取得
    ECS::EntityID gameControllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);

    // 部屋 Entityの生成
    for (const auto& room : map.layout.rooms)
    {
        // BUG-08で問題となったローカル変数の定義を回避し、playerID, guardIDを更新する
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
    if (!map.layout.rooms.empty())
    {
        const auto& lastRoom = map.layout.rooms.back();
        XMFLOAT3 exitPos = { lastRoom.center.x * map.mapping.scaleXZ, ITEM_HEIGHT, lastRoom.center.y * map.mapping.scaleXZ };
        //EntityFactory::CreateExitPoint(m_coordinator, exitPos);
    }
}