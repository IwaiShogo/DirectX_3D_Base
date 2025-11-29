/*****************************************************************//**
 * @file	GuardAISystem.cpp
 * @brief	警備員AIの追跡ロジックを実装するシステム
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Fukudome Hiroaki / Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/09	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/AllComponents.h"
#include "ECS/AllSystems.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "Systems/Geometory.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <limits>

using namespace DirectX;
using namespace ECS;

// --------------------------------------------------------------------------------
// ヘルパー関数: XMINT2の比較とハッシュ化（A*実装の簡易化のため）
// --------------------------------------------------------------------------------

/**
 * @brief XMINT2に対する等価比較演算子
 */
inline bool operator==(const XMINT2& lhs, const XMINT2& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * @brief XMINT2をキーとして使用するための比較演算子 (std::map用)
 * @note std::map<XMINT2, AStarNode> を使用するために必要
 */
struct XMINT2Comparer
{
    bool operator()(const XMINT2& lhs, const XMINT2& rhs) const
    {
        if (lhs.y != rhs.y) return lhs.y < rhs.y;
        return lhs.x < rhs.x;
    }
};

// --------------------------------------------------------------------------------
// A*探索ロジックの本体 (ステップ2-1)
// --------------------------------------------------------------------------------

/**
 * @brief A*アルゴリズムを使用して、スタートからゴールまでの最短経路を探索する
 * @param startGrid - 探索開始グリッド座標
 * @param targetGrid - 探索目標グリッド座標 (プレイヤー位置)
 * @param mapComp - マップデータ
 * @return 経路の次のステップとなるグリッド座標。経路が見つからない場合は無効な座標(-1, -1)
 */
XMINT2 GuardAISystem::FindNextTargetGridPos(
    XMINT2 startGrid,
    XMINT2 targetGrid,
    const MapComponent& mapComp
)
{
    // マップの境界チェック (スタートとターゲットが有効なグリッド内か)
    const int MAX_INDEX_X = mapComp.gridSizeX - 1;
    const int MAX_INDEX_Y = mapComp.gridSizeY - 1;
    if (startGrid.x <= 0 || startGrid.x >= MAX_INDEX_X || startGrid.y <= 0 || startGrid.y >= MAX_INDEX_Y ||
        targetGrid.x <= 0 || targetGrid.x >= MAX_INDEX_X || targetGrid.y <= 0 || targetGrid.y >= MAX_INDEX_Y)
    {
        return { -1, -1 }; // 無効な位置
    }

    // 1. ノード管理セットの初期化
    // Open Set: 優先度キュー (fCostの最も低いノードを優先)
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;

    // 全ノード情報を保持するマップ (グリッド座標をキーとする)
    std::map<XMINT2, AStarNode, XMINT2Comparer> allNodes;

    // 2. スタートノードの初期化
    AStarNode startNode;
    startNode.gridPos = startGrid;
    startNode.gCost = 0.0f;
    startNode.hCost = (float)std::abs(targetGrid.x - startGrid.x) + std::abs(targetGrid.y - startGrid.y); // マンハッタン距離
    startNode.fCost = startNode.hCost;
    startNode.parentPos = { startGrid.x, startGrid.y }; // 親は自分自身とする

    openSet.push(startNode);
    allNodes[startGrid] = startNode;

    // 3. 探索ループ
    while (!openSet.empty())
    {
        // 3-1. Open SetからFコストが最小のノードを取り出す
        AStarNode current = openSet.top();
        openSet.pop();

        // ノードは複数回キューに入れられる可能性があるので、allNodesで最新情報を確認
        if (current.fCost > allNodes.at(current.gridPos).fCost) {
            continue;
        }

        // 3-2. 終了判定: ゴールに到達
        if (current.gridPos == targetGrid)
        {
            // 4. 経路の復元と次のターゲットの特定
            XMINT2 currentPos = targetGrid;
            XMINT2 nextTarget = currentPos;

            // 親ノードを辿り、スタートの次に来るノードを見つける
            while (allNodes.at(currentPos).parentPos.x != currentPos.x || allNodes.at(currentPos).parentPos.y != currentPos.y)
            {
                // 次のターゲット（スタートの隣）まで親を辿り続ける
                if (allNodes.at(currentPos).parentPos == startGrid)
                {
                    // スタートの親に到達したとき、currentPosがスタートの次のノードである
                    nextTarget = currentPos;
                    break;
                }
                currentPos = allNodes.at(currentPos).parentPos;
            }
            return nextTarget;
        }

        // 3-3. 隣接ノードの探索
        int directions[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } }; // 4方向移動 (上下左右)

        for (int i = 0; i < 4; ++i)
        {
            XMINT2 neighborPos = { current.gridPos.x + directions[i][0], current.gridPos.y + directions[i][1] };

            // 境界チェック
            if (neighborPos.x <= 0 || neighborPos.x >= mapComp.gridSizeX - 1 ||
                neighborPos.y <= 0 || neighborPos.y >= mapComp.gridSizeY - 1)
            {
                continue;
            }

            // 壁チェック: 壁セルは移動不可
            CellType cellType = mapComp.grid[neighborPos.y][neighborPos.x].type;
            if (cellType == CellType::Wall || cellType == CellType::Unvisited)
            {
                continue;
            }

            // 暫定Gコストの計算 (今回は直線移動なので+1.0f)
            float newGCost = current.gCost + 1.0f;

            // 隣接ノードがallNodesに存在しないか、または新しいGコストが既存より小さい場合
            if (allNodes.find(neighborPos) == allNodes.end() || newGCost < allNodes.at(neighborPos).gCost)
            {
                // ノードの更新または新規作成
                AStarNode& neighborNode = allNodes[neighborPos];

                neighborNode.gridPos = neighborPos;
                neighborNode.gCost = newGCost;
                neighborNode.hCost = (float)std::abs(targetGrid.x - neighborPos.x) + std::abs(targetGrid.y - neighborPos.y);
                neighborNode.fCost = neighborNode.gCost + neighborNode.hCost;
                neighborNode.parentPos = current.gridPos;

                // Open Setに追加 (更新の場合はOpen Setに古い情報が残るが、Fコストチェックでスキップされる)
                openSet.push(neighborNode);
            }
        }
    }

    // 経路が見つからなかった場合
    return { -1, -1 };
}

