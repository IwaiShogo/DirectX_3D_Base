// TeleportSystem.cpp

#include "ECS/Systems/Gimmick/TeleportSystem.h"
#include "ECS/EntityFactory.h"
#include "ECS/Components/Gimmick/TeleportComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Gameplay/PlayerControlComponent.h"
#include "ECS/Components/UI/UIImageComponent.h" // UI操作用に必要
#include "ECS/Components/Core/TagComponent.h"   // フェード用Entityを探すために必要

using namespace DirectX;
using namespace ECS;

void TeleportSystem::Init(Coordinator* coordinator) {
    m_coordinator = coordinator;
}

void TeleportSystem::Update(float deltaTime) {
    if (!m_coordinator) return;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;

    EntityID fadeUI = INVALID_ENTITY_ID;
    for (auto const& e : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<TagComponent>(e) &&
            m_coordinator->GetComponent<TagComponent>(e).tag == "SCREEN_FADE") {
            fadeUI = e;
            break;
        }
    }

    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    for (auto const& entity : m_entities) {
        auto& tp = m_coordinator->GetComponent<TeleportComponent>(entity);
        auto& tTrans = m_coordinator->GetComponent<TransformComponent>(entity);

        // クールダウン更新
        if (tp.coolDownTimer > 0.0f) {
            tp.coolDownTimer -= deltaTime;
        }

        // --- ステートマシンによるテレポート制御 ---
        switch (tp.state) {
        case TeleportState::Idle: {
            if (tp.coolDownTimer > 0.0f) continue;

            XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
            XMVECTOR tPos = XMLoadFloat3(&tTrans.position);
            float distSq = XMVectorGetX(XMVector3LengthSq(pPos - tPos));

            if (distSq < 2.0f * 2.0f) {
                tp.state = TeleportState::FadingOut;
                tp.currentAlpha = 0.0f;
            }
            break;
        }

        case TeleportState::FadingOut: {
            tp.currentAlpha += TeleportComponent::FADE_SPEED * deltaTime;
            if (tp.currentAlpha >= 1.0f) {
                tp.currentAlpha = 1.0f;

                // 座標移動（画面が真っ暗なタイミングで実行）
                if (tp.targetEntity != INVALID_ENTITY_ID) {
                    auto& destTrans = m_coordinator->GetComponent<TransformComponent>(tp.targetEntity);
                    auto& destTp = m_coordinator->GetComponent<TeleportComponent>(tp.targetEntity);

                    pTrans.position = destTrans.position;
                    pTrans.position.y += 0.8f;

                    tp.state = TeleportState::FadingIn;
                    // 送り先のクールダウンも設定
                    destTp.coolDownTimer = TeleportComponent::COOLDOWN_MAX;
                }
            }
            break;
        }

        case TeleportState::FadingIn: {
            tp.currentAlpha -= TeleportComponent::FADE_SPEED * deltaTime;
            if (tp.currentAlpha <= 0.0f) {
                tp.currentAlpha = 0.0f;
                tp.state = TeleportState::Idle;
                tp.coolDownTimer = TeleportComponent::COOLDOWN_MAX;
            }
            break;
        }
        }

        // --- フェードUIへの反映 ---
        if (tp.state != TeleportState::Idle && fadeUI != INVALID_ENTITY_ID) {
            auto& img = m_coordinator->GetComponent<UIImageComponent>(fadeUI);
            img.isVisible = true;
            img.color.w = tp.currentAlpha; // A値を更新
        }
    }
}