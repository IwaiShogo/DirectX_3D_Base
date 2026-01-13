/*****************************************************************//**
 * @file    AnimationSystem.cpp
 * @brief   Updates animation playback for entities that have AnimationComponent.
 *********************************************************************/

#include "ECS/Systems/Rendering/AnimationSystem.h"
#include "ECS/ECS.h"
#include <DirectXMath.h>

using namespace ECS;

// Wrap angle delta across +-PI (prevents snapping)
static float WrapAngleDelta(float d)
{
    while (d > DirectX::XM_PI)  d -= DirectX::XM_2PI;
    while (d < -DirectX::XM_PI) d += DirectX::XM_2PI;
    return d;
}



// AnimationSystem.cpp

void AnimationSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        auto& animComp = m_coordinator->GetComponent<AnimationComponent>(entity);

        if (!modelComp.pModel) continue;

        // 1) Preload animations
        if (!animComp.preloadList.empty())
        {
            for (const auto& animeID : animComp.preloadList)
            {
                modelComp.pModel->AddAnimation(animeID);
            }
            animComp.preloadList.clear();
        }

        // 2) Handle play requests
        if (animComp.hasRequest)
        {
            const auto& req = animComp.currentRequest;

            if (req.isBlend)
                modelComp.pModel->PlayBlend(req.animeID, req.blendTime, req.loop, req.speed);
            else
                modelComp.pModel->Play(req.animeID, req.loop, req.speed);

            // ★★★ 修正箇所：ここに追加 ★★★
            // 再生開始時に、古い履歴データ（前のEntityの残りカス）を消す
            if (m_prevNodeAnimPos.count(entity)) m_prevNodeAnimPos.erase(entity);
            if (m_prevNodeAnimRot.count(entity)) m_prevNodeAnimRot.erase(entity);
            // ★★★ 追加終わり ★★★

            // リクエスト時はリセットフラグを立てる
            m_resetNodeAnimPos.insert(entity);
            animComp.hasRequest = false;
        }

        // 3) Advance animation time
        modelComp.pModel->Step(deltaTime);

        // 4) Apply node-animation transform
        auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

        DirectX::XMFLOAT3 animPos{};
        DirectX::XMFLOAT3 animRot{};
        DirectX::XMFLOAT3 animScale{};

        if (modelComp.pModel->GetAnimatedTransform(animPos, animRot, animScale))
        {
            // --- 位置用のイテレータ取得 ---
            auto itPrevPos = m_prevNodeAnimPos.find(entity);
            if (itPrevPos == m_prevNodeAnimPos.end())
            {
                // 新規、またはPlayでeraseされた直後はここに来る
                itPrevPos = m_prevNodeAnimPos.emplace(entity, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)).first;
                m_resetNodeAnimPos.insert(entity); // 念のためリセットフラグを保証
            }

            // --- 回転用のイテレータ取得 ---
            auto itPrevRot = m_prevNodeAnimRot.find(entity);
            if (itPrevRot == m_prevNodeAnimRot.end())
            {
                itPrevRot = m_prevNodeAnimRot.emplace(entity, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)).first;
            }

            auto& prevPos = itPrevPos->second;
            auto& prevRot = itPrevRot->second;

            // リセット要求がある場合（再生開始直後など）
            if (m_resetNodeAnimPos.find(entity) != m_resetNodeAnimPos.end())
            {
                prevPos = animPos;
                prevRot = animRot; // 現在のアニメ状態を「前回」として記録（差分0にするため）
                m_resetNodeAnimPos.erase(entity);
            }
            else
            {
                // --- 位置の適用 (Delta) ---
                transform.position.x += (animPos.x - prevPos.x);
                transform.position.y += (animPos.y - prevPos.y);
                transform.position.z += (animPos.z - prevPos.z);
                prevPos = animPos;

                // --- 回転の適用 (Delta) ---
                transform.rotation.x += WrapAngleDelta(animRot.x - prevRot.x);
                transform.rotation.y += WrapAngleDelta(animRot.y - prevRot.y);
                transform.rotation.z += WrapAngleDelta(animRot.z - prevRot.z);
                prevRot = animRot;
            }

            // スケールは絶対値適用
            transform.scale = animScale;
        }
    }
}