// --------------------------------------------------------------------------------
// ヘルパー関数: ワールド座標からグリッド座標への変換 (A*に利用)
// NOTE: MapGenerationSystem::GetWorldPositionの逆変換を行う
// --------------------------------------------------------------------------------
/**
 * @brief ワールド座標をグリッド座標 (XMINT2) に変換するヘルパー関数
 * @param worldPos - ワールド座標 (Y軸は無視)
 * @return グリッド座標 (X, Y)
 */
XMINT2 GuardAISystem::GetGridPosition(const XMFLOAT3& worldPos, const MapComponent& mapComp)
{
    const float MAP_CENTER_OFFSET_X = (mapComp.gridSizeX / 2.0f) * mapComp.tileSize; // 20.0f
    const float MAP_CENTER_OFFSET_Y = (mapComp.gridSizeY / 2.0f) * mapComp.tileSize;
    const float X_ADJUSTMENT = 0.5f * mapComp.tileSize; // 1.0f
    const float Z_ADJUSTMENT = 1.0f * mapComp.tileSize; // 2.0f

    float x_f = (worldPos.x - X_ADJUSTMENT + MAP_CENTER_OFFSET_X) / mapComp.tileSize;
    float y_f = (worldPos.z - Z_ADJUSTMENT + MAP_CENTER_OFFSET_Y) / mapComp.tileSize;

    // 四捨五入ではなく、単にキャストしてグリッドインデックスを取得
    int x = static_cast<int>(x_f);
    int y = static_cast<int>(y_f);

    // 境界クランプ
    x = std::min(std::max(0, x), mapComp.gridSizeX - 1);
    y = std::min(std::max(0, y), mapComp.gridSizeY - 1);

    return { x, y };
}

