#ifndef ___MOVEMENTSYSTEM_H___
#define ___MOVEMENTSYSTEM_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include "ECS/Component/Transform.h"

// ECSコアへの依存は、System基底クラス(Types.h)とComponentのみに限定

/**
 * @class MovementSystem
 * @brief Entityに速度を適用し、位置を更新するシステム
 * @note このシステムは Transform Component を持つ全てのEntityを処理対象とします。
 */
class MovementSystem : public System
{
public:
    // Updateは純粋仮想関数なので必ず実装
    void Update(float deltaTime) override
    {
        // ECSでは、Coordinatorから渡された「処理対象のEntity ID」の集合 (this->entities) をループする
        for (const Entity entity : *entities)
        {
            // CoordinatorからTransform Componentを取得（ここでは仮のCoordinatorとして静的なグローバル変数が必要）
            // 実際の使用ではCoordinatorの参照を渡すか、グローバルなCoordinatorを利用します。

            // ★重要★ ここではCoordinatorが未統合のため、擬似的に動作を記述します。
            // 実際は Coordinator::GetComponent<Transform>(entity) のような呼び出しが必要です。

            // Transform* t = g_Coordinator->GetComponent<Transform>(entity); // 実際のコード

            // 処理例: 全EntityをY軸方向に移動させる（デモ用）
            // if (t) 
            // {
            //     t->position.y += 1.0f * deltaTime; 
            // }

            // 可読性の高いコメントを記述
            // ----------------------------------------------------------------------
            // NOTE: このSystemをCoordinatorに登録する際、
            //       Coordinator::SetSystemSignature<MovementSystem>(signature); 
            //       にて、MovementSystemの処理対象Component Signatureを設定する必要があります。
            // ----------------------------------------------------------------------
        }
    }
};

#endif // !___MOVEMENTSYSTEM_H___