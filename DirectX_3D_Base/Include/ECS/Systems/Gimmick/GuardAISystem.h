/*****************************************************************//**
 * @file	GuardAISystem.h
 * @brief	プレイヤーの位置を追跡する警備員AIのコンポーネント定義
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Fukudome Hiroaki
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

#ifndef ___GUARD_AI_SYSTEM_H___
#define ___GUARD_AI_SYSTEM_H___

#include "ECS/ECS.h"

/**
 * struct   AStarNode
 * @brief   A*探索で使用するノード情報
 */
struct AStarNode
{
    // マップのグリッド座標 (x, y)
    DirectX::XMINT2 gridPos;

    // 始点からの実コスト (Gコスト)
    float gCost;

    // 終点までの推定コスト (Hコスト - マンハッタン距離など)
    float hCost;

    // 総合評価コスト (F = G + H)
    float fCost;

    // 経路復元のための親ノード座標
    DirectX::XMINT2 parentPos;

    // コンストラクタ
    AStarNode()
        : gCost(std::numeric_limits<float>::max())
        , hCost(0.0f)
        , fCost(std::numeric_limits<float>::max())
        , parentPos({ -1, -1 })
    {}

    // OpenSetで使用するための比較演算子 (fCostが小さいノードを優先)
    // std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> で使用
    bool operator>(const AStarNode& other) const {
        return fCost > other.fCost;
    }

    // グリッド座標を比較するための演算子 (ClosedSet管理用)
    bool operator==(const AStarNode& other) const {
        return gridPos.x == other.gridPos.x && gridPos.y == other.gridPos.y;
    }
};

class GuardAISystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

    // ヘルパー関数の宣言を private セクションに移動
    DirectX::XMINT2 FindNextTargetGridPos(DirectX::XMINT2 startGrid, DirectX::XMINT2 targetGrid, const MapComponent& mapComp);

    DirectX::XMINT2 GetGridPosition(const DirectX::XMFLOAT3& worldPos);
public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    void Update();
};

#endif // !___GUARD_AI_SYSTEM_H___