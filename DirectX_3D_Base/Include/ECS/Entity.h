/*****************************************************************//**
 * @file	Entity.h
 * @brief	ECSアーキテクチャのエンティティ定義
 * 
 * @details	Entity Component System (ECS) アーキテクチャにおける
 *			エンティティの基本定義を提供。
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

#ifndef ___ENTITY_H___
#define ___ENTITY_H___

// ===== インクルード =====
#include <cstdint>

/**
 * @struct	Entity
 * @brief	ゲーム世界に存在するオブジェクトを表す一意な識別子
 * 
 * @details	Entityはuint32_t型のID番号のみを保持する、非常に軽量な構造体です。
 *			WorldによってユニークなIDが割り当てられ、このIDを通じてコンポーネントにアクセスします。
 * 
 * @par		使用例：
 * @code
 *	// World wはWorldクラスのインスタンスとします
 *	Entity player = w.Create()
 *		.With<Transform>()
 *		.Build();
 *	if (player.IsValid()) {
 *	// エンティティが有効な場合の処理
 *  }
 * @endcode
 * 
 * @note	エンティティのIDは自動的に割り当てられるため、直接操作する必要はありません。
 * @warning	エンティティを削除する際は、必ずWorld::DestroyEntity()を使用し、IDの再利用処理を行ってください。
 */
struct Entity
{
	// @brief	エンティティを一意に識別するID
	using ID = std::uint32_t;
	/**
	 * @var		id
	 * @brief	エンティティの一意識別番号
	 * 
	 * @details	Worldクラスによって自動的に割り当てられる一意なID。
	 *			このIDを使って、Worldからコンポーネントを取得・追加・削除する。
	 *			0は無効なIDを示します。
	 * 
	 * @warning	IDを直接変更しないこと。
	 */
	ID id = 0;

	/**
	 * [bool - IsValid]
	 * @brief	エンティティが有効なIDを持っているかを判定
	 * 
	 * @return	true.有効 false.無効
	 */
	bool IsValid() const { return id != 0; }

	// --- 比較演算子のオーバーロード ---
	bool operator ==(const Entity& other) const { return id == other.id; }
	bool operator !=(const Entity& other) const { return id != other.id; }
};

#endif // !___ENTITY_H___