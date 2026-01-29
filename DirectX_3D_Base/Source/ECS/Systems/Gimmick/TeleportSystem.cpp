// TeleportSystem.cpp

#include "ECS/Systems/Gimmick/TeleportSystem.h"
#include "ECS/EntityFactory.h"
#include "ECS/Components/Gimmick/TeleportComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Gameplay/PlayerControlComponent.h"
#include "ECS/Components/UI/UIImageComponent.h" // UI操作用に必要
#include "ECS/Components/Core/TagComponent.h"   // フェード用Entityを探すために必要
#include <algorithm>

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

    // --- Teleport trigger radius ---
    // EFK_TELEPORT のスケールを 0.1 にしたので、当たり判定（起動距離）も 1/3 に縮小する。
    // 元は半径 2.0f だったため、2.0f / 3.0f を使用。
    const float kTeleportTriggerRadius = 2.0f / 3.0f;
    const float kTeleportTriggerRadiusSq = kTeleportTriggerRadius * kTeleportTriggerRadius;

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

            if (distSq < kTeleportTriggerRadiusSq) {
                tp.state = TeleportState::FadingOut;
                tp.currentAlpha = 0.0f;
                tp.warpAnimTimer = 0.0f; // アニメーションタイマーリセット
                tp.warpYOffset = 0.0f;   // オフセットリセット
            }
            break;
        }

        case TeleportState::FadingOut: {
            tp.currentAlpha += TeleportComponent::FADE_SPEED * deltaTime;

            // ワープアニメーション: 下から上へ移動（浮き上がる）
            tp.warpAnimTimer += deltaTime;
            float progress = std::min(tp.warpAnimTimer / TeleportComponent::WARP_ANIM_DURATION, 1.0f);
            float newOffset = TeleportComponent::WARP_HEIGHT * progress; // 0から+3へ（上昇）

            // 前フレームからの差分を適用
            float deltaOffset = newOffset - tp.warpYOffset;
            pTrans.position.y += deltaOffset;
            tp.warpYOffset = newOffset;

            if (tp.currentAlpha >= 1.0f && progress >= 1.0f) {
                tp.currentAlpha = 1.0f;

                // 座標移動(画面が真っ暗なタイミングで実行)
                if (tp.targetEntity != INVALID_ENTITY_ID) {
                    auto& destTrans = m_coordinator->GetComponent<TransformComponent>(tp.targetEntity);
                    auto& destTp = m_coordinator->GetComponent<TeleportComponent>(tp.targetEntity);

                    // ワープアニメーション中のオフセットを元に戻す
                    pTrans.position.y -= tp.warpYOffset;

                    pTrans.position = destTrans.position;
                    pTrans.position.y += 0.8f;

                    tp.state = TeleportState::FadingIn;
                    tp.warpAnimTimer = 0.0f; // アニメタイマーリセット
                    // 出現時のオフセットを最大値（+3）から開始
                    tp.warpYOffset = TeleportComponent::WARP_HEIGHT;
                    // プレイヤーを最初から上に配置
                    pTrans.position.y += tp.warpYOffset;

                    // 送り先のクールダウンも設定
                    destTp.coolDownTimer = TeleportComponent::COOLDOWN_MAX;
                }
            }
            break;
        }

        case TeleportState::FadingIn: {
            tp.currentAlpha -= (TeleportComponent::FADE_SPEED * 2.5f) * deltaTime;

            // ワープアニメーション: 上から下へ移動（降下）
            tp.warpAnimTimer += deltaTime;
            float progress = std::min(tp.warpAnimTimer / TeleportComponent::WARP_ANIM_DURATION, 1.0f);
            float newOffset = TeleportComponent::WARP_HEIGHT * (1.0f - progress); // +3から0へ（降下）

            // 前フレームからの差分を適用
            float deltaOffset = newOffset - tp.warpYOffset;
            pTrans.position.y += deltaOffset;
            tp.warpYOffset = newOffset;

            if (progress >= 1.0f && tp.currentAlpha <= 0.0f) {
                tp.currentAlpha = 0.0f;

                // ワープアニメーション中のオフセットを元に戻す
                pTrans.position.y -= tp.warpYOffset;

                tp.state = TeleportState::Idle;
                tp.coolDownTimer = TeleportComponent::COOLDOWN_MAX;
                tp.warpYOffset = 0.0f; // オフセットリセット
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
