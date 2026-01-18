#include "ECS/Systems/Core/TitleControlSystem.h"
#include "ECS/ECS.h"
#include "Systems/Input.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h" 
#include <ECS/Components/Rendering/RenderComponent.h>
#include <algorithm>
#include <cmath>

using namespace ECS;
using namespace DirectX;

namespace TitleTuning
{
    constexpr float LOGO_FADE_OUT_DURATION  = 0.8f;
    constexpr float ZOOM_START_DELAY        = 1.1f;
    constexpr float MENU_APPEAR_DELAY       = 0.0f;
    constexpr float BUTTON_HOVER_SCALE_MAG  = 1.15f;
    constexpr float BUTTON_HOVER_LERP_SPEED = 12.0f;

    // --- スウィープ演出用定数 --- 
    constexpr float SWEEP_DURATION  = 8.0f;       // エフェクトが下から上へ移動する時間
    constexpr float SWEEP_START_Y   = 1.37f;      // 3D空間での開始Y座標（下側）
    constexpr float SWEEP_MOVE_DIST = 0.5f;       // 移動距離（上方向への加算分）
    constexpr float FADE_IN_SPEED   = 1.5f;       // ボタンが通過した後のフェード速度

    // --- トランプエフェクト設定（カメラの視界に合わせて調整） ---
    constexpr float LAMP_SPAWN_INTERVAL = 0.8f;   // 降らす頻度
    constexpr float LAMP_LIFE_DURATION  = 50.0f;  // 画面外に落ちるまでの寿命
    constexpr float LAMP_SPAWN_HEIGHT   = 6.0f;   // カメラ(Y=2.5)より高い位置から降らす

    // カメラの向き(-X)に合わせた座標設定
    constexpr float LAMP_X_DEPTH = -6.0f;         // カメラの正面方向（-X側）の距離
    constexpr float LAMP_Z_CENTER = -9.8f;        // カメラのZ座標を中心に
    constexpr float LAMP_Z_RANGE = 1.0f;          // 画面の横方向への広がり
}

void TitleControlSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& ctrl = m_coordinator->GetComponent<TitleControllerComponent>(entity);
        auto it = ctrl.activeLamps.begin();
        while (it != ctrl.activeLamps.end()) {
            it->lifeTimer += deltaTime;
            if (it->lifeTimer >= TitleTuning::LAMP_LIFE_DURATION) {
                m_coordinator->DestroyEntity(it->id);
                it = ctrl.activeLamps.erase(it);
            }
            else {
                ++it;
            }
        }
        switch (ctrl.state)
        {
        case TitleState::WaitInput:
        {
            // --- タイマー更新 ---
            ctrl.animTimer += deltaTime;      // UI点滅用のタイマー
          
            ctrl.TitlelogoFadeTimer += deltaTime; // ロゴフェード用

            // --- UI表示・点滅制御 ---
            float blinkT = (std::sin(ctrl.animTimer * 5.0f) + 1.0f) * 0.5f;

            // ゼロ除算防止（Durationが0なら即座に1.0fにする）
            float fadeDuration = std::max(0.01f, ctrl.TitlelogoFadeDuration);
            float logoFadeT = std::min(1.0f, ctrl.TitlelogoFadeTimer / fadeDuration);

            // ロゴのフェードイン
            if (ctrl.logoEntityID != INVALID_ENTITY_ID && m_coordinator->HasComponent<UIImageComponent>(ctrl.logoEntityID))
            {
                auto& img = m_coordinator->GetComponent<UIImageComponent>(ctrl.logoEntityID);
                img.isVisible = true;
                img.color.w = logoFadeT;

                // Press Start UIの点滅
                for (auto uiEntity : ctrl.pressStartUIEntities) {
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                        auto& startImg = m_coordinator->GetComponent<UIImageComponent>(uiEntity);
                        startImg.isVisible = true;
                        // フェードインしつつ、サイン波で明滅させる
                        startImg.color.w = (0.2f + 0.8f * blinkT) * logoFadeT;
                    }
                }
            }
           
            if (IsKeyTrigger(VK_RETURN) || IsButtonTriggered(BUTTON_A)) {
                ctrl.state = TitleState::ZoomAnimation;
                ctrl.animTimer = 0.0f; // 次のステートのためにリセット
            }
            break;
        }

        case TitleState::ZoomAnimation:
        {
            ctrl.animTimer += deltaTime;
            float alpha = std::max(0.0f, 1.0f - (ctrl.animTimer / TitleTuning::LOGO_FADE_OUT_DURATION));
            auto fadeOutUI = [&](ECS::EntityID id) {
                if (id != INVALID_ENTITY_ID && m_coordinator->HasComponent<UIImageComponent>(id)) {
                    auto& img = m_coordinator->GetComponent<UIImageComponent>(id);
                    img.color.w = alpha;
                    if (alpha <= 0.0f) img.isVisible = false;
                }
                };
            for (auto uiEntity : ctrl.pressStartUIEntities) fadeOutUI(uiEntity);
            fadeOutUI(ctrl.logoEntityID);
          
            if (ctrl.cameraEntityID != INVALID_ENTITY_ID && ctrl.animTimer > TitleTuning::ZOOM_START_DELAY)
            {
                float camProgress = std::min(1.0f, (ctrl.animTimer - TitleTuning::ZOOM_START_DELAY) / ctrl.animDuration);
                float easeOut = 1.0f - std::pow(1.0f - camProgress, 2.5f);
                auto& camTrans = m_coordinator->GetComponent<TransformComponent>(ctrl.cameraEntityID);
                float u = 1.0f - easeOut, tt = easeOut * easeOut, uu = u * u, ut2 = 2.0f * u * easeOut;
                camTrans.position.x = (uu * ctrl.camStartPos.x) + (ut2 * ctrl.camControlPos.x) + (tt * ctrl.camEndPos.x);
                camTrans.position.y = (uu * ctrl.camStartPos.y) + (ut2 * ctrl.camControlPos.y) + (tt * ctrl.camEndPos.y);
                camTrans.position.z = (uu * ctrl.camStartPos.z) + (ut2 * ctrl.camControlPos.z) + (tt * ctrl.camEndPos.z);
                camTrans.rotation.y = ctrl.startRotY + (ctrl.endRotY - ctrl.startRotY) * easeOut;
            }

            if (ctrl.animTimer >= TitleTuning::ZOOM_START_DELAY + ctrl.animDuration) {
                ctrl.state = TitleState::ModeSelect;
                ctrl.uiAnimTimer = TitleTuning::MENU_APPEAR_DELAY;
            }
            break;
        }

        case TitleState::ModeSelect:
        {
            ctrl.uiAnimTimer += deltaTime;

            ctrl.uiAnimTimer += deltaTime;

            // ============================================================
            // 1. 移動エフェクトの生成 (左右2列を維持)
            // ============================================================
            if (!ctrl.effectTriggered) {
                const char* effectName = "EFK_TITLE_SHINE2";
                float effectScale = 0.008f;
                float effectZ = -4.0f;
                float startX[] = { -0.02f, 0.02f };

                for (int i = 0; i < 2; ++i) {
                    EntityID eff = m_coordinator->CreateEntity(
                        TransformComponent(
                            { startX[i], TitleTuning::SWEEP_START_Y, effectZ },
                            { 0.0f, 0.0f, XMConvertToRadians(-20.0f) },
                            { 1.0f, 1.0f, 1.0f }
                        ),
                        // ループなし(false)設定
                        EffectComponent(effectName, true, false, { 0,0,0 }, effectScale)
                    );
                    ctrl.buttonEffectEntities.push_back(eff);
                }
                ctrl.effectTriggered = true;
            }

            // ============================================================
            // 2. エフェクトの移動更新 ＆ 2回目が出る前に消去
            // ============================================================
            float progress = std::min(1.0f, ctrl.uiAnimTimer / TitleTuning::SWEEP_DURATION);
            float current3DY = TitleTuning::SWEEP_START_Y + (TitleTuning::SWEEP_MOVE_DIST * progress);

            auto it = ctrl.buttonEffectEntities.begin();
            while (it != ctrl.buttonEffectEntities.end()) {
                EntityID effId = *it;
                if (m_coordinator->HasComponent<TransformComponent>(effId)) {
                    auto& trans = m_coordinator->GetComponent<TransformComponent>(effId);
                    trans.position.y = current3DY;

                    // 【2回目防止】
                    // 全体の移動(6秒)を待たず、エフェクトがボタンを通り過ぎるくらいの
                    // 短い時間（例: 1.5秒）で実体を消去してしまう
                    if (ctrl.uiAnimTimer > 1.5f) {
                        m_coordinator->DestroyEntity(effId);
                        it = ctrl.buttonEffectEntities.erase(it);
                        continue;
                    }
                }
                ++it;
            }

            // ============================================================
            // 3. ボタンの出現制御 (出現速度を「秒」基準にして高速化)
            // ============================================================

            ctrl.lampSpawnTimer += deltaTime; // トランプ生成用のタイマー
            // --- トランプ（エフェクト）の生成 ---
            if (ctrl.lampSpawnTimer >= TitleTuning::LAMP_SPAWN_INTERVAL) {
                ctrl.lampSpawnTimer = 0.0f;

                float randomZ = TitleTuning::LAMP_Z_CENTER +
                    (((float)rand() / RAND_MAX * TitleTuning::LAMP_Z_RANGE) - (TitleTuning::LAMP_Z_RANGE * 0.5f));
                float randomX = TitleTuning::LAMP_X_DEPTH + (((float)rand() / RAND_MAX * 2.0f) - 1.0f);

                const char* lampAssets[] = { "EFK_TITLE_TRUMP_RED", "EFK_TITLE_TRUMP_BLUE", "EFK_TITLE_TRUMP_PURPLE" };
                const char* selectedEffect = lampAssets[rand() % 3];

                EntityID lamp = m_coordinator->CreateEntity(
                    TransformComponent(
                        { 0.0f, 1.2f, -3.0f },
                        { 0.0f, 0.0f, 0.0f },
                        { 1.0f, 1.0f, 1.0f }
                    ),
                    EffectComponent(selectedEffect, false, true, { 0,0,0 }, 0.1f)
                );

                ctrl.activeLamps.push_back({ lamp, 0.0f });
            }

            for (size_t i = 0; i < ctrl.menuUIEntities.size(); ++i) 
            {
                EntityID uiEntity = ctrl.menuUIEntities[i];

                // 出現開始の「秒数」を計算
                float revealStartTime = (i == 1) ?
                    (0.045f * TitleTuning::SWEEP_DURATION) :
                    (0.085f * TitleTuning::SWEEP_DURATION);

                if (ctrl.uiAnimTimer > revealStartTime) {
                    // 【速度改善】progress(割合)ではなく、開始してからの「秒数」で計算
                    // TitleTuning::FADE_IN_SPEED (4.0) なら、0.25秒でパッと出ます
                    float timeSinceStart = ctrl.uiAnimTimer - revealStartTime;
                    float localT = std::min(1.0f, timeSinceStart * TitleTuning::FADE_IN_SPEED);

                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                        auto& img = m_coordinator->GetComponent<UIImageComponent>(uiEntity);
                        img.isVisible = true;
                        img.color.w = localT;
                    }

                    if (localT > 0.8f && m_coordinator->HasComponent<UIButtonComponent>(uiEntity)) {
                        m_coordinator->GetComponent<UIButtonComponent>(uiEntity).isVisible = true;
                    }
                }
            }

            // ============================================================
            // 4. ホバー時のスケール制御 (既存のボタンアクション)
            // ============================================================
            for (auto uiEntity : ctrl.menuUIEntities) {
                if (m_coordinator->HasComponent<UIButtonComponent>(uiEntity) && m_coordinator->HasComponent<TransformComponent>(uiEntity)) {
                    auto& btn = m_coordinator->GetComponent<UIButtonComponent>(uiEntity);
                    auto& trans = m_coordinator->GetComponent<TransformComponent>(uiEntity);
                    if (!btn.isVisible) continue;

                    float invRatio = 1.0f / 0.6f;
                    float ratio = (btn.state == ButtonState::Hover) ? TitleTuning::BUTTON_HOVER_SCALE_MAG : 1.0f;
                    float targetX = (btn.originalScale.x * invRatio) * ratio;
                    float targetY = (btn.originalScale.y * invRatio) * ratio;

                    float step = TitleTuning::BUTTON_HOVER_LERP_SPEED * deltaTime;
                    trans.scale.x += (targetX - trans.scale.x) * step;
                    trans.scale.y += (targetY - trans.scale.y) * step;
                }
            }
            break;
        }
        }
    }
}