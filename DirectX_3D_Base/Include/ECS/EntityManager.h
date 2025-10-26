/*****************************************************************//**
 * @file	EntityManager.h
 * @brief	ECSのEntityの生成・管理を行うクラス。
 * 
 * @details	
 * EntityIDのプール管理と、各EntityのComponent Signatureを保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ENTITY_MANAGER_H___
#define ___ENTITY_MANAGER_H___

// ===== インクルード =====
#include "Types.h"
#include <queue>
#include <array>
#include <stdexcept>
#include <string>

namespace ECS
{
	/**
	 * @class EntityManager
	 * @brief EntityIDの生成、再利用、およびSignatureの管理を行う。
	 */
	class EntityManager
	{
	private:
		// 再利用可能なEntityIDを保持するキュー（プーリング）
		std::queue<EntityID> m_availableEntities;

		// EntityIDに対応するComponent Signature（どのComponentを持つかを示すビットマスク）
		std::array<Signature, MAX_ENTITIES> m_signatures;

		// 現在までに作成されたEntityの総数
		EntityID m_livingEntityCount = 0;

	public:
		EntityManager()
		{
			// MAX_ENTITIESまでの全てのIDを、利用可能なキューに追加する
			for (EntityID i = 0; i < MAX_ENTITIES; ++i)
			{
				m_availableEntities.push(i);
			}
		}

		/// @brief 新しいEntityIDを生成し、割り当てる
		EntityID CreateEntity()
		{
			// 利用可能なIDが残っているかチェック
			if (m_livingEntityCount >= MAX_ENTITIES)
			{
				// TODO: エラー処理。ログに出力すべき
				throw std::runtime_error("Error: Entity limit reached!");
			}

			// キューから利用可能なEntityIDを取得
			EntityID id = m_availableEntities.front();
			m_availableEntities.pop();
			m_livingEntityCount++;

			// 初期状態ではSignatureをクリアしておく
			m_signatures[id].reset();

			return id;
		}

		/// @brief EntityIDを破棄し、再利用キューに戻す
		void DestroyEntity(EntityID entityID)
		{
			// 妥当なID範囲かチェック
			if (entityID >= MAX_ENTITIES)
			{
				// TODO: エラー処理
				throw std::runtime_error("Error: Invalid entityID for destruction!");
			}

			// Signatureをクリアする
			m_signatures[entityID].reset();

			// IDを再利用キューに戻す
			m_availableEntities.push(entityID);
			m_livingEntityCount--;
		}

		/// @brief EntityのComponent Signatureを設定する
		void SetSignature(EntityID entityID, Signature signature)
		{
			// 妥当なID範囲かチェック
			if (entityID >= MAX_ENTITIES)
			{
				// TODO: エラー処理
				throw std::runtime_error("Error: Invalid entityID for signature setting!");
			}

			m_signatures[entityID] = signature;
		}

		/// @brief EntityのComponent Signatureを取得する
		Signature GetSignature(EntityID entityID) const
		{
			// 妥当なID範囲かチェック
			if (entityID >= MAX_ENTITIES)
			{
				// TODO: エラー処理
				throw std::runtime_error("Error: Invalid entityID for signature retrieval!");
			}

			return m_signatures[entityID];
		}
	};
}

#endif // !___ENTITY_MANAGER_H___