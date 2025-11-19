/*****************************************************************//**
 * @file	Coordinator.h
 * @brief	ECSのすべての機能を統合するFacade (窓口) クラス。
 * 
 * @details	
 * EntityManager, ComponentManager, SystemManagerの3つを所有し、
 * 外部からのアクセスを仲介することで、ECSのルールを一貫して適用する。
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

#ifndef ___COORDINATOR_H___
#define ___COORDINATOR_H___

// ===== インクルード =====
#include "Types.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

namespace ECS
{
	/**
	 * @class Coordinator
	 * @brief ECSの中心となるファサードクラス。
	 * * 全てのEntity、Component、Systemの操作は、このCoordinatorを経由して行われる。
	 */
	class Coordinator
	{
	private:
		// 1. ベースケース (終端 - 0個の型パックに対応)
		void SetSignatureHelper(Signature& signature) {}

		// 2. 終端ケース (最後の1つの要素が残ったときに呼ばれる)
		template<typename TLast>
		void SetSignatureHelper(Signature& signature)
		{
			signature.set(GetComponentTypeID<TLast>());
			// テンプレート引数なしで非テンプレートのベースケースを呼び出す
			SetSignatureHelper(signature);
		}

		// 3. 標準的な再帰ケース (2つ以上の要素が残っているときに呼ばれる)
		// ★★★ 修正箇所: TCurrent, TSecond の2つの型を必須とし、曖昧さを回避 ★★★
		template<typename TCurrent, typename TSecond, typename... TRest>
		void SetSignatureHelper(Signature& signature)
		{
			// TCurrent のビットを設定
			signature.set(GetComponentTypeID<TCurrent>());

			// TSecond と TRest... を渡して再帰 (この呼び出しは、(2)または(3)に誘導される)
			SetSignatureHelper<TSecond, TRest...>(signature);
		}

	public:
		Coordinator() = default;

		// マネージャはCoordinatorが責任を持って初期化・管理する
		std::unique_ptr<ComponentManager> m_componentManager;
		std::unique_ptr<EntityManager> m_entityManager;
		std::unique_ptr<SystemManager> m_systemManager;

		/// @brief ECSシステムの初期化
		void Init()
		{
			// 各マネージャの初期化（インスタンス生成）
			m_componentManager = std::make_unique<ComponentManager>();
			m_entityManager = std::make_unique<EntityManager>();
			m_systemManager = std::make_unique<SystemManager>();
		}

		// =================================
		//       Component 管理メソッド
		// =================================

		/// @brief 新しいComponent型を登録する
		template<typename T>
		void RegisterComponentType()
		{
			m_componentManager->RegisterComponentType<T>();
		}

		/// @brief EntityにComponentを追加し、初期値を設定する
		template<typename T, typename... Args>
		void AddComponent(EntityID entityID, Args&&... args)
		{
			// 1. ComponentManagerにComponentの追加を依頼
			m_componentManager->template AddComponent(entityID, std::forward<Args>(args)...);

			// 2. EntityのSignatureを更新
			Signature signature = m_entityManager->GetSignature(entityID);
			signature.set(m_componentManager->GetComponentTypeID<T>());
			m_entityManager->SetSignature(entityID, signature);

			// 3. SystemManagerにSignatureの変更を通知し、Systemへの登録/解除を促す
			m_systemManager->EntitySignatureChanged(entityID, signature);
		}

		template<typename T>
		void AddComponent(EntityID entityID, T&& component)
		{
			// ComponentArray<T> を取得し、ムーブしてAddComponentを呼び出す
			// この呼び出しは ComponentArray<T>::AddComponent<T&&> に繋がる
			m_componentManager->template AddComponent(entityID, std::forward<T>(component));
		}

		/// @brief EntityからComponentを削除する
		template<typename T>
		void RemoveComponent(EntityID entityID)
		{
			// 1. ComponentManagerからComponentの削除を依頼
			m_componentManager->RemoveComponent<T>(entityID);

			// 2. EntityのSignatureを更新
			Signature signature = m_entityManager->GetSignature(entityID);
			signature.reset(m_componentManager->GetComponentTypeID<T>());
			m_entityManager->SetSignature(entityID, signature);

			// 3. SystemManagerにSignatureの変更を通知し、Systemからの解除を促す
			m_systemManager->EntitySignatureChanged(entityID, signature);
		}

		/// @brief EntityのComponentを参照として取得する
		template<typename T>
		T& GetComponent(EntityID entityID)
		{
			return m_componentManager->GetComponent<T>(entityID);
		}

		template<typename T>
		bool HasComponent(EntityID entityID)
		{
			// 1. ComponentのTypeIDを取得
			ComponentTypeID typeID = m_componentManager->GetComponentTypeID<T>(); // ComponentManager.h

			// 2. EntityのSignatureを取得
			Signature entitySignature = m_entityManager->GetSignature(entityID); // EntityManager.h

			// 3. Signatureの対応するビットが立っているかチェック
			// std::bitset::test() を使用して、ComponentTypeIDに対応するビットを確認する
			return entitySignature.test(typeID);
		}

		/// @brief 特定のComponentのTypeIDを取得する
		template<typename T>
		ComponentTypeID GetComponentTypeID()
		{
			return m_componentManager->GetComponentTypeID<T>();
		}

		// =================================
		//       Entity 管理メソッド
		// =================================

		/// @brief 新しいEntityIDを取得する
		EntityID CreateEntity()
		{
			return m_entityManager->CreateEntity();
		}

		// --- 【新規】一括追加ヘルパーの本体（再帰終端関数） ---
		// コンポーネントリストが空になったときに呼ばれる
		void AddComponentsInternal(EntityID entityID)
		{
			// ベースケース：何もしない
		}

		// --- 【新規】一括追加ヘルパーの本体（再帰関数） ---
		// 最初のコンポーネントを追加し、残りを再帰的に処理する
		template<typename T, typename... Rest>
		void AddComponentsInternal(EntityID entityID, T&& component, Rest&&... rest)
		{
			// 1. ComponentManagerにComponentインスタンスの追加を依頼
			// ComponentManagerに実装したオーバーロードを使用
			m_componentManager->template AddComponent<T>(entityID, std::forward<T>(component));

			// 2. EntityのSignatureを更新（AddComponentのロジックをコピー）
			Signature signature = m_entityManager->GetSignature(entityID);
			signature.set(m_componentManager->GetComponentTypeID<T>());
			m_entityManager->SetSignature(entityID, signature);

			// 3. SystemManagerにSignatureの変更を通知
			m_systemManager->EntitySignatureChanged(entityID, signature);

			// 残りのコンポーネントを処理（再帰呼び出し）
			AddComponentsInternal(entityID, std::forward<Rest>(rest)...);
		}

		// --- 【新規】一括生成 + 一括追加のヘルパー関数 (CreateEntityに繋げるイメージ) ---
		/**
		 * @brief Entityを生成し、可変個のコンポーネントインスタンスを一度に追加する。
		 * @tparam Components - 追加するコンポーネントの型リスト。
		 * @param components - コンポーネントのインスタンス。
		 * @return EntityID - 生成された新しいEntityID。
		 */
		template<typename... Components>
		EntityID CreateEntity(Components&&... components)
		{
			EntityID entityID = CreateEntity(); // まずIDを生成

			// 再帰ヘルパー関数を呼び出し、全てのコンポーネントを追加
			AddComponentsInternal(entityID, std::forward<Components>(components)...);

			return entityID;
		}

		/// @brief Entityを破棄する
		void DestroyEntity(EntityID entityID)
		{
			// 1. SystemManagerにEntityの破棄を通知
			m_systemManager->EntityDestroyed(entityID);

			// 2. ComponentManagerにEntityの持つ全てのComponentの削除を通知
			m_componentManager->EntityDestroyed(entityID);

			// 3. EntityManagerにIDの再利用を依頼
			m_entityManager->DestroyEntity(entityID);
		}

		const std::set<EntityID>& GetActiveEntities() const
		{
			return m_entityManager->GetActiveEntities();
		}

		// =================================
		//       System 管理メソッド
		// =================================

		/// @brief Systemを登録し、インスタンスを取得する
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			return m_systemManager->RegisterSystem<T>();
		}

		/// @brief Systemが処理対象とするComponent Signatureを設定する
		template<typename T>
		void SetSystemSignature(Signature signature)
		{
			m_systemManager->SetSignature<T>(signature);
		}
		
		/**
		 * @brief システムを登録し、関連するコンポーネントのシグネチャを同時に設定します。（C++11/14互換の再帰テンプレート版）
		 *
		 * @tparam TSystem 登録するシステム型
		 * @tparam TComponents システムが関心を持つコンポーネント型 (可変引数)
		 * @return 登録されたシステムへのshared_ptr
		 */
		template<typename TSystem, typename... TComponents>
		std::shared_ptr<TSystem> RegisterSystemWithSignature()
		{
			// 1. システムの登録
			std::shared_ptr<TSystem> system = RegisterSystem<TSystem>();

			// 2. シグネチャの構築と設定
			Signature signature;

			// ★ 修正箇所: 再帰ヘルパー関数を呼び出し、型パックを展開させる ★
			SetSignatureHelper<TComponents...>(signature);

			// 3. Coordinatorにシグネチャを適用
			SetSystemSignature<TSystem>(signature);

			return system;
		}

		void UpdateSystems(float deltaTime)
		{
			m_systemManager->UpdateSystems(deltaTime);
		}
	};

	template<typename T>
	EntityID FindFirstEntityWithComponent(Coordinator* coordinator)
	{
		// Coordinatorが有効かチェック
		if (coordinator == nullptr)
		{
			return INVALID_ENTITY_ID;
		}

		// 1. 検索対象のComponentのTypeIDを取得
		ComponentTypeID targetTypeID = coordinator->GetComponentTypeID<T>();

		// 2. 全アクティブEntityを走査
		const auto& allEntities = coordinator->GetActiveEntities();

		for (EntityID entityID : allEntities)
		{
			// 3. EntityのSignatureを取得し、対象Componentを持っているかチェック
			Signature entitySignature = coordinator->m_entityManager->GetSignature(entityID);

			// Signatureのビットフラグをテストし、Componentを持っているか確認
			if (entitySignature.test(targetTypeID))
			{
				// 見つかった最初のEntityIDを返す
				return entityID;
			}
		}
		return INVALID_ENTITY_ID;	// 見つからなかった
	}
}

#endif // !___COORDINATOR_H___
