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

using namespace DirectX;

void CollectionSystem::Update(float deltaTime)
{
	(void)deltaTime;

	if (!m_coordinator) return;

	// 1. プレイヤーとトラッカー（GameController）のEntityIDを取得
	ECS::EntityID playerID = ECS::FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
	ECS::EntityID trackerID = ECS::FindFirstEntityWithComponent<ItemTrackerComponent>(m_coordinator);

	if (playerID == ECS::INVALID_ENTITY_ID || trackerID == ECS::INVALID_ENTITY_ID) return;

	// プレイヤーとトラッカーのComponentを取得
	TransformComponent& playerTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
	ItemTrackerComponent& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(trackerID);
	XMVECTOR playerPosV = XMLoadFloat3(&playerTrans.position);

	// 破壊対象のEntityIDを一時的に書くのするリスト
	std::vector<ECS::EntityID> entitiesToDestroy;

	// 2. アイテムエンティティを走査
	for (auto const& itemEntity : m_entities) // CollectableComponentを持つEntityを対象
	{
		auto& collectable = m_coordinator->GetComponent<CollectableComponent>(itemEntity);
		if (collectable.isCollected) continue; // 既に回収済みの場合はスキップ

		TransformComponent& itemTrans = m_coordinator->GetComponent<TransformComponent>(itemEntity);
		XMVECTOR itemPosV = XMLoadFloat3(&itemTrans.position);

		// 3. 衝突（自動回収）判定ロジック
		// (シンプルな距離チェックで代用。厳密にはCollisionSystemが行う)
		XMVECTOR distanceV = itemPosV - playerPosV;
		float distanceSq = XMVectorGetX(XMVector3LengthSq(distanceV));
		float requiredDistanceSq = collectable.collectionRadius * collectable.collectionRadius;

		if (distanceSq <= requiredDistanceSq)
		{
			// 回収処理

			// 3-a. トラッカーの更新
			tracker.collectedItems++;
			collectable.isCollected = true;

			// 3-b. アイテムエンティティの破棄
			entitiesToDestroy.push_back(itemEntity);	// リストに追加

			// TODO: 回収エフェクト、サウンド再生などのロジックを追加
		}
	}

	// 4. ループ終了後に破壊対象のEntityをまとめて破棄する
	for (ECS::EntityID entity : entitiesToDestroy)
	{
		m_coordinator->DestroyEntity(entity);
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