#include "ECS/Systems/Core/TitleControlSystem.h"
#include "ECS/ECS.h"
#include "Systems/Input.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h" // 遷移先

using namespace ECS;
using namespace DirectX;

void TitleControlSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& ctrl = m_coordinator->GetComponent<TitleControllerComponent>(entity);

        switch (ctrl.state)
        {
        case TitleState::WaitInput:
            // 入力判定
            if (IsKeyTrigger(VK_RETURN) || IsButtonTriggered(BUTTON_A))
            {
                // 状態遷移 -> アニメーション開始
                ctrl.state = TitleState::ZoomAnimation;
                ctrl.animTimer = 0.0f;

                // PressStart UIを非表示にする（透明化）
                for (auto uiEntity : ctrl.pressStartUIEntities) {
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                        auto& img = m_coordinator->GetComponent<UIImageComponent>(uiEntity).isVisible = false;
                    }
                }
            }
            else if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_B))
            {
                PostQuitMessage(0);
            }
            break;

        case TitleState::ZoomAnimation:
        {
            ctrl.animTimer += deltaTime;
            float t = ctrl.animTimer / ctrl.animDuration;

            if (t >= 1.0f)
            {
                t = 1.0f;
                // 完了 -> モード選択へ
                ctrl.state = TitleState::ModeSelect;

                // メニューUIのボタン機能を有効化
                for (auto uiEntity : ctrl.menuUIEntities) {
                    if (m_coordinator->HasComponent<UIButtonComponent>(uiEntity)) {
                        m_coordinator->GetComponent<UIButtonComponent>(uiEntity).isVisible = true;
                    }
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                        m_coordinator->GetComponent<UIImageComponent>(uiEntity).isVisible = true;
                    }
                }
            }

            // イージング (SmoothStep)
            float smoothT = t * t * (3.0f - 2.0f * t);

            // カメラ位置更新
            if (ctrl.cameraEntityID != INVALID_ENTITY_ID)
            {
                auto& trans = m_coordinator->GetComponent<TransformComponent>(ctrl.cameraEntityID);
                trans.position.x = ctrl.camStartPos.x + (ctrl.camEndPos.x - ctrl.camStartPos.x) * smoothT;
                trans.position.y = ctrl.camStartPos.y + (ctrl.camEndPos.y - ctrl.camStartPos.y) * smoothT;
                trans.position.z = ctrl.camStartPos.z + (ctrl.camEndPos.z - ctrl.camStartPos.z) * smoothT;
            }
        }
        break;

        case TitleState::ModeSelect:
            // ここでの処理は不要。
            // UIInputSystemとUICursorSystemがボタンクリックを処理し、
            // ボタンのonClickコールバックでシーン遷移が発生する。

            // キャンセル処理（WaitInputに戻るなど）を入れる場合はここに記述
            if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_B))
            {
                // リセット処理など（省略）
            }
            break;
        }
    }
}