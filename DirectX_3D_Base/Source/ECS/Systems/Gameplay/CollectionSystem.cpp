/*****************************************************************//**
 * @file	CollectionSystem.cpp
 * @brief	アイテムとプレイヤーの距離をチェックし、回収処理をするシステムの実装
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/Gameplay/CollectionSystem.h"
#include <DirectXMath.h>
#include <vector>
#include "ECS/EntityFactory.h"

using namespace DirectX;

void CollectionSystem::Update(float deltaTime)
{
	(void)deltaTime;
	if (!m_coordinator) return;

	// 1. プレイヤーとトラッカー取得
	ECS::EntityID playerID = ECS::FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
	ECS::EntityID trackerID = ECS::FindFirstEntityWithComponent<ItemTrackerComponent>(m_coordinator);

	if (playerID == ECS::INVALID_ENTITY_ID || trackerID == ECS::INVALID_ENTITY_ID) return;

	TransformComponent& playerTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
	ItemTrackerComponent& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(trackerID);
	XMVECTOR playerPosV = XMLoadFloat3(&playerTrans.position);

	std::vector<ECS::EntityID> entitiesToDestroy;
	bool itemCollectedThisFrame = false; // アイテム回収があったかフラグ

	// 2. アイテム走査＆回収判定
	for (auto const& itemEntity : m_entities)
	{
		auto& collectable = m_coordinator->GetComponent<CollectableComponent>(itemEntity);

		// 既に回収済みなら無視（ここで弾かないと破壊済みエンティティを参照してしまう恐れがあるため）
		if (collectable.isCollected) continue;

		TransformComponent& itemTrans = m_coordinator->GetComponent<TransformComponent>(itemEntity);
		XMVECTOR itemPosV = XMLoadFloat3(&itemTrans.position);

		// 距離チェック
		XMVECTOR distanceV = itemPosV - playerPosV;
		float distanceSq = XMVectorGetX(XMVector3LengthSq(distanceV));
		float requiredDistanceSq = collectable.collectionRadius * collectable.collectionRadius;

		if (distanceSq <= requiredDistanceSq)
		{
			// --- 回収処理 ---
			collectable.isCollected = true;
			tracker.collectedItems++;
			entitiesToDestroy.push_back(itemEntity);
			itemCollectedThisFrame = true;

			// --- 正解・不正解判定 ---
			if (tracker.useOrderedCollection)
			{
				// 今のターゲット番号と一致するか？
				if (collectable.orderIndex == tracker.currentTargetOrder)
				{
					// 正解！
					ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST3");
				}
				else
				{
					// 不正解（ペナルティ）
					ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_BUZZER");
					printf("Penalty! Wrong Order. Target: %d, Got: %d\n", tracker.currentTargetOrder, collectable.orderIndex);

					// ペナルティ処理を記述...
				}
			}
			else
			{
				// 順序なしモード
				ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST3");
			}
		}
	}

	// 3. 破壊処理
	for (ECS::EntityID entity : entitiesToDestroy)
	{
		m_coordinator->DestroyEntity(entity);
	}

	// 4. 次のターゲットを再検索（アイテム回収があった場合のみ更新）
	if (itemCollectedThisFrame && tracker.useOrderedCollection)
	{
		int minOrder = 99999; // 十分大きな数
		bool foundNext = false;

		// 全エンティティを走査して「未回収」かつ「最小の番号」を探す
		for (auto const& entity : m_entities)
		{
			// Destroy予定のものは除外したいが、m_entitiesにはまだ残っている可能性がある
			// しかし collectable.isCollected = true にしたので、それで判定可能
			auto& item = m_coordinator->GetComponent<CollectableComponent>(entity);

			if (!item.isCollected)
			{
				if (item.orderIndex < minOrder)
				{
					minOrder = item.orderIndex;
					foundNext = true;
				}
			}
		}

		if (foundNext)
		{
			// 未回収の中で一番小さい番号を次のターゲットにする
			tracker.currentTargetOrder = minOrder;
		}
		else
		{
			// 未回収がない＝コンプリート
			// 便宜上ターゲット番号をあり得ない値にしておく
			tracker.currentTargetOrder = tracker.totalItems + 1;
		}

		printf("Next Target Order Updated: %d\n", tracker.currentTargetOrder);
	}

	// 5. クリア判定
	if (tracker.totalItems > 0 && tracker.collectedItems == tracker.totalItems)
	{
		// TODO: 全アイテム回収後の脱出ロジック (タスク2-4で実装)

		// 便宜上、GameStateComponentにクリアフラグを立てる（タスク2-4で修正）
		ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
		if (controllerID != ECS::INVALID_ENTITY_ID)
		{
			// GameStateComponentにWIN/CLEAR状態を追加する必要がある
			// 現状のGameMode enumにはないため、ここでは仮にフラグを立てる
			// 例: m_coordinator->GetComponent<GameStateComponent>(controllerID).isCleared = true;
		}
	}
}