bool GuardAISystem::IsTargetInSight(const TransformComponent& guardTransform, const GuardComponent& guardInfo, const TransformComponent& targetTransform)
{
    // 1. 距離判定 (2乗距離で計算)
    XMVECTOR guardPos = XMLoadFloat3(&guardTransform.position);
    XMVECTOR targetPos = XMLoadFloat3(&targetTransform.position);
    XMVECTOR toTarget = targetPos - guardPos;

    // 高さを無視（XZ平面のみ）
    toTarget = XMVectorSetY(toTarget, 0.0f);

    XMVECTOR distSqVec = XMVector3LengthSq(toTarget);
    float distSq;
    XMStoreFloat(&distSq, distSqVec);

    if (distSq > guardInfo.viewRange * guardInfo.viewRange)
    {
        return false; // 距離外
    }

    // 2. 角度判定 (内積)
    XMVECTOR toTargetDir = XMVector3Normalize(toTarget);

    // Guardの現在の向き（Y軸回転）から前方ベクトルを算出
    float yawRad = XMConvertToRadians(guardTransform.rotation.y);
    // DirectX座標系 (Z+が前方と仮定)
    float dirX = std::sin(yawRad);
    float dirZ = std::cos(yawRad);

    XMVECTOR forwardDir = XMVectorSet(dirX, 0.0f, dirZ, 0.0f);

    // 内積を計算
    XMVECTOR dotVec = XMVector3Dot(forwardDir, toTargetDir);
    float dot;
    XMStoreFloat(&dot, dotVec);

    // 視野角の半分と比較 (dotが大きいほど正面に近い)
    float angleThreshold = std::cos(XMConvertToRadians(guardInfo.viewAngle * 0.5f));

    if (dot >= angleThreshold)
    {
        return true; // 視界内
    }

    return false;
}

// --------------------------------------------------------------------------------
// GuardAISystem::Update() の本体
// --------------------------------------------------------------------------------

