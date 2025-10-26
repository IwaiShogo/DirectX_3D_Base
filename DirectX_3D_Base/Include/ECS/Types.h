/*****************************************************************//**
 * @file	Types.h
 * @brief	ECSのコアとなる型定義と定数を集めたファイル
 * 
 * @details	
 * ECSのEntityID、ComponentID、SystemID、およびComponentの
 * Signature（ビットマスク）に必要な定義を行う。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：ECSのコアとなる型定義（ID、Signature）と定数を定義。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TYPES_H___
#define ___TYPES_H___

// ===== インクルード =====
#include <bitset>
#include <cstdint>
#include <limits>

/**
 * @namespace ECS
 * @brief Entity Component Systemの定義を格納する名前空間
 */
namespace ECS
{
	// ===== 型エイリアス定義 (ECSの基本要素) =====

	/// @brief エンティティを一意に識別するための型 (32ビット整数)
	using EntityID = std::uint32_t;

	/// @brief コンポーネントのインデックスを識別するための型 (8ビット整数)
	/// (コンポーネントの種類が増えたら変更を検討)
	using ComponentTypeID = std::uint8_t;

	/// @brief エンティティが保持するコンポーネントの組み合わせを表すビットマスク (Signature)
	/// MAX_COMPONENTSの値に応じてビット数が変化
	using Signature = std::bitset<std::numeric_limits<ComponentTypeID>::max()>;

	/// @brief システムのインデックスを識別するための型 (8ビット整数)
	using SystemTypeID = std::uint8_t;


	// ===== 定数定義 (ECSの制約) =====

	/// @brief 登録可能なコンポーネントの最大数
	/// ComponentTypeIDの最大値に依存する
	static const size_t MAX_COMPONENTS = std::numeric_limits<ComponentTypeID>::max();

	/// @brief 作成可能なエンティティの最大数
	static const size_t MAX_ENTITIES = 5000;

	/// @brief 無効な（未割り当ての）エンティティID
	static const EntityID INVALID_ENTITY_ID = static_cast<EntityID>(-1);
}

#endif // !___TYPES_H___