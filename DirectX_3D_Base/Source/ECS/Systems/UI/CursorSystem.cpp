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
        bool isMouseClick = IsMouseTrigger(0);
        bool isPadClick = false;
        isPadClick = IsButtonTriggered(BUTTON_A); 

        cursor.isTriggered = isMouseClick || isPadClick;

        // --- 2. 位置の更新 (ピクセル座標) ---

        DirectX::XMFLOAT2 mouseDelta = GetMouseDelta();

        // マウスによる移動 (感度は適宜調整してください、例えば 1.0f)
        transform.position.x += mouseDelta.x * 1.0f;
        transform.position.y += mouseDelta.y * 1.0f;

        // パッド(スティック)による移動
        auto stick = GetLeftStick();
        if (std::abs(stick.x) > 0.1f || std::abs(stick.y) > 0.1f)
        {
            transform.position.x += stick.x * cursor.moveSpeed;
            transform.position.y -= stick.y * cursor.moveSpeed; // Y軸反転注意
        }

        // 画面外に出ないようにクランプ
        // UIカーソルのホットスポットが左上(0,0)だと仮定した場合のクランプ
        if (transform.position.x < 0.0f) transform.position.x = 0.0f;
        if (transform.position.x > SCREEN_WIDTH) transform.position.x = (float)SCREEN_WIDTH;
        if (transform.position.y < 0.0f) transform.position.y = 0.0f;
        if (transform.position.y > SCREEN_HEIGHT) transform.position.y = (float)SCREEN_HEIGHT;
    }
}