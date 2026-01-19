#include "ECS/Systems/Core/TitleControlSystem.h"
#include "ECS/ECS.h"
#include "Systems/Input.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h" 
#include <ECS/Components/Rendering/RenderComponent.h>

using namespace ECS;
using namespace DirectX;

namespace TitleTuning
{
    constexpr float LOGO_FADE_OUT_DURATION = 0.8f;
    constexpr float ZOOM_START_DELAY = 0.8f;
    constexpr float MENU_APPEAR_DELAY = 0.0f;
    constexpr float BUTTON_HOVER_SCALE_MAG = 1.15f;
    constexpr float BUTTON_HOVER_LERP_SPEED = 12.0f;
}

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
            float blinkT = (std::sin(ctrl.animTimer * 5.0f) + 1.0f) * 0.5f;

            if (ctrl.logoEntityID != INVALID_ENTITY_ID && m_coordinator->HasComponent<UIImageComponent>(ctrl.logoEntityID))
            {
                auto& img = m_coordinator->GetComponent<UIImageComponent>(ctrl.logoEntityID);
                ctrl.TitlelogoFadeTimer += deltaTime;
                float logoFadeT = std::min(1.0f, ctrl.TitlelogoFadeTimer / ctrl.TitlelogoFadeDuration);
                img.isVisible = true;
                img.color.w = logoFadeT;

                for (auto uiEntity : ctrl.pressStartUIEntities) {
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                        auto& startImg = m_coordinator->GetComponent<UIImageComponent>(uiEntity);
                        startImg.isVisible = true;
                        startImg.color.w = (0.2f + 0.8f * blinkT) * logoFadeT;
                    }
                }
            }

            if (IsKeyTrigger(VK_RETURN) || IsButtonTriggered(BUTTON_A)) {
                ctrl.state = TitleState::ZoomAnimation;
                ctrl.animTimer = 0.0f;
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

            // ============================================================
            // 修正箇所：座標移動（startYの計算）を削除し、不透明度のみ更新
            // ============================================================
            if (ctrl.uiAnimTimer >= 0.0f) {
                float t = std::min(1.0f, ctrl.uiAnimTimer / ctrl.uiAnimDuration);
                float easeT = 1.0f - (1.0f - t) * (1.0f - t);

                for (size_t i = 0; i < ctrl.menuUIEntities.size(); ++i) {
                    EntityID uiEntity = ctrl.menuUIEntities[i];

                    // 1. 不透明度の更新（フェードイン）
                    if (m_coordinator->HasComponent<UIImageComponent>(uiEntity)) {
                        auto& img = m_coordinator->GetComponent<UIImageComponent>(uiEntity);
                        img.isVisible = true;
                        img.color.w = easeT; // 0.0f から 1.0f へフェード
                    }

                    // 2. ボタン判定の有効化（フェードがほぼ終わったら）
                    if (t > 0.8f && m_coordinator->HasComponent<UIButtonComponent>(uiEntity)) {
                        m_coordinator->GetComponent<UIButtonComponent>(uiEntity).isVisible = true;
                    }
                }
            }

            // ホバー時のスケール制御
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