/*****************************************************************//**
 * @file	AnimationSystem.cpp
 * @brief	AnimationComponentを持つEntityのアニメーションを更新するSystemの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/Rendering/AnimationSystem.h"
#include "ECS/ECS.h"

using namespace ECS;

void AnimationSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        auto& animComp = m_coordinator->GetComponent<AnimationComponent>(entity);

        // モデルの実体がなければスキップ
        if (!modelComp.pModel) continue;

        // ----------------------------------------------------
        // 1. 未ロードのアニメーションがあればロードを実行
        // ----------------------------------------------------
        if (!animComp.preloadList.empty())
        {
            for (const auto& animeID : animComp.preloadList)
            {
                // STEP 2でModelに追加した「IDだけでロードする関数」を呼ぶ
                modelComp.pModel->AddAnimation(animeID);
            }
            animComp.preloadList.clear();
        }

        // ----------------------------------------------------
        // 2. 再生リクエストがあれば実行 (C++14対応)
        // ----------------------------------------------------
        if (animComp.hasRequest)
        {
            const auto& req = animComp.currentRequest;

            if (req.isBlend)
            {
                modelComp.pModel->PlayBlend(req.animeID, req.blendTime, req.loop, req.speed);
            }
            else
            {
                modelComp.pModel->Play(req.animeID, req.loop, req.speed);
            }

            // リクエスト消費完了（フラグを下ろす）
            animComp.hasRequest = false;
        }

        // ----------------------------------------------------
        // 3. アニメーションの更新ステップ実行
        // ----------------------------------------------------
        modelComp.pModel->Step(deltaTime);
    }
}