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
#include "ECS/ECS.h"
#include "ECS/EntityFactory.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

// ヘルパー関数の定義 (DirectXMathの機能拡張を想定)
namespace MathHelper
{
	// XMVector3Dotで内積を計算
	inline float Dot(DirectX::XMVECTOR v1, DirectX::XMVECTOR v2)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v1, v2));
	}
}

/**
 * @brief AABB間の衝突検出と最小移動ベクトル(MTV)を計算する。
 * @param entityA, entityB - 衝突チェック対象のEntityID
 * @param mtv - 計算された最小移動ベクトルと衝突面法線
 * @return bool - 衝突している場合はtrue
 */
bool CollisionSystem::CheckCollision(ECS::EntityID entityA, ECS::EntityID entityB, XMFLOAT3& mtv)
{
	// A: Dynamic Entity (プレイヤー: AABBを想定)
	TransformComponent& transA = m_coordinator->GetComponent<TransformComponent>(entityA);
	CollisionComponent& collA = m_coordinator->GetComponent<CollisionComponent>(entityA);
	// B: Static Entity (壁: OBBを想定)
	TransformComponent& transB = m_coordinator->GetComponent<TransformComponent>(entityB);
	CollisionComponent& collB = m_coordinator->GetComponent<CollisionComponent>(entityB);

	// AABBの中心座標を計算 (オフセットを考慮)
	XMVECTOR posA = XMLoadFloat3(&transA.position) + XMLoadFloat3(&collA.offset);
	// OBBの中心座標を計算 (オフセットを考慮)
	XMVECTOR posB = XMLoadFloat3(&transB.position) + XMLoadFloat3(&collB.offset);

	// A, B間の中心距離ベクトル
	XMVECTOR separation = posB - posA;

	// A, Bのハーフエクステント (サイズ)
	XMFLOAT3 extentsA = collA.size;
	XMFLOAT3 extentsB = collB.size;

	// =========================================================================
	// OBB (entityB) の軸情報取得
	// TransformComponentのY軸回転 (ラジアン) から回転行列を構築する
	// =========================================================================
	// Y軸回転成分のみを取得
	float rotationY_B = transB.rotation.y;

	// 回転行列を構築 (Y軸回転のみ)
	XMMATRIX rotMatB = XMMatrixRotationY(rotationY_B);

	// OBBのローカル軸（X, Y, Z）を抽出する (DirectXMathでは行ベクトル)
	XMVECTOR axisBX = rotMatB.r[0]; // BのローカルX軸
	XMVECTOR axisBY = rotMatB.r[1]; // BのローカルY軸
	XMVECTOR axisBZ = rotMatB.r[2]; // BのローカルZ軸

	// 衝突判定に使用する軸（ワールド軸 3本 + OBBのローカル軸 3本 = 計6本）
	XMVECTOR axes[] = {
		XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), // World X (AABB軸)
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), // World Y (AABB軸)
		XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // World Z (AABB軸)
		axisBX, // OBB Local X
		axisBY, // OBB Local Y
		axisBZ  // OBB Local Z
	};

	float minOverlap = std::numeric_limits<float>::max();
	XMVECTOR minAxis = XMVectorZero();
	bool isCollision = true;

	for (int i = 0; i < 6; ++i)
	{
		XMVECTOR axis = XMVector3Normalize(axes[i]);

		// 軸上の中心間の距離 (|P_A - P_B| ・ N)
		float centerProjection = std::abs(MathHelper::Dot(separation, axis));

		// Aの投影半径 (AABBを軸Nに投影: |e_A・N_x| + |e_A・N_y| + |e_A・N_z|)
		// AABBはワールド軸に整列しているため、Projection Radiusは簡単に計算できる
		float radiusA = std::abs(extentsA.x * MathHelper::Dot(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), axis))
			+ std::abs(extentsA.y * MathHelper::Dot(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), axis))
			+ std::abs(extentsA.z * MathHelper::Dot(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), axis));

		// Bの投影半径 (OBBを軸Nに投影: Bのローカル軸とNの内積を使用)
		float radiusB = std::abs(extentsB.x * MathHelper::Dot(axisBX, axis))
			+ std::abs(extentsB.y * MathHelper::Dot(axisBY, axis))
			+ std::abs(extentsB.z * MathHelper::Dot(axisBZ, axis));

		float totalRadius = radiusA + radiusB;

		// 分離軸定理チェック
		if (centerProjection > totalRadius)
		{
			isCollision = false; // 分離軸が見つかった
			break;
		}

		// 重なりを計算し、最小重なり軸を更新
		float overlap = totalRadius - centerProjection;
		if (overlap < minOverlap)
		{
			minOverlap = overlap;
			minAxis = axis;
		}
	}

	if (isCollision)
	{
		// MTVの方向を Separationベクトル（B-A）とは逆、すなわちAからBを押し出す方向に設定
		// d = centerA - centerB なので、separation = centerB - centerA = -d
		// minAxisが分離ベクトル(B-A)と同じ方向を向いていれば、反転させる必要がある
		XMVECTOR dV = posA - posB; // 衝突検出のMTV符号決定に使用
		if (MathHelper::Dot(minAxis, dV) < 0.0f)
		{
			minAxis = -minAxis;
		}

		XMVECTOR mtvV = minAxis * minOverlap;
		XMStoreFloat3(&mtv, mtvV);
		return true;
	}

	return false;
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
	if (collA.type == COLLIDER_DYNAMIC && collB.type == COLLIDER_STATIC)
	{
		// 1. 位置の修正 (めり込み解消)
		transA.position.x += mtv.x;
		transA.position.y += mtv.y;
		transA.position.z += mtv.z;

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
			if (rigidA.velocity.y * normal.y < 0)
			{
				rigidA.velocity.y *= -rigidA.restitution; // 減衰を伴う反発

				// IsGroundedフラグの更新 (PlayerControlComponentを持つ場合)
				if (m_coordinator->m_entityManager->GetSignature(entityA).test(m_coordinator->GetComponentTypeID<PlayerControlComponent>()))
				{
					PlayerControlComponent& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entityA);
					//playerControl.isGrounded = true;
				}
			}

			// ジャンプ初速が重力に負けないように、速度が非常に小さい場合はゼロにクリッピング
			if (std::abs(rigidA.velocity.y) < 0.1f)
			{
				rigidA.velocity.y = 0.0f;
			}
		}

		// X軸衝突の場合 (壁)
		if (normal.x != 0.0f && rigidA.velocity.x * normal.x < 0)
		{
			rigidA.velocity.x *= -rigidA.restitution;
		}
	}

	// TODO: Dynamic vs Dynamic の衝突応答も必要に応じて追加する
}

