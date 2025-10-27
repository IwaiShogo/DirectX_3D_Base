/*****************************************************************//**
 * @file	CollisionSystem.cpp
 * @brief	CollisionSystemの実装。AABB衝突検出と基本的な応答を処理。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：AABB衝突検出（CheckCollision）と応答（ResolveCollision）のロジックを実装
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/CollisionSystem.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

/**
 * @brief AABB間の衝突検出と最小移動ベクトル(MTV)を計算する。
 * @param entityA, entityB - 衝突チェック対象のEntityID
 * @param mtv - 計算された最小移動ベクトルと衝突面法線
 * @return bool - 衝突している場合はtrue
 */
bool CollisionSystem::CheckCollision(ECS::EntityID entityA, ECS::EntityID entityB, XMFLOAT3& mtv)
{
	TransformComponent& transA = m_coordinator->GetComponent<TransformComponent>(entityA);
	CollisionComponent& collA = m_coordinator->GetComponent<CollisionComponent>(entityA);
	TransformComponent& transB = m_coordinator->GetComponent<TransformComponent>(entityB);
	CollisionComponent& collB = m_coordinator->GetComponent<CollisionComponent>(entityB);

	// AABBの中心座標を計算
	XMFLOAT3 centerA = transA.Position;
	centerA.x += collA.Offset.x;
	centerA.y += collA.Offset.y;
	centerA.z += collA.Offset.z;

	XMFLOAT3 centerB = transB.Position;
	centerB.x += collB.Offset.x;
	centerB.y += collB.Offset.y;
	centerB.z += collB.Offset.z;

	// 中心間のベクトル (distance)
	XMFLOAT3 d;
	d.x = centerA.x - centerB.x;
	d.y = centerA.y - centerB.y;
	d.z = centerA.z - centerB.z;

	// 衝突していない軸を探す
	// 半分の合計サイズ (extent)
	XMFLOAT3 extent;
	extent.x = collA.Size.x + collB.Size.x;
	extent.y = collA.Size.y + collB.Size.y;
	extent.z = collA.Size.z + collB.Size.z;

	// 軸が分離している場合
	if (std::abs(d.x) >= extent.x ||
		std::abs(d.y) >= extent.y ||
		std::abs(d.z) >= extent.z)
	{
		return false; // 衝突なし
	}

	// 衝突している場合: 最小移動ベクトル(MTV)を計算

	// めり込み量 (penetration)
	float pX = extent.x - std::abs(d.x);
	float pY = extent.y - std::abs(d.y);
	float pZ = extent.z - std::abs(d.z);

	// 最もめり込み量が少ない軸を見つける
	if (pX < pY&& pX < pZ)
	{
		// X軸がMTV
		mtv.x = d.x > 0 ? pX : -pX; // めり込み量 pX を符号付きで設定
		mtv.y = 0.0f;
		mtv.z = 0.0f;
	}
	else if (pY < pZ)
	{
		// Y軸がMTV
		mtv.x = 0.0f;
		mtv.y = d.y > 0 ? pY : -pY; // めり込み量 pY を符号付きで設定
		mtv.z = 0.0f;
	}
	else
	{
		// Z軸がMTV
		mtv.x = 0.0f;
		mtv.y = 0.0f;
		mtv.z = d.z > 0 ? pZ : -pZ; // めり込み量 pZ を符号付きで設定
	}

	return true;
}

/**
 * @brief 衝突が発生した場合の応答処理（位置修正と速度変更）
 * @param entityA, entityB - 衝突したEntityID
 * @param mtv - 最小移動ベクトル
 */
