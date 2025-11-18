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
    constexpr int MAX_INDEX = MAP_GRID_SIZE - 1;
    if (startGrid.x <= 0 || startGrid.x >= MAX_INDEX || startGrid.y <= 0 || startGrid.y >= MAX_INDEX ||
        targetGrid.x <= 0 || targetGrid.x >= MAX_INDEX || targetGrid.y <= 0 || targetGrid.y >= MAX_INDEX)
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
            if (neighborPos.x <= 0 || neighborPos.x >= MAP_GRID_SIZE - 1 ||
                neighborPos.y <= 0 || neighborPos.y >= MAP_GRID_SIZE - 1)
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
XMINT2 GuardAISystem::GetGridPosition(const XMFLOAT3& worldPos)
{
    // MapGenerationSystem.cpp の GetWorldPositionの逆算
    // pos.x = (float)x * TILE_SIZE - MAP_CENTER_OFFSET + X_ADJUSTMENT;
    // (pos.x - X_ADJUSTMENT + MAP_CENTER_OFFSET) / TILE_SIZE = (float)x

    float x_f = (worldPos.x - X_ADJUSTMENT + MAP_CENTER_OFFSET) / TILE_SIZE;
    float y_f = (worldPos.z - Z_ADJUSTMENT + MAP_CENTER_OFFSET) / TILE_SIZE;

    // 四捨五入ではなく、単にキャストしてグリッドインデックスを取得
    int x = static_cast<int>(x_f);
    int y = static_cast<int>(y_f);

    // 境界クランプ
    x = std::min(std::max(0, x), MAP_GRID_SIZE - 1);
    y = std::min(std::max(0, y), MAP_GRID_SIZE - 1);

    return { x, y };
}

// --------------------------------------------------------------------------------
// GuardAISystem::Update() の本体
// --------------------------------------------------------------------------------

void GuardAISystem::Update()
{
    float deltaTime = 1.0f / fFPS;

    // プレイヤーエンティティの検索 (追跡目標)
    EntityID playerEntity = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerEntity == INVALID_ENTITY_ID) return;

    // プレイヤーのTransformComponentを取得
    const TransformComponent& playerTransform = m_coordinator->GetComponent<TransformComponent>(playerEntity);
    XMINT2 playerGridPos = GetGridPosition(playerTransform.position);

    // MapComponentを持つEntity (GameController) を検索
    EntityID mapEntity = FindFirstEntityWithComponent<MapComponent>(m_coordinator);
    if (mapEntity == INVALID_ENTITY_ID) return;

    const MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);
    const GameStateComponent& gameStateComp = m_coordinator->GetComponent<GameStateComponent>(mapEntity);

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
                guardTransform.position.y = TILE_SIZE / 2.0f;

                guardComp.elapsedTime += deltaTime; // デルタタイムでタイマーを進める

                if (guardComp.elapsedTime >= guardComp.delayBeforeChase)
                {
                    // 遅延時間経過後、AI追跡を有効化
                    guardComp.isActive = true;
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

        XMINT2 guardGridPos = GetGridPosition(guardTransform.position);

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

            // 探索結果をGuardComponentに保存 (ステップ2-2のロジックを再利用)
            guardComp.nextTargetGridPos = nextTarget;

            if (nextTarget.x != -1) {
                // MapGenerationSystem::GetWorldPosition の逆算を避け、
                // MapGenerationSystem のGetWorldPositionが返すセルの角座標 + TILE_SIZE/2.0f が中心と仮定

                // 【注意】ここでは MapGenerationSystem::GetWorldPosition が直接利用できないため、
                // ワールド座標の再計算ロジック（GetGridPositionの逆）を一時的に使用します。

                // X座標: (グリッドX * TILE_SIZE - MAP_CENTER_OFFSET + X_ADJUSTMENT) + TILE_SIZE/2.0f
                float targetWorldX = (float)nextTarget.x * TILE_SIZE - MAP_CENTER_OFFSET + X_ADJUSTMENT + TILE_SIZE / 2.0f;
                // Z座標: (グリッドY * TILE_SIZE - MAP_CENTER_OFFSET + Z_ADJUSTMENT) + TILE_SIZE/2.0f
                float targetWorldZ = (float)nextTarget.y * TILE_SIZE - MAP_CENTER_OFFSET + Z_ADJUSTMENT + TILE_SIZE / 2.0f;

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