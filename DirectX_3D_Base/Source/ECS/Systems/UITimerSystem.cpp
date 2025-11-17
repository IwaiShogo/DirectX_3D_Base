/*****************************************************************//**
 * @file	UITimerSystem.cpp
 * @brief	UITimerSystemの実装。
 * * @details
 * * ------------------------------------------------------------
 * @author	Iwai Shogo (Assisted by Machiko DX)
 * ------------------------------------------------------------
 * * @date	2025/11/16	初回作成日
 * 作業内容：	- 追加：タイマーを減算し、0になったらUIを非表示にして
 * TemporaryUIComponent を削除するロジックを実装。
 * * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/UITimerSystem.h"

 // Update() から TemporaryUIComponent と UIComponent にアクセスするため
#include "ECS/Components/TemporaryUIComponent.h"
#include "ECS/Components/UIComponent.h"

/**
 * @brief 更新処理 (毎フレーム)
 * @param deltaTime - 前フレームからの経過時間
 */
void UITimerSystem::Update(float deltaTime)
{
	if (!m_coordinator) return;

	// イテレータの無効化を避けるため、
	// 処理（コンポーネント削除）対象のエンティティを一時リストに格納する
	std::vector<ECS::EntityID> entitiesToRemoveTimer;

	// 1. 監視対象のエンティティを走査し、タイマーを更新
	for (auto const& entity : m_entities)
	{
		// このエンティティは TemporaryUIComponent を持つことが保証されている
		auto& timerComp = m_coordinator->GetComponent<TemporaryUIComponent>(entity);
		timerComp.displayTimer -= deltaTime;

		// 2. タイマーが0以下になったかチェック
		if (timerComp.displayTimer <= 0.0f)
		{
			// UIを非表示にする
			try
			{
				// このエンティティは UIComponent も持つことが保証されている
				auto& uiComp = m_coordinator->GetComponent<UIComponent>(entity);
				uiComp.IsVisible = false;
			}
			catch (...)
			{
				// 基本的にあり得ないが、GetComponent<UIComponent>が失敗した場合
				// (ログ出力など)
			}

			// 処理対象リストに追加
			entitiesToRemoveTimer.push_back(entity);
		}
	}

	// 3. 処理対象リスト（タイマーが切れたエンティティ）を処理
	for (ECS::EntityID entity : entitiesToRemoveTimer)
	{
		// TemporaryUIComponent をエンティティから削除する
		// これにより、このエンティティは UITimerSystem の監視対象から外れる
		m_coordinator->RemoveComponent<TemporaryUIComponent>(entity);
	}
}