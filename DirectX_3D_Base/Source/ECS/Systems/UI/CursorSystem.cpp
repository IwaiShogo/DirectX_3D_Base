#include "ECS/Systems/UI/CursorSystem.h" 
#include "ECS/ECS.h"
#include "Systems/Input.h"
#include "Main.h"

void CursorSystem::Update(float deltaTime)
{
    if (!m_coordinator) return;

    for (auto const& entity : m_entities)
    {
        auto& cursor = m_coordinator->GetComponent<UICursorComponent>(entity);
        auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

        // --- 1. トリガー状態の更新 ---
        // マウス左クリック(0) または パッドのAボタン
        // ※Inputクラスの仕様に合わせて調整してください
        bool isMouseClick = IsMousePress(0);
        bool isPadClick = false;
        isPadClick = IsButtonTriggered(BUTTON_A); 

        cursor.isTriggered = isMouseClick || isPadClick;

        // --- 2. 位置の更新 (ピクセル座標) ---

        // マウス位置の適用
        DirectX::XMFLOAT2 mousePos = GetMousePosition();

        // 直前のフレームとマウスが動いているか判定するロジックがあれば尚良いですが、
        // ここでは簡易的にマウス位置を適用します
        transform.position.x = (float)mousePos.x;
        transform.position.y = (float)mousePos.y;

        // パッド移動 (マウス操作と競合する場合は「入力があった方」を優先する処理が必要です)
        auto stick = GetLeftStick();
        if (std::abs(stick.x) > 0.1f || std::abs(stick.y) > 0.1f)
        {
            transform.position.x += stick.x * cursor.moveSpeed;
            transform.position.y -= stick.y * cursor.moveSpeed; // Y軸反転注意
        }

        // 画面外に出ないようにクランプ
        if (transform.position.x < 0.0f) transform.position.x = 0.0f;
        if (transform.position.x > SCREEN_WIDTH) transform.position.x = (float)SCREEN_WIDTH;
        if (transform.position.y < 0.0f) transform.position.y = 0.0f;
        if (transform.position.y > SCREEN_HEIGHT) transform.position.y = (float)SCREEN_HEIGHT;
    }
}