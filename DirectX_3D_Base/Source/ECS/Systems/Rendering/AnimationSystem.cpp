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

using namespace ECS;

/**
 * [void - Update]
 * @brief	アニメーションを更新する
 * 
 * @param	[in] deltaTime デルタタイム
 */
void AnimationSystem::Update(float deltaTime)
{
	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// 必要なコンポーネントを取得
		ModelComponent& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
		AnimationComponent& animComp = m_coordinator->GetComponent<AnimationComponent>(entity);

		if (modelComp.pModel)
		{
			// コンポーネントが保持する「状態(m_state)」を渡して、
			// モデルの計算ロジックを実行する。
			modelComp.pModel->UpdateAnimation(animComp.m_state, deltaTime);
		}
	}
}