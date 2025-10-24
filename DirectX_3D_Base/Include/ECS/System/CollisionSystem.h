/*****************************************************************//**
 * @file	CollisionSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLISIONSYSTEM_H___
#define ___COLLISIONSYSTEM_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include "ECS/Coordinator.h"
// Componentの依存
#include "ECS/Component/Transform.h"
#include "ECS/Component/Collider.h"

// 外部システムへの依存
#include "Systems/Geometory.h" // 既存の衝突判定ヘルパー関数を想定
#include <DirectXMath.h>
#include <algorithm> // std::min, std::max用

// Coordinatorのグローバル参照
extern Coordinator* g_Coordinator;

// --------------------------------------------------
// ヘルパー構造体: World AABB (軸並行バウンディングボックス)
// --------------------------------------------------
// 衝突判定ロジックをGeometory.hに渡すためのワールド座標系データ構造
struct WorldAABB
{
    DirectX::XMFLOAT3 min = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 max = { 0.0f, 0.0f, 0.0f };
};

/**
 * @class CollisionSystem
 * @brief TransformとColliderを持つEntity間の衝突判定を実行するシステム
 * @note 物理的な応答（押し戻しなど）は、このシステムの後に実行される別のPhysicsSystemが担当すべきです。
 */
class CollisionSystem
    : public System
{
public:
    // Updateは純粋仮想関数なので必ず実装
    void Update(float deltaTime) override
    {
        if (entities->empty()) return;

        // --------------------------------------------------
        // 1. 全てのColliderの衝突フラグをリセット
        // --------------------------------------------------
        for (const Entity entity : *entities)
        {
            g_Coordinator->GetComponent<Collider>(entity).collidedThisFrame = false;
        }

        // --------------------------------------------------
        // 2. 全てのEntityペアに対してブルートフォース衝突判定を実行
        // --------------------------------------------------

        // Entity IDのセットをイテレータを使って操作
        auto itA = entities->begin();
        for (size_t i = 0; i < entities->size(); ++i, ++itA)
        {
            Entity entityA = *itA;
            Transform& tA = g_Coordinator->GetComponent<Transform>(entityA);
            Collider& cA = g_Coordinator->GetComponent<Collider>(entityA);

            // 自身の後のEntityのみをチェック (A vs B, B vs Aの重複チェックを避ける)
            auto itB = std::next(itA);
            for (size_t j = i + 1; j < entities->size(); ++j, ++itB)
            {
                Entity entityB = *itB;
                Transform& tB = g_Coordinator->GetComponent<Transform>(entityB);
                Collider& cB = g_Coordinator->GetComponent<Collider>(entityB);

                // **Step 2-a: 事前判定 (レイヤー/トリガー)**
                if (cA.isKinematic && cB.isKinematic) continue; // 両方キネマティックなら物理的な衝突を無視

                // **Step 2-b: ワールド空間でのAABBを計算**
                // Colliderのlocal extentとTransformのworld positionを組み合わせてAABBを作成 (回転は無視する簡易版)
                WorldAABB boxA;
                boxA.min.x = tA.position.x + cA.center.x - cA.extent.x;
                boxA.max.x = tA.position.x + cA.center.x + cA.extent.x;
                boxA.min.y = tA.position.y + cA.center.y - cA.extent.y;
                boxA.max.y = tA.position.y + cA.center.y + cA.extent.y;
                boxA.min.z = tA.position.z + cA.center.z - cA.extent.z;
                boxA.max.z = tA.position.z + cA.center.z + cA.extent.z;

                WorldAABB boxB; // Entity Bも同様に計算 (省略)

                // --------------------------------------------------
                // **Step 2-c: 実際の衝突判定ロジックの呼び出し**
                // --------------------------------------------------

                // ※ 既存のGeometory::CheckAABBCollision(AABB_A, AABB_B)という関数が存在すると仮定
                // bool hit = Geometory::CheckAABBCollision(boxA, boxB); // 擬似コード

                // ここでは、衝突判定が成功したと仮定してフラグを設定します
                bool hit = false;
                // AABB衝突のロジックを直接記述
                if (
                    boxA.min.x <= boxB.max.x && boxA.max.x >= boxB.min.x &&
                    boxA.min.y <= boxB.max.y && boxA.max.y >= boxB.min.y &&
                    boxA.min.z <= boxB.max.z && boxA.max.z >= boxB.min.z
                    )
                {
                    hit = true;
                }

                if (hit)
                {
                    // **Step 2-d: 結果の書き込み**
                    // 衝突が発生したことをComponentに記録
                    cA.collidedThisFrame = true;
                    cB.collidedThisFrame = true;

                    // ※ 実際のゲームでは、ここで衝突イベントを発生させる必要があります
                }
            }
        }
    }

    // CollisionSystemはロジックシステムなのでInitializeは省略可能
};

#endif // !___COLLISIONSYSTEM_H___