void CollisionSystem::ResolveCollision(ECS::EntityID entityA, ECS::EntityID entityB, const XMFLOAT3& mtv)
{
	// RigidBodyとTransformを取得
	TransformComponent& transA = m_coordinator->GetComponent<TransformComponent>(entityA);
	RigidBodyComponent& rigidA = m_coordinator->GetComponent<RigidBodyComponent>(entityA);
	CollisionComponent& collA = m_coordinator->GetComponent<CollisionComponent>(entityA);

	TransformComponent& transB = m_coordinator->GetComponent<TransformComponent>(entityB);
	CollisionComponent& collB = m_coordinator->GetComponent<CollisionComponent>(entityB);

	// 静的なオブジェクト(entityB)にめり込んだ動的なオブジェクト(entityA)を修正する、シンプルな応答
	if (collA.Type == COLLIDER_DYNAMIC && collB.Type == COLLIDER_STATIC)
	{
		// 1. 位置の修正 (めり込み解消)
		transA.Position.x += mtv.x;
		transA.Position.y += mtv.y;
		transA.Position.z += mtv.z;

		// 2. 速度の修正 (反発/停止)

		// 衝突面法線 (mtvの非ゼロ軸)
		XMFLOAT3 normal = { 0.0f, 0.0f, 0.0f };
		if (mtv.x != 0.0f) normal.x = mtv.x > 0 ? 1.0f : -1.0f;
		if (mtv.y != 0.0f) normal.y = mtv.y > 0 ? 1.0f : -1.0f;
		if (mtv.z != 0.0f) normal.z = mtv.z > 0 ? 1.0f : -1.0f;

		// 衝突面に沿った速度をキャンセルし、反発係数を適用
		// Y軸（地面）衝突の場合
		if (normal.y != 0.0f)
		{
			// 下向きの速度 (normal.y > 0 はAが上から衝突) を反発させる
			if (rigidA.Velocity.y * normal.y < 0)
			{
				rigidA.Velocity.y *= -rigidA.Restitution; // 減衰を伴う反発

				// IsGroundedフラグの更新 (PlayerControlComponentを持つ場合)
				if (m_coordinator->m_entityManager->GetSignature(entityA).test(m_coordinator->GetComponentTypeID<PlayerControlComponent>()))
				{
					PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entityA);
					playerControl.IsGrounded = true;
				}
			}

			// ジャンプ初速が重力に負けないように、速度が非常に小さい場合はゼロにクリッピング
			if (std::abs(rigidA.Velocity.y) < 0.1f)
			{
				rigidA.Velocity.y = 0.0f;
			}
		}

		// X軸衝突の場合 (壁)
		if (normal.x != 0.0f && rigidA.Velocity.x * normal.x < 0)
		{
			rigidA.Velocity.x *= -rigidA.Restitution;
		}
	}

	// TODO: Dynamic vs Dynamic の衝突応答も必要に応じて追加する
}

/**
 * @brief 衝突検出と応答を行う
 */
void CollisionSystem::Update()
{
	// StaticなEntityとDynamicなEntityのリストを作成
	std::vector<ECS::EntityID> dynamicEntities;
	std::vector<ECS::EntityID> staticEntities;

	for (auto const& entity : m_entities)
	{
		CollisionComponent& coll = m_coordinator->GetComponent<CollisionComponent>(entity);
		if (coll.Type == COLLIDER_DYNAMIC)
		{
			dynamicEntities.push_back(entity);
		}
		else if (coll.Type == COLLIDER_STATIC)
		{
			staticEntities.push_back(entity);
		}
	}

	// Dynamic Entityと Static Entity間の衝突チェック (Dynamic vs Static)
	for (ECS::EntityID dynamicEntity : dynamicEntities)
	{
		// IsGroundedを一旦リセット (Updateの始めに実行されるべきだが、ここでは衝突前にリセット)
		if (m_coordinator->m_entityManager->GetSignature(dynamicEntity).test(m_coordinator->GetComponentTypeID<PlayerControlComponent>()))
		{
			PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(dynamicEntity);
			playerControl.IsGrounded = false;
		}

		for (ECS::EntityID staticEntity : staticEntities)
		{
			XMFLOAT3 mtv = { 0.0f, 0.0f, 0.0f };

			if (CheckCollision(dynamicEntity, staticEntity, mtv))
			{
				ResolveCollision(dynamicEntity, staticEntity, mtv);
			}
		}
	}
}