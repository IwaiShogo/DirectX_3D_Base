/*****************************************************************//**
 * @file	World.h
 * @brief	ECSワールド管理システムとエンティティビルダーの定義
 * 
 * @details	このファイルは、ECSアーキテクチャの中核となるWorldクラスと、
 *			エンティティを簡単に作成するためのEntityBuilderクラスを定義します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/17	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___WORLD_H___
#define ___WORLD_H___

// ===== インクルード =====
#include "Entity.h"
#include "Component.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

// ===== 前方宣言 =====
class World;

// --------------------------------------------------
// 1. コンポーネントストアの基盤 (IStore, Store<T>)
// --------------------------------------------------

/**
 * @struct	IStore
 * @brief	全てのコンポーネントストアの抽象基底クラス
 * 
 * @details	Worldクラスが型を意識せずに（std::type_indexで）特定の
 *			コンポーネントストアへのポインタを保持できるように
 *			するためのインターフェースです。
 */
struct IStore
{
	virtual ~IStore() = default;

	/**
	 * [void - RemoveComponent]
	 * @brief	エンティティが破棄された際、そのコンポーネントをストアから削除する
	 * 
	 * @param	[in] e	削除対象のエンティティ 
	 */
	virtual void RemoveComponent(Entity e) = 0;
};

#endif // !___WORLD_H___