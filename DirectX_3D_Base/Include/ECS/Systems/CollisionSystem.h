/*****************************************************************//**
 * @file	CollisionSystem.h
 * @brief	CollisionComponentを持つEntity間の衝突検出と応答を処理するSystem
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：ECS::Systemを継承した'CollisionSystem'を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLISION_SYSTEM_H___
#define ___COLLISION_SYSTEM_H___

// ===== インクルード =====
// ECS
#include "../ECS/Coordinator.h"
#include "../ECS/SystemManager.h"
// Components
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/CollisionComponent.h"
#include "../Components/PlayerControlComponent.h" // 衝突応答でIsGroundedフラグ更新に使用
// Scene
#include "../Scene/GameScene.h" 

#include <DirectXMath.h>
#include <memory>
#include <tuple> // 衝突情報を保持するため

/**
 * @class CollisionSystem
 * @brief Entity間の衝突を検出し、剛体の位置と速度を修正するSystem
 * * 処理対象: TransformComponent, RigidBodyComponent, CollisionComponentを持つEntity
 */
class CollisionSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator;

	/**
	 * @brief AABB間の衝突検出と最小移動ベクトル(MTV)を計算する。
	 * @param entityA, entityB - 衝突チェック対象のEntityID
	 * @param mtv - 計算された最小移動ベクトルと衝突面法線
	 * @return bool - 衝突している場合はtrue
	 */
	bool CheckCollision(ECS::EntityID entityA, ECS::EntityID entityB, DirectX::XMFLOAT3& mtv);

	/**
	 * @brief 衝突が発生した場合の応答処理（位置修正と速度変更）
	 * @param entityA, entityB - 衝突したEntityID
	 * @param mtv - 最小移動ベクトル
	 */
	void ResolveCollision(ECS::EntityID entityA, ECS::EntityID entityB, const DirectX::XMFLOAT3& mtv);

public:
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief 衝突検出と応答を行う
	void Update();
};

#endif // !___COLLISION_SYSTEM_H___