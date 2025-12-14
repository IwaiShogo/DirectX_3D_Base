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
#include "ECS/ECS.h"
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
// A* 探索: ゴールまでの全経路を返す
// --------------------------------------------------------------------------------
std::vector<XMINT2> GuardAISystem::FindPath(XMINT2 startGrid, XMINT2 targetGrid, const MapComponent& mapComp)
{
    // ... (境界チェックなどは既存と同じ) ...
    const int MAX_INDEX_X = mapComp.gridSizeX - 1;
    const int MAX_INDEX_Y = mapComp.gridSizeY - 1;
    if (startGrid.x <= 0 || startGrid.x >= MAX_INDEX_X || startGrid.y <= 0 || startGrid.y >= MAX_INDEX_Y ||
        targetGrid.x <= 0 || targetGrid.x >= MAX_INDEX_X || targetGrid.y <= 0 || targetGrid.y >= MAX_INDEX_Y)
    {
        return {}; // 空のパス
    }

    // A*のセットアップ
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
    std::map<XMINT2, AStarNode, XMINT2Comparer> allNodes;

    AStarNode startNode;
    startNode.gridPos = startGrid;
    startNode.gCost = 0.0f;
    startNode.hCost = (float)std::abs(targetGrid.x - startGrid.x) + std::abs(targetGrid.y - startGrid.y);
    startNode.fCost = startNode.hCost;
    startNode.parentPos = startGrid; // 親は自分

    openSet.push(startNode);
    allNodes[startGrid] = startNode;

    while (!openSet.empty())
    {
        AStarNode current = openSet.top();
        openSet.pop();

        if (current.fCost > allNodes.at(current.gridPos).fCost) continue;

        // ゴール到達
        if (current.gridPos == targetGrid)
        {
            // --- 経路復元 ---
            std::vector<XMINT2> path;
            XMINT2 curr = targetGrid;
            while (!(curr == startGrid))
            {
                path.push_back(curr);
                curr = allNodes.at(curr).parentPos;
            }
            // path.push_back(startGrid); // スタート地点は含めなくて良い（現在地なので）
            std::reverse(path.begin(), path.end()); // ゴール→スタート順なので反転
            return path;
        }

        // 隣接ノード探索
        int directions[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
        for (int i = 0; i < 4; ++i)
        {
            XMINT2 neighborPos = { current.gridPos.x + directions[i][0], current.gridPos.y + directions[i][1] };

            // 壁チェック
            if (neighborPos.x <= 0 || neighborPos.x >= mapComp.gridSizeX - 1 ||
                neighborPos.y <= 0 || neighborPos.y >= mapComp.gridSizeY - 1) continue;

            CellType type = mapComp.grid[neighborPos.y][neighborPos.x].type;
            if (type == CellType::Wall || type == CellType::Unvisited) continue;

            float newGCost = current.gCost + 1.0f;

            if (allNodes.find(neighborPos) == allNodes.end() || newGCost < allNodes.at(neighborPos).gCost)
            {
                AStarNode neighborNode;
                neighborNode.gridPos = neighborPos;
                neighborNode.gCost = newGCost;
                neighborNode.hCost = (float)std::abs(targetGrid.x - neighborPos.x) + std::abs(targetGrid.y - neighborPos.y);
                neighborNode.fCost = neighborNode.gCost + neighborNode.hCost;
                neighborNode.parentPos = current.gridPos;

                allNodes[neighborPos] = neighborNode;
                openSet.push(neighborNode);
            }
        }
    }
    return {}; // 見つからなかった
}

// --------------------------------------------------------------------------------
// パススムージング (String Pulling)
// --------------------------------------------------------------------------------
std::vector<XMFLOAT3> GuardAISystem::SmoothPath(const std::vector<XMINT2>& gridPath, const MapComponent& mapComp)
{
    if (gridPath.empty()) return {};

    std::vector<XMFLOAT3> worldPath;

    // グリッド座標 -> ワールド座標への変換ヘルパー
    auto ToWorld = [&](XMINT2 g) -> XMFLOAT3 {
        // MapGenerationSystem::GetWorldPositionの簡易版
        // グリッドの中心座標を返す
        float x = (float)g.x * mapComp.tileSize - ((mapComp.gridSizeX / 2.0f) * mapComp.tileSize) + 0.5f * mapComp.tileSize + mapComp.tileSize / 2.0f;
        float z = (float)g.y * mapComp.tileSize - ((mapComp.gridSizeY / 2.0f) * mapComp.tileSize) + 1.0f * mapComp.tileSize + mapComp.tileSize / 2.0f;
        return XMFLOAT3(x, 0.0f, z);
        };

    // 最初のポイントを追加
    worldPath.push_back(ToWorld(gridPath[0]));

    // チェック開始点
    XMFLOAT3 currentCheckPoint = worldPath[0];
    int checkIndex = 0;

    // パスを間引く
    for (size_t i = 1; i < gridPath.size(); ++i)
    {
        XMFLOAT3 nextPoint = ToWorld(gridPath[i]);

        // 現在のチェック点から、次の次の点へ直接行けるか確認
        // 壁にぶつかるなら、一つ前の点（i-1）を経由地点として確定する
        if (RaycastHitWall(currentCheckPoint, nextPoint, mapComp))
        {
            // 壁に当たった -> i-1番目の点は必須
            // (ただしi-1がcurrentCheckPointと同じならスキップ)
            if (i - 1 > checkIndex) {
                XMFLOAT3 wayPoint = ToWorld(gridPath[i - 1]);
                worldPath.push_back(wayPoint);
                currentCheckPoint = wayPoint;
                checkIndex = (int)i - 1;
            }
        }
    }

    // 最後のゴール地点は必ず追加
    worldPath.push_back(ToWorld(gridPath.back()));

    return worldPath;
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

// --------------------------------------------------------------------------------
// ヘルパー関数: 2点間のレイキャストで壁があるかチェック
// --------------------------------------------------------------------------------
bool GuardAISystem::RaycastHitWall(
    const DirectX::XMFLOAT3& start,
    const DirectX::XMFLOAT3& end,
    const MapComponent& mapComp
)
{
    XMVECTOR startV = XMLoadFloat3(&start);
    XMVECTOR endV = XMLoadFloat3(&end);
    XMVECTOR dirV = endV - startV;
    float dist = XMVectorGetX(XMVector3Length(dirV));

    if (dist < 0.001f) return false; // 同じ位置なら壁はないとみなす

    dirV = XMVector3Normalize(dirV);

    // レイキャストのステップ幅 (タイルの半分程度)
    float stepSize = mapComp.tileSize * 0.5f;
    int steps = static_cast<int>(dist / stepSize);

    for (int i = 0; i <= steps; ++i)
    {
        XMVECTOR currentPosV = startV + dirV * (float)i * stepSize;
        XMFLOAT3 currentPos;
        XMStoreFloat3(&currentPos, currentPosV);

        XMINT2 gridPos = GetGridPosition(currentPos, mapComp);

        // グリッド範囲内チェック
        if (gridPos.x >= 0 && gridPos.x < mapComp.gridSizeX &&
            gridPos.y >= 0 && gridPos.y < mapComp.gridSizeY)
        {
            if (mapComp.grid[gridPos.y][gridPos.x].type == CellType::Wall)
            {
                return true; // 壁にヒット
            }
        }
    }

    return false; // 壁にヒットしなかった
}

// --------------------------------------------------------------------------------
// ターゲットが視界内か判定 (距離、角度、遮蔽物)
// --------------------------------------------------------------------------------
bool GuardAISystem::IsTargetInSight(
    const TransformComponent& guardTransform,
    const GuardComponent& guardInfo,
    const TransformComponent& targetTransform,
    const MapComponent& mapComp
)
{
    // 1. 距離判定
    XMVECTOR guardPos = XMLoadFloat3(&guardTransform.position);
    XMVECTOR targetPos = XMLoadFloat3(&targetTransform.position);
    XMVECTOR toTarget = targetPos - guardPos;

    // 高さ無視
    toTarget = XMVectorSetY(toTarget, 0.0f);

    XMVECTOR distSqVec = XMVector3LengthSq(toTarget);
    float distSq;
    XMStoreFloat(&distSq, distSqVec);

    if (distSq > guardInfo.viewRange * guardInfo.viewRange)
    {
        return false; // 範囲外
    }

    // 2. 角度判定
    XMVECTOR toTargetDir = XMVector3Normalize(toTarget);
    float yawRad = guardTransform.rotation.y;

    float dirX = std::sin(yawRad);
    float dirZ = std::cos(yawRad);
    XMVECTOR forwardDir = XMVectorSet(dirX, 0.0f, dirZ, 0.0f);

    XMVECTOR dotVec = XMVector3Dot(forwardDir, toTargetDir);
    float dot;
    XMStoreFloat(&dot, dotVec);

    float angleThreshold = std::cos(XMConvertToRadians(guardInfo.viewAngle * 0.5f));

    if (dot < angleThreshold)
    {
        return false; // 視野角外
    }

    // 3. 遮蔽物判定 (Raycast)
    // 自身の位置(少し上)からターゲットの位置(少し上)へレイを飛ばす
    XMFLOAT3 rayStart = guardTransform.position;
    rayStart.y += 0.5f; // 目の高さ
    XMFLOAT3 rayEnd = targetTransform.position;
    rayEnd.y += 0.5f;

    if (RaycastHitWall(rayStart, rayEnd, mapComp))
    {
        return false; // 壁に遮られた
    }

    return true; // 全ての条件をクリア
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
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;

    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
    auto& pEffect = m_coordinator->GetComponent<EffectComponent>(playerID);

    // 2. 「近くに警備員がいるか？」フラグをリセット
    bool isAnyGuardClose = false;
    float alertDistance = 15.0f; // 10m以内なら反応

    // 3. 全ての警備員をチェック
    for (auto const& entity : m_entities)
    {
        // 警備員タグまたはコンポーネントを持つエンティティのみ対象
        if (!m_coordinator->HasComponent<GuardComponent>(entity)) continue;

        auto& gTrans = m_coordinator->GetComponent<TransformComponent>(entity);

        // 距離計算
        float dx = pTrans.position.x - gTrans.position.x;
        float dy = pTrans.position.y - gTrans.position.y;
        float dz = pTrans.position.z - gTrans.position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        // もし1体でも近くにいたらフラグを立ててループ終了
        if (distSq < alertDistance * alertDistance)
        {
            isAnyGuardClose = true;
            break;
        }
    }


    // 4. フラグに基づいてプレイヤーのエフェクトを制御
    if (isAnyGuardClose)
    {
        // 危険なのに、まだ再生していなければ -> 再生
        if (pEffect.handle == 0)
        {
            pEffect.requestPlay = true;
        }
    }
    else
    {
        // 安全なのに、まだ再生中なら -> 停止
        // (少し余裕を持たせたい場合は、ここでの判定距離を12mなどにする手もあります)
        if (pEffect.handle != 0)
        {
            pEffect.requestStop = true;
        }
    }

    XMINT2 playerGridPos = GetGridPosition(pTrans.position, mapComp);

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
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_FAINDPLAYER");

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
            float currentYawRad = guardTransform.rotation.y;

            // 始点（足元より少し上）
            DirectX::XMFLOAT3 startPos = guardTransform.position;
            startPos.y += 0.5f;

            // 色の設定
            DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 赤
            if (IsTargetInSight(guardTransform, guardComp, pTrans, mapComp))
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
            if (IsTargetInSight(guardTransform, guardComp, pTrans, mapComp))
            {
                // --- プレイヤー発見時の処理 ---

                // 1. 発見音/接触音の再生
                //ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST5");

                // 2. ゲームオーバーフラグを立てる (CollisionSystemと同じ処理)
                if (auto gameCtrl = ECS::ECSInitializer::GetSystem<GameControlSystem>())
                {
                    gameCtrl->TriggerCaughtSequence(entity);
                }

                // 3. 以降の処理を行わず、即座に関数を抜ける
                // (これによりGameControlSystemが次のフレームでリスタート処理を行います)
                return;
            }
        }

        XMINT2 guardGridPos = GetGridPosition(guardTransform.position, mapComp);

        // ------------------------------------------------------------------
        // 1. 目標到達判定とパス再計算の必要性チェック
        // ------------------------------------------------------------------
        // --- プレイヤー追跡ロジック ---
        if (guardComp.isActive && gameStateComp.currentMode == GameMode::ACTION_MODE)
        {
            // 1. パスの再計算 (一定時間ごと)
            guardComp.pathRecalcTimer -= deltaTime;
            if (guardComp.pathRecalcTimer <= 0.0f)
            {
                guardComp.pathRecalcTimer = guardComp.PATH_RECALC_INTERVAL;

                XMINT2 guardGrid = GetGridPosition(guardTransform.position, mapComp);
                XMINT2 playerGrid = GetGridPosition(pTrans.position, mapComp);

                // A* で全経路取得
                std::vector<XMINT2> rawPath = FindPath(guardGrid, playerGrid, mapComp);

                // スムージング
                guardComp.path = SmoothPath(rawPath, mapComp);
                guardComp.currentPathIndex = 0;
            }

            // 2. パス追従 (Steering: Seek)
            if (!guardComp.path.empty() && guardComp.currentPathIndex < guardComp.path.size())
            {
                XMFLOAT3 targetPos = guardComp.path[guardComp.currentPathIndex];

                // 現在地からターゲットへのベクトル
                XMVECTOR myPos = XMLoadFloat3(&guardTransform.position);
                XMVECTOR targetV = XMLoadFloat3(&targetPos);
                XMVECTOR toTarget = targetV - myPos;
                toTarget = XMVectorSetY(toTarget, 0.0f); // Y軸無視

                float distSq = XMVectorGetX(XMVector3LengthSq(toTarget));

                // ウェイポイント到達判定 (半径1.0m以内なら次へ)
                // ゴール地点(最後)の場合は厳密に判定
                float threshold = (guardComp.currentPathIndex == guardComp.path.size() - 1) ? 0.5f : 1.0f;

                if (distSq < threshold * threshold)
                {
                    // 次のポイントへ
                    guardComp.currentPathIndex++;
                    // まだパスが残っているなら、今の速度を維持してスムーズに移行したいので velocity はゼロにしない
                }
                else
                {
                    // --- ステアリング移動 ---
                    XMVECTOR desiredDir = XMVector3Normalize(toTarget);

                    // 加速・旋回処理
                    // 現在の速度ベクトル
                    XMVECTOR currentVel = XMLoadFloat3(&guardRigidBody.velocity);

                    // 簡易ステアリング: 即座に方向を変えず、補間する
                    // Lerpで「現在の進行方向」から「行きたい方向」へ徐々に近づける
                    // 係数を調整することで旋回性能が変わる (0.1f くらいが鈍くて人間っぽい)
                    XMVECTOR newVel = XMVectorLerp(currentVel, desiredDir * guardComp.speed, 5.0f * deltaTime);

                    // 速度更新
                    XMStoreFloat3(&guardRigidBody.velocity, newVel);

                    // --- 向きの更新 ---
                    // 移動方向を向く
                    if (distSq > 0.01f)
                    {
                        // 2. 「速度」ではなく「目的地への方向」を使って角度を計算する
                        XMVECTOR dirVec = XMLoadFloat3(&targetPos) - XMLoadFloat3(&guardTransform.position);
                        dirVec = XMVectorSetY(dirVec, 0.0f); // 高さは無視
                        dirVec = XMVector3Normalize(dirVec);

                        // atan2 の結果はラジアン
                        float targetAngle = std::atan2(XMVectorGetX(dirVec), XMVectorGetZ(dirVec));

                        // ★重要: Transformにはラジアンが入っている前提なので、変換せずそのまま使う
                        float currentAngle = guardTransform.rotation.y;

                        // 角度差の計算
                        float diff = targetAngle - currentAngle;

                        // -PI ~ PI に補正 (最短経路で回転させる)
                        while (diff <= -XM_PI) diff += XM_2PI;
                        while (diff > XM_PI) diff -= XM_2PI;

                        // 少しずつ回転 (補間)
                        float rotSpeed = 5.0f * deltaTime;

                        // デッドゾーン (差がごく僅かなら回転しない)
                        if (std::abs(diff) > 0.001f)
                        {
                            if (std::abs(diff) < rotSpeed) {
                                currentAngle = targetAngle;
                            }
                            else {
                                currentAngle += (diff > 0) ? rotSpeed : -rotSpeed;
                            }

                            // ★重要: ラジアンのまま保存する (ConvertToDegreesを削除)
                            guardTransform.rotation.y = currentAngle;
                        }
                    }
                }
            }
            else
            {
                // パスがない、または終了 -> 停止
                guardRigidBody.velocity = { 0,0,0 };
            }
        }
    }
}