void GuardAISystem::Update(float deltaTime)
{
    // MapComponentを持つEntity (GameController) を検索
    EntityID mapEntity = FindFirstEntityWithComponent<MapComponent>(m_coordinator);
    if (mapEntity == INVALID_ENTITY_ID) return;

    const MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);
    GameStateComponent& gameStateComp = m_coordinator->GetComponent<GameStateComponent>(mapEntity);

    // プレイヤーエンティティの検索 (追跡目標)
    EntityID playerEntity = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerEntity == INVALID_ENTITY_ID) return;

    // プレイヤーのTransformComponentを取得
    const TransformComponent& playerTransform = m_coordinator->GetComponent<TransformComponent>(playerEntity);
    XMINT2 playerGridPos = GetGridPosition(playerTransform.position, mapComp);

    // 動的マップサイズを取得
    const int GRID_SIZE_X = mapComp.gridSizeX;
    const int GRID_SIZE_Y = mapComp.gridSizeY;

    // 警備員エンティティ全体を反復処理
    for (auto const& entity : m_entities)
    {
        GuardComponent& guardComp = m_coordinator->GetComponent<GuardComponent>(entity);
        TransformComponent& guardTransform = m_coordinator->GetComponent<TransformComponent>(entity);
        RigidBodyComponent& guardRigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);

        // 1. トップビューモードでの初期化
        if (gameStateComp.currentMode == GameMode::SCOUTING_MODE)
        {
            // 警備員を非アクティブ化し、速度をゼロにする (スポーン位置は後で上書きされるため、初期位置は問わない)
            guardComp.isActive = false;
            guardComp.elapsedTime = 0.0f; // タイマーリセット
            guardRigidBody.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

            // NOTE: ここでプレイヤーと同じ位置に警備員を配置する初期スポーン処理は不要。
            // なぜなら、ACTION_MODEへの移行時に正確な位置に配置するため。
            continue; // トップビュー時は以降のAIロジックをスキップ
        }

        // 2. アクションモード (TPS) 移行後の遅延スポーン
        else if (gameStateComp.currentMode == GameMode::ACTION_MODE)
        {
            //// 追跡開始遅延時間を設定（例: 3秒）
            //guardComp.delayBeforeChase = 3.0f;
          
            // 警備員がまだアクティブでない場合のみ初期配置とタイマー処理を行う
            if (!guardComp.isActive)
            {
                // Y座標は床の高さに合わせる (MapGenerationSystem.cppのロジックを再現)
                guardTransform.position.y = mapComp.tileSize / 2.0f;

                guardComp.elapsedTime += deltaTime; // デルタタイムでタイマーを進める

                if (guardComp.elapsedTime >= guardComp.delayBeforeChase)
                {
                    // 遅延時間経過後、AI追跡を有効化
                    guardComp.isActive = true;
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST2");

                    // 以降、AI追跡ロジックに移行
                }
                else
                {
                    // 遅延中は動かない
                    guardRigidBody.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
                    continue; // 遅延中はAIロジックをスキップ
                }
            }
        }

        // -------------------------------------------------------------
        // 視界判定とゲームオーバー処理
        // -------------------------------------------------------------
        // 警備員がアクティブで、かつアクションモードの場合のみ判定
        if (guardComp.isActive && gameStateComp.currentMode == GameMode::ACTION_MODE)
        {
#ifdef _DEBUG
            // 1. 基本情報の準備
            float viewRange = guardComp.viewRange;
            float halfAngleRad = DirectX::XMConvertToRadians(guardComp.viewAngle * 0.5f);
            float currentYawRad = DirectX::XMConvertToRadians(guardTransform.rotation.y);

            // 始点（足元より少し上）
            DirectX::XMFLOAT3 startPos = guardTransform.position;
            startPos.y += 0.5f;

            // 色の設定
            DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 赤
            if (IsTargetInSight(guardTransform, guardComp, playerTransform))
            {
                color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 黄色
            }

            // 2. 扇の描画（分割して線を引く）
            // 左端の角度
            float startAngle = currentYawRad - halfAngleRad;
            // 右端の角度
            float endAngle = currentYawRad + halfAngleRad;

            // 分割数（この数が多いほど滑らかな円になります）
            const int SEGMENTS = 16;
            float stepAngle = (endAngle - startAngle) / SEGMENTS;

            // 左端の点を計算
            DirectX::XMFLOAT3 prevPos = {
                startPos.x + std::sin(startAngle) * viewRange,
                startPos.y,
                startPos.z + std::cos(startAngle) * viewRange
            };

            // 中心から左端への線
            Geometory::AddLine(startPos, prevPos, color);

            // 弧を描くループ
            for (int i = 1; i <= SEGMENTS; ++i)
            {
                float angle = startAngle + (stepAngle * i);

                // 次の点の座標
                DirectX::XMFLOAT3 nextPos = {
                    startPos.x + std::sin(angle) * viewRange,
                    startPos.y,
                    startPos.z + std::cos(angle) * viewRange
                };

                // 前の点から次の点へ線を引く（これで円弧になる）
                Geometory::AddLine(prevPos, nextPos, color);

                prevPos = nextPos;
            }

            // 右端から中心への線（最後に閉じる）
            Geometory::AddLine(startPos, prevPos, color);
#endif

            // -------------------------------------------------------------
            // 視界判定とゲームオーバー処理
            // -------------------------------------------------------------
            if (IsTargetInSight(guardTransform, guardComp, playerTransform))
            {
                // --- プレイヤー発見時の処理 ---

                // 1. 発見音/接触音の再生
                ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST5");

                // 2. ゲームオーバーフラグを立てる (CollisionSystemと同じ処理)
                gameStateComp.isGameOver = true;

                // 3. 以降の処理を行わず、即座に関数を抜ける
                // (これによりGameFlowSystemが次のフレームでリスタート処理を行います)
                return;
            }
        }

        XMINT2 guardGridPos = GetGridPosition(guardTransform.position, mapComp);

        // ------------------------------------------------------------------
        // 1. 目標到達判定とパス再計算の必要性チェック
        // ------------------------------------------------------------------
        bool needsPathRecalc = !guardComp.isPathCalculated;

        // 目標が有効なワールド座標として設定されているかチェック (GuardComponent::targetGridPosはXZ平面の座標)
        if (guardComp.isPathCalculated)
        {
            // 現在位置と目標位置のXZ平面の距離を計算
            XMVECTOR currentPos = XMLoadFloat3(&guardTransform.position);
            XMVECTOR targetPos = XMVectorSet(guardComp.targetGridPos.x, guardTransform.position.y, guardComp.targetGridPos.y, 1.0f); // XZをX, Zに設定

            // 距離の2乗を計算 (平方根を避けて高速化)
            float distanceSq = XMVectorGetX(XMVector3LengthSq(currentPos - targetPos));

            // 目標グリッドへの到達判定閾値 (TILE_SIZE=2.0f の0.1倍程度)
            constexpr float ARRIVAL_THRESHOLD_SQ = 0.05f * 0.05f; // 0.05m以内を到達と見なす

            if (distanceSq < ARRIVAL_THRESHOLD_SQ)
            {
                // 目標地点に到達したと見なし、次のフレームで再計算が必要
                needsPathRecalc = true;
                guardComp.isPathCalculated = false; // パスを無効化
                guardRigidBody.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f); // 一旦停止
            }
        }

        // ------------------------------------------------------------------
        // 2. パス再計算の実行
        // ------------------------------------------------------------------
        // パス再計算が必要、または、グリッドが変わった場合（移動が遅い場合などに備え、ここでは単純化のため到達判定に任せる）
        if (needsPathRecalc)
        {
            // A*探索を実行し、次の目標グリッド座標を取得
            XMINT2 nextTarget = FindNextTargetGridPos(
                guardGridPos,
                playerGridPos,
                mapComp
            );

            const float MAP_CENTER_OFFSET_X = (mapComp.gridSizeX / 2.0f) * mapComp.tileSize; // 20.0f
            const float MAP_CENTER_OFFSET_Y = (mapComp.gridSizeY / 2.0f) * mapComp.tileSize;
            const float X_ADJUSTMENT = 0.5f * mapComp.tileSize; // 1.0f
            const float Z_ADJUSTMENT = 1.0f * mapComp.tileSize; // 2.0f

            // 探索結果をGuardComponentに保存 (ステップ2-2のロジックを再利用)
            guardComp.nextTargetGridPos = nextTarget;

            if (nextTarget.x != -1) {
                // MapGenerationSystem::GetWorldPosition の逆算を避け、
                // MapGenerationSystem のGetWorldPositionが返すセルの角座標 + TILE_SIZE/2.0f が中心と仮定

                // 【注意】ここでは MapGenerationSystem::GetWorldPosition が直接利用できないため、
                // ワールド座標の再計算ロジック（GetGridPositionの逆）を一時的に使用します。

                // X座標: (グリッドX * TILE_SIZE - MAP_CENTER_OFFSET + X_ADJUSTMENT) + TILE_SIZE/2.0f
                float targetWorldX = (float)nextTarget.x * mapComp.tileSize - MAP_CENTER_OFFSET_X + X_ADJUSTMENT + mapComp.tileSize / 2.0f;
                // Z座標: (グリッドY * TILE_SIZE - MAP_CENTER_OFFSET + Z_ADJUSTMENT) + TILE_SIZE/2.0f
                float targetWorldZ = (float)nextTarget.y * mapComp.tileSize - MAP_CENTER_OFFSET_Y + Z_ADJUSTMENT + mapComp.tileSize / 2.0f;

                guardComp.targetGridPos.x = targetWorldX; // ワールドX
                guardComp.targetGridPos.y = targetWorldZ; // ワールドZ (ComponentではYと命名)

                guardComp.isPathCalculated = true;
            }
            else {
                guardComp.isPathCalculated = false;
            }
        }

        // ------------------------------------------------------------------
        // 3. 移動ロジック (ステップ3-2の内容を統合)
        // ------------------------------------------------------------------
        if (guardComp.isPathCalculated)
        {
            // 目標座標へ向かう方向ベクトルを計算
            XMVECTOR currentPos = XMLoadFloat3(&guardTransform.position);
            XMVECTOR targetPos = XMVectorSet(guardComp.targetGridPos.x, guardTransform.position.y, guardComp.targetGridPos.y, 1.0f);

            // 方向ベクトル
            XMVECTOR direction = targetPos - currentPos;

            // Y軸（高さ）方向の移動は無視し、XZ平面の移動のみ考慮
            direction = XMVectorSetY(direction, 0.0f);

            // 正規化
            XMVECTOR normalizedDirection = XMVector3Normalize(direction);

            // 速度を設定 (GuardComponent::speed を利用)
            XMVECTOR velocity = normalizedDirection * guardComp.speed;

            // RigidBodyComponentに反映
            XMStoreFloat3(&guardRigidBody.velocity, velocity);

            // 向きの更新（Z軸方向の回転を計算）
            // 方向ベクトルをXZ平面の回転角度に変換
            float angle = std::atan2(XMVectorGetX(normalizedDirection), XMVectorGetZ(normalizedDirection));
            guardTransform.rotation.y = XMConvertToDegrees(angle);
        }
    }
}