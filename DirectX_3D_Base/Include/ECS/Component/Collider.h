/*****************************************************************//**
 * @file	Collider.h
 * @brief	衝突判定に必要な情報を保持するコンポーネント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLIDER_H___
#define ___COLLIDER_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include <DirectXMath.h>
#include <cstdint>

// 既存の幾何学的なデータ型への依存
#include "Systems/Geometory.h"	// AABB、OBB、Sphereなどの定義

enum class ColliderType
	: std::uint8_t
{
	NONE = 0,
	BOX_AABB,	// 軸並行バウンディングボックス（Axis-Aligned Bounding Box）
	BOX_OBB,	// 方向付きバウンディングボックス（Oriented Bounding Box）
	SPHERE,		// バウンディングスフィア
	CAPSULE,	// カプセル形状
	MESH,		// メッシュベースの衝突判定（※プロトタイプでは除外）
};

struct Collider
	: public IComponent
{
    // --------------------------------------------------
    // 形状定義
    // --------------------------------------------------

    /// @brief このColliderが使用する形状タイプ
    ColliderType type = ColliderType::NONE;

    /// @brief 形状データ (共用体/アライメントを考慮した実装が必要ですが、ここではシンプルな構造体を使用)
    // 実際の実装では、メモリ効率とECSのデータ構造のシンプルさを保つため、
    // ShapeDataという形で別の構造体や共用体として定義し、Coordinatorで管理を分けることがあります。

    // BoundingBox (AABBまたはOBB)
    DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f }; // ローカル座標系での中心
    DirectX::XMFLOAT3 extent = { 0.5f, 0.5f, 0.5f }; // 半径/半幅 (AABBのExtent/OBBのHalf-Extent)

    // BoundingSphere
    float radius = 0.5f;

    // --------------------------------------------------
    // 物理プロパティ
    // --------------------------------------------------

    /// @brief 衝突判定のレイヤー/カテゴリ (どのグループと衝突するかを制御)
    std::uint32_t collisionLayer = 1;

    /// @brief 衝突時の応答 (ヒットストップ、ダメージ計算など) を必要とするかどうか
    bool isTrigger = false;

    /// @brief 物理的な相互作用（押し合いなど）を行うかどうか
    bool isKinematic = false; // trueの場合、物理演算で動かされない

    // --------------------------------------------------
    // 状態（CollisionSystemによって書き込まれる）
    // --------------------------------------------------

    /// @brief 前フレームで衝突が発生したかどうか
    bool collidedThisFrame = false;

    // 衝突相手のEntity IDのリスト（複雑になるため、プロトタイプでは単一のIDを保持するか、別途イベントシステムで処理）
    // Entity lastHitEntity = 0;
};

#endif // !___COLLIDER_H___