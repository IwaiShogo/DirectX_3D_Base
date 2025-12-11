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
        {

            ctrl.animTimer += deltaTime;

            float speed = 5.0f; // 点滅速度
            float t = (std::sin(ctrl.animTimer * speed) + 1.0f) * 0.5f;
            // 0.0～1.0の範囲に正規化

            for (auto uiEntity : ctrl.pressStartUIEntities)
            {
                // サイズ変更 (TransformComponent)
                if (m_coordinator->HasComponent<TransformComponent>(uiEntity))
                {
                    auto& trans = m_coordinator->GetComponent<TransformComponent>(uiEntity);

                    // 基準サイズ (TitleScene::Initで設定した 450, 150 を基準にする)
                    // ※本来はComponentに初期値を保存するのが理想ですが、ここでは固定値で計算します
                    float baseW = 450.0f;
                    float baseH = 150.0f;

                    // 1.0倍 ? 1.1倍 の間で伸縮
                    float scaleFactor = 1.0f + (t * 0.1f);

                    trans.scale.x = baseW * scaleFactor;
                    trans.scale.y = baseH * scaleFactor;
                }

                // 透明度変更 (UIImageComponent)
                if (m_coordinator->HasComponent<UIImageComponent>(uiEntity))
                {
                    auto& img = m_coordinator->GetComponent<UIImageComponent>(uiEntity);

                    // t=0(小)のとき不透明(1.0)、t=1(大)のとき半透明(0.2)
                    // Lerp: A + (B - A) * t
                    float maxAlpha = 1.0f;
                    float minAlpha = 0.2f; // ここを0.0fにすると完全に消えます

                    img.color.w = minAlpha + (maxAlpha - minAlpha) * t;
                    
                }
            }


            // 入力判定
            if (IsKeyTrigger(VK_RETURN) || IsButtonTriggered(BUTTON_A))
            {
                // 状態遷移 -> アニメーション開始
                ctrl.state = TitleState::ZoomAnimation;
                ctrl.animTimer = 0.0f;

                // PressStart UIを非表示にする（透明化）
                for (auto uiEntity : ctrl.pressStartUIEntities)
                {
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity))
                    {
                        auto& img = m_coordinator->GetComponent<UIImageComponent>(uiEntity).isVisible = false;
                    }
                }
                if (ctrl.logoEntityID != ECS::INVALID_ENTITY_ID &&
                    m_coordinator->HasComponent<UIImageComponent>(ctrl.logoEntityID))
                {
                    m_coordinator->GetComponent<UIImageComponent>(ctrl.logoEntityID).isVisible = false;
                }
            }
            else if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_B))
            {
                PostQuitMessage(0);
            }
            break;
        }
        case TitleState::ZoomAnimation:
        {
            ctrl.animTimer += deltaTime;
            float t = ctrl.animTimer / ctrl.animDuration;

            if (t >= 1.0f)
            {
                t = 1.0f;
                // 完了 -> モード選択へ
                ctrl.state = TitleState::ModeSelect;

                ctrl.uiAnimTimer = 0.0f;

                // メニューUIのボタン機能を有効化
                for (auto uiEntity : ctrl.menuUIEntities) {
                    if (m_coordinator->HasComponent<UIButtonComponent>(uiEntity)) {
                        m_coordinator->GetComponent<UIButtonComponent>(uiEntity).isVisible = true;
                    }
                    /*  if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                          m_coordinator->GetComponent<UIImageComponent>(uiEntity).isVisible = true;
                      }*/
                }
            }

            // イージング (SmoothStep)
            float smoothT = t * t * (3.0f - 2.0f * t);

            // ベジエ曲線の計算係数
            float u = 1.0f - smoothT;
            float tt = smoothT * smoothT;
            float uu = u * u;
            float ut2 = 2.0f * u * smoothT;

            // カメラ位置更新
            if (ctrl.cameraEntityID != INVALID_ENTITY_ID)
            {
                auto& trans = m_coordinator->GetComponent<TransformComponent>(ctrl.cameraEntityID);
                trans.position.x = (uu * ctrl.camStartPos.x) + (ut2 * ctrl.camControlPos.x) + (tt * ctrl.camEndPos.x);
                trans.position.y = (uu * ctrl.camStartPos.y) + (ut2 * ctrl.camControlPos.y) + (tt * ctrl.camEndPos.y);
                trans.position.z = (uu * ctrl.camStartPos.z) + (ut2 * ctrl.camControlPos.z) + (tt * ctrl.camEndPos.z);

                trans.rotation.y = ctrl.startRotY + (ctrl.endRotY - ctrl.startRotY) * smoothT;
            }
        }
        break;

        case TitleState::ModeSelect:

            if (ctrl.uiAnimTimer < ctrl.uiAnimDuration)
            {
                ctrl.uiAnimTimer += deltaTime;
                float t = ctrl.uiAnimTimer / ctrl.uiAnimDuration;
                if (t > 1.0f) t = 1.0f;

                // イージング関数 (EaseOutQuad: ふわっと減速して止まる)
                float easeT = 1.0f - (1.0f - t) * (1.0f - t);

                for (size_t i = 0; i < ctrl.menuUIEntities.size(); ++i)
                {
                    // 範囲チェック
                    if (i >= ctrl.menuTargetYs.size()) break;

                    ECS::EntityID uiEntity = ctrl.menuUIEntities[i];
                    float targetY = ctrl.menuTargetYs[i];
                    float startY = SCREEN_HEIGHT + 100.0f; // Initで設定した開始位置と同じ値

                    // 1. 位置の更新 (スライドアップ)
                    if (m_coordinator->HasComponent<TransformComponent>(uiEntity))
                    {
                        auto& trans = m_coordinator->GetComponent<TransformComponent>(uiEntity);
                        // 現在位置 = 開始位置 + (移動距離 * 進行度)
                        trans.position.y = startY + (targetY - startY) * easeT;
                    }

                    // 2. 透明度の更新 (フェードイン)
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity))
                    {
                        auto& img = m_coordinator->GetComponent<UIImageComponent>(uiEntity);
                        img.color.w = easeT; // 0.0(透明) -> 1.0(不透明) に変化
                    }
                }

            }
            for (auto uiEntity : ctrl.menuUIEntities)
            {
                if (m_coordinator->HasComponent<UIButtonComponent>(uiEntity) &&
                    m_coordinator->HasComponent<TransformComponent>(uiEntity))
                {
                    auto& btn = m_coordinator->GetComponent<UIButtonComponent>(uiEntity);
                    auto& trans = m_coordinator->GetComponent<TransformComponent>(uiEntity);

                    // 1. 目標倍率を決める (Hoverなら1.2倍、それ以外は1.0倍)
                    float targetRatio = (btn.state == ButtonState::Hover) ? 1.2f : 1.0f;

                    // 2. 目標サイズを計算 (元のサイズ × 倍率)
                    float targetX = btn.originalScale.x * targetRatio;
                    float targetY = btn.originalScale.y * targetRatio;

                    // 3. 線形補間(Lerp)で滑らかにサイズ変更
                    float speed = 15.0f * deltaTime;

                    trans.scale.x += (targetX - trans.scale.x) * speed;
                    trans.scale.y += (targetY - trans.scale.y) * speed;
                }
            }

            // キャンセル処理（WaitInputに戻るなど）を入れる場合はここに記述
            if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_B))
            {
                // リセット処理など（省略）
            }
            break;
        }
    }
}