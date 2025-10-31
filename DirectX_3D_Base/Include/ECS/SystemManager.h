/*****************************************************************//**
 * @file	SystemManager.h
 * @brief	ECSのSystemを管理するマネージャクラス。
 * 
 * @details	
 * Systemの登録、Signatureの設定、およびEntityのSignatureが変更された際に
 * Systemへの登録/解除を自動的に行うロジックを実装する。
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

#ifndef ___SYSTEM_MANAGER_H___
#define ___SYSTEM_MANAGER_H___

// ===== インクルード =====
#include "Types.h"
#include <memory>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <typeindex>

namespace ECS
{
	class Coordinator;

	/**
	 * @class System
	 * @brief 全てのSystemの抽象基底クラス。
	 * * SystemはEntityIDのセット（m_entities）を持ち、Coordinatorによって
	 * Entityの追加・削除が通知されることでこのセットが更新される。
	 */
	class System
	{
	public:
		/// @brief このSystemが処理すべきEntityIDの集合
		std::set<EntityID> m_entities;

		/**
		 * @brief Systemの初期化とCoordinatorの依存性注入を行う
		 * @param coordinator - Coordinatorインスタンスへの生ポインタ
		 */
		virtual void Init(Coordinator* coordinator) {}
	};

	/**
	 * @class SystemManager
	 * @brief Systemのインスタンスと、そのSystemが要求するComponent Signatureを管理する。
	 */
	class SystemManager
	{
	private:
		// Systemの型情報（std::type_index）と Systemのインスタンスへのポインタのマッピング
		std::unordered_map<std::type_index, std::shared_ptr<System>> m_systems;

		// Systemの型情報と、そのSystemが要求するComponent Signatureのマッピング
		std::unordered_map<std::type_index, Signature> m_signatures;

	public:
		SystemManager() = default;

		/// @brief Systemを登録し、インスタンスを生成する
		/// @tparam T - 登録するSystemの具象型（Systemクラスを継承していること）
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			std::type_index type = std::type_index(typeid(T));

			if (m_systems.count(type))
			{
				// 既に登録済み
				throw std::runtime_error("Error: System T is already registered!");
			}

			// Systemのインスタンスを生成し、マップに格納
			std::shared_ptr<T> system = std::make_shared<T>();
			m_systems[type] = system;
			return system;
		}

		/// @brief Systemが処理すべきEntityのComponent Signatureを設定する
		/// @tparam T - Systemの具象型
		/// @param signature - このSystemが処理するために必要なComponentのビットマスク
		template<typename T>
		void SetSignature(Signature signature)
		{
			std::type_index type = std::type_index(typeid(T));

			if (!m_systems.count(type))
			{
				throw std::runtime_error("Error: System T not registered before setting signature!");
			}

			// Signatureを登録
			m_signatures[type] = signature;
		}

		/// @brief Entityが作成された、またはComponentが追加/削除された際に呼び出す
		/// @param entityID - 変更があったEntityのID
		/// @param entitySignature - 変更後のEntityのComponent Signature
		void EntitySignatureChanged(EntityID entityID, Signature entitySignature)
		{
			// 全てのSystemに対して、EntityのSignatureがマッチするかチェック
			for (auto const& pair : m_systems)
			{
				// Systemの型を識別
				std::type_index type = pair.first;

				// Systemのインスタンスポインタ
				std::shared_ptr<System> system = pair.second;

				// Systemが要求するSignatureを取得
				Signature systemSignature = m_signatures[type];

				// Systemが処理すべきEntityであるかどうかの判定ロジック
				// EntityのSignatureとSystemの要求するSignatureのAND演算結果が、
				// Systemの要求するSignatureと完全に一致する場合、Systemの処理対象となる。
				// つまり、Systemが必要とするComponentをEntityがすべて持っている。
				if ((entitySignature & systemSignature) == systemSignature)
				{
					// マッチする場合: Systemの処理対象に追加
					system->m_entities.insert(entityID);
				}
				else
				{
					// マッチしない場合: Systemの処理対象から削除（既に登録されていても安全）
					system->m_entities.erase(entityID);
				}
			}
		}

		/// @brief Entityが破棄されたことを全てのSystemに通知する
		void EntityDestroyed(EntityID entityID)
		{
			// 全てのSystemからEntityIDを削除
			// SystemのSignatureにマッチするか否かに関わらず、強制的に削除する
			for (auto const& pair : m_systems)
			{
				pair.second->m_entities.erase(entityID);
			}
		}
	};
}

#endif // !___SYSTEM_MANAGER_H___