#ifndef ___MOVEMENTSYSTEM_H___
#define ___MOVEMENTSYSTEM_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/Input.h"

// ECSコアへの依存は、System基底クラス(Types.h)とComponentのみに限定

// Coordinatorのグローバル参照 (GameScene.cppでポインターとして定義済み)
extern Coordinator* g_pCoordinator;

/**
 * @class MovementSystem
 * @brief Entityに速度を適用し、位置を更新するシステム
 * @note このシステムは Transform Component を持つ全てのEntityを処理対象とします。
 */
class MovementSystem 
    : public System
{
public:
    // Updateは純粋仮想関数なので必ず実装
    void Update(float deltaTime) override
    {
        // ECSでは、Coordinatorから渡された「処理対象のEntity ID」の集合 (this->entities) をループする
        for (const Entity entity : *entities)
        {
            // 処理対象のEntity IDの集合 (this->entities) をループする
            for (const Entity entity : *entities)
            {
                // Coordinatorを通じてComponentデータを取得
                // Transformは書き込み (in/out)
                Transform& t = g_pCoordinator->GetComponent<Transform>(entity);
                // Inputは読み取り (in)
                Input& input = g_pCoordinator->GetComponent<Input>(entity);

                // --------------------------------------------------
                // 1. 移動ベクトルの適用
                // --------------------------------------------------

                // Input Componentから移動ベクトルを取得
                DirectX::XMVECTOR moveVec = DirectX::XMLoadFloat3(&input.movementVector);

                // 移動ベクトルがゼロではないかチェック
                if (DirectX::XMVector3LengthSq(moveVec).m128_f32[0] > 0.0f)
                {
                    // 移動量 = 移動ベクトル * 速度 * デルタタイム
                    float scalar = MOVEMENT_SPEED * deltaTime;
                    DirectX::XMVECTOR deltaPos = DirectX::XMVectorScale(moveVec, scalar);

                    // 現在の位置に移動量を加算
                    DirectX::XMVECTOR currentPos = DirectX::XMLoadFloat3(&t.position);
                    DirectX::XMVECTOR newPos = DirectX::XMVectorAdd(currentPos, deltaPos);

                    // 結果をTransform Componentに書き戻す
                    DirectX::XMStoreFloat3(&t.position, newPos);
                }

                // --------------------------------------------------
                // 2. 入力データの消費 (次のフレームのためにリセット)
                // --------------------------------------------------

                // MovementSystemが処理を終えた後、Input Componentのデータをリセットします。
                // これにより、キーを押していないフレームでは移動が停止します。
                if (input.shouldResetAfterUse)
                {
                    input.movementVector = { 0.0f, 0.0f, 0.0f };
                    input.isMovingForward = false;
                    input.isMovingBackward = false;
                    input.isMovingLeft = false;
                    input.isMovingRight = false;
                    input.isJumpPressed = false;
                    // マウス入力は通常、InputSystem側でリセットするか、MovementSystem内で使用後にリセットします
                    // 今回はInputSystem側でリセットロジックを実装しなかったため、MovementSystem側で一旦処理済みとしてフラグを倒すだけでも良い。
                }
            }
        }
    }

private:
    // ゲームの移動速度
    const float MOVEMENT_SPEED = 15.0f;
};

#endif // !___MOVEMENTSYSTEM_H___