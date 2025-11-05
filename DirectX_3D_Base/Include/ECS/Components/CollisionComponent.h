/*****************************************************************//**
 * @file	CollisionComponent.h
 * @brief	Entityの当たり判定の形状と属性を定義するComponent。
 * 
 * @details	
 * AABB (Axis-Aligned Bounding Box) を使用し、衝突判定に必要なサイズや
 * 衝突タイプなどの情報を保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：衝突判定に必要なサイズ、オフセット、衝突タイプなどを保持する `CollisionComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COLLISION_COMPONENT_H___
#define ___COLLISION_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include <cstdint>

 /**
  * @enum ColliderType
  * @brief 当たり判定の属性タイプ。
  * * Static: 動かないオブジェクト（床、壁など）。
  * * Dynamic: 物理演算の影響を受ける動くオブジェクト（プレイヤー、敵など）。
  */
enum ColliderType : uint8_t
{
	COLLIDER_STATIC,	///< 静的な衝突体（位置・サイズは変化しない）
	COLLIDER_DYNAMIC,	///< 動的な衝突体（位置が物理演算によって変化する）
	COLLIDER_TRIGGER,	///< 衝突応答は行わず、通過・イベントのみ発生させる（未実装）
};

/**
 * @enum	CollisionLayer
 * @brief	衝突判定のレイヤー定義（ビットフラグとして使用）
 */
enum class CollisionLayer : uint32_t
{
	NONE	= 0,
	WORLD	= 1 << 0, // 地面、壁など静的な環境
	PLAYER	= 1 << 1,
	GUARD	= 1 << 2,
	ITEM	= 1 << 3,
};

/**
 * @struct CollisionComponent
 * @brief Entityの当たり判定情報
 */
struct CollisionComponent
{
	DirectX::XMFLOAT3 size;		///< 当たり判定の半分のサイズ (ハーフエクステント)
	DirectX::XMFLOAT3 offset;	///< TransformComponent.Positionからの相対的なオフセット
	ColliderType type;			///< 衝突体のタイプ（静的/動的）
	uint32_t collisionGroup;	///< 衝突フィルタリング用グループID (ビットマスクを推奨)

	CollisionLayer layer = CollisionLayer::WORLD;       ///< このEntityが所属するレイヤー
	uint32_t mask = (uint32_t)CollisionLayer::WORLD;    ///< 衝突を検出する対象レイヤーのマスク（ビットフラグ）

	/**
	 * @brief コンストラクタ
	 */
	CollisionComponent(
		DirectX::XMFLOAT3 size = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),
		DirectX::XMFLOAT3 offset = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		ColliderType type = COLLIDER_DYNAMIC,
		uint32_t group = 1 // デフォルトはGroup 0 (ビット0)
	) : size(size), offset(offset), type(type), collisionGroup(group)
	{
	}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(CollisionComponent)

#endif // !___COLLISION_COMPONENT_H___