/**
 * @brief 衝突検出と応答を行う
 */
void CollisionSystem::Update(float deltaTime)
{
	// --- 0. 初期化とゲーム状態の取得 ---
	ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
	if (controllerID == ECS::INVALID_ENTITY_ID) return;

	GameStateComponent& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

	// ゲームが既に終了していたら、衝突チェックをスキップ
	if (state.isGameOver || state.isGameClear) return;

	// Deferred destruction list (アイテム回収による破壊の遅延)
	std::vector<ECS::EntityID> entitiesToDestroy;

	ECS::EntityID playerID = ECS::FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
	if (playerID == ECS::INVALID_ENTITY_ID) return;

	// StaticなEntityとDynamicなEntityのリストを作成
	std::vector<ECS::EntityID> dynamicEntities;
	std::vector<ECS::EntityID> staticEntities;

	for (auto const& entity : m_entities)
	{
		auto& coll = m_coordinator->GetComponent<CollisionComponent>(entity);

		if (coll.type == COLLIDER_DYNAMIC)
		{
			dynamicEntities.push_back(entity);
		}
		else if (coll.type == COLLIDER_STATIC)
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
			//playerControl.isGrounded = false;
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

	// ここでは、Playerと他のすべての衝突可能なエンティティ間の衝突をチェックします
	for (ECS::EntityID entityB : m_entities)
	{
		if (entityB == playerID) continue;

		XMFLOAT3 mtv_dummy = { 0.0f, 0.0f, 0.0f };

		// CheckCollisionは、PhysicsSystemの衝突解決とは別に、ゲームロジックのトリガーとして使用
		if (CheckCollision(playerID, entityB, mtv_dummy))
		{
			// 衝突相手のコンポーネントシグネチャ（タグ）を取得
			auto& tagB = m_coordinator->GetComponent<TagComponent>(entityB);

			// --- 1. GAME OVER TRIGGER (Player vs Guard) ---
			if (tagB.tag == "guard")
			{
				auto& guard = m_coordinator->GetComponent<GuardComponent>(entityB);

				if (!guard.isActive) continue;

				ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST5");

				state.isGameOver = true;
				// ゲームオーバー時は、すぐにリターンし、他の処理（アイテム回収など）を停止
				return;
			}

			// 2. ITEM COLLECTION TRIGGER (Player vs Collectable)
			if (tagB.tag == "goal")
			{
				ItemTrackerComponent& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);

				if (tracker.totalItems > 0 && tracker.collectedItems == tracker.totalItems)
				{
					ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST4");

					state.isGameClear = true;
				}
			}
			if (tagB.tag == "taser")
			{
				ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST6");
				
				state.isGameOver = true;
				
				return;
			}
		}
	}

#ifdef _DEBUG
	auto debugEntityID = ECS::FindFirstEntityWithComponent<DebugComponent>(m_coordinator);
	if (debugEntityID != ECS::INVALID_ENTITY_ID)
	{
		auto& debug = m_coordinator->GetComponent<DebugComponent>(debugEntityID);

		// 当たり判定の可視化
		if (!debug.isCollisionDrawEnabled) return;

		// TransformComponentとCollisionComponentを持つEntityを走査
		for (const auto& entity : m_entities)
		{
			// CollisionComponentの形状に基づき
			// Geometory::AddLine() を使用してヒットボックスの外枠を描画
		}
	}
#endif // _DEBUG
}