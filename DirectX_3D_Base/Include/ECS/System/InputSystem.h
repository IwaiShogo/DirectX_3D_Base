#pragma once

#include "ECS/Types.h"
#include "ECS/Coordinator.h"
#include "ECS/Component/Input.h"

// 既存の入力処理システムへの依存
#include "Systems/Input.h" // 既存のDirectX基盤の入力ヘルパークラス (Input::IsKeyPresed()などを想定)

// Coordinatorのグローバル参照 (GameScene.cppでポインターとして定義済み)
extern Coordinator* g_pCoordinator;

/**
 * @class InputSystem
 * @brief 外部の入力状態を監視し、Input Componentにその状態を書き込むシステム
 * @note このシステムは、Input Componentを持つ全てのEntityを処理対象とします。
 */
class InputSystem : public System
{
public:
    // ゲームの入力設定（キーコードとアクションのマッピング）は、このクラスの外部または内部で設定可能

    // System基底クラスのInitializeはオーバーライドしても良いが、ここではシンプルにUpdateのみ
    void Initialize() override
    {
        // 例えば、マウスカーソルのロック/非表示などをここで行う
    }

    /**
     * @brief 毎フレームの入力状態をInput Componentに書き込む
     * @param[in] deltaTime 前フレームからの経過時間（秒）
     */
    void Update(float deltaTime) override
    {
        // 入力データを取得する前に、Entityが存在するか確認
        if (entities->empty()) return;

        // --------------------------------------------------
        // 1. 外部のキーボード/マウス入力状態を取得
        // --------------------------------------------------
        // 既存の Input.h/Input.cpp に依存
        bool keyW = IsKeyPress('W'); // 前進
        bool keyS = IsKeyPress('S'); // 後退
        bool keyA = IsKeyPress('A'); // 左移動
        bool keyD = IsKeyPress('D'); // 右移動
        bool keySpace = IsKeyPress(VK_SPACE); // ジャンプ

        // マウス移動量（ここでは擬似的に取得。実際のInputシステムから取得が必要）
        float mouseX = 0.0f; // Input::GetMouseDeltaX();
        float mouseY = 0.0f; // Input::GetMouseDeltaY();


        // --------------------------------------------------
        // 2. 処理対象のEntity（Input Componentを持つもの）に対してループ
        // --------------------------------------------------
        for (const Entity entity : *entities)
        {
            // Coordinatorを通じてInput Componentデータを取得
            // g_Coordinatorはポインターなので -> でアクセス
            Input& input = g_pCoordinator->GetComponent<Input>(entity);

            // a. 既存の値をリセット
            if (input.shouldResetAfterUse)
            {
                // 移動ベクトルは加算される可能性もあるため、ここではフラグのみリセットする設計も可能だが、
                // MovementSystemとの連携を考え、今回は入力Componentを完全に更新します。
                input.movementVector = { 0.0f, 0.0f, 0.0f };
                input.mouseDeltaX = 0.0f;
                input.mouseDeltaY = 0.0f;
            }

            // b. キー状態の書き込み
            input.isMovingForward = keyW;
            input.isMovingBackward = keyS;
            input.isMovingLeft = keyA;
            input.isMovingRight = keyD;
            input.isJumpPressed = keySpace;

            // c. 移動ベクトルの計算
            if (keyW) input.movementVector.z += 1.0f;
            if (keyS) input.movementVector.z -= 1.0f;
            if (keyA) input.movementVector.x -= 1.0f;
            if (keyD) input.movementVector.x += 1.0f;

            // 移動ベクトルを正規化（斜め移動の速度を等しくするため）
            if (DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&input.movementVector)).m128_f32[0] > 0.0f)
            {
                DirectX::XMStoreFloat3(&input.movementVector,
                    DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&input.movementVector)));
            }

            // d. マウス移動量の書き込み
            input.mouseDeltaX = mouseX;
            input.mouseDeltaY = mouseY;

            // Input Componentデータが更新されました。
        }
    }
};