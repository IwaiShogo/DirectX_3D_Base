/*****************************************************************//**
 * @file	ComponentManager.h
 * @brief	ECSのComponentを管理するマネージャクラス群を定義。
 * 
 * @details	
 * すべてのComponentを保持する抽象基底クラスと、
 * 個別のComponent型を扱うためのテンプレートクラス、
 * そしてそれらを統合して管理するComponentManagerを定義する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：Componentの抽象ストレージと具象ストレージ、およびComponentManagerクラスを作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COMPONENT_MANAGER_H___
#define ___COMPONENT_MANAGER_H___

#include "Types.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace ECS
{
	/**
	 * @class IComponentArray
	 * @brief Componentの配列（ストレージ）を抽象化するインターフェース
	 * * ComponentManagerが型情報を持たずに操作できるようにするための基底クラス。
	 * EntityIDに対するComponentの追加/削除と、Coordinatorへの通知機能を持つ。
	 */
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		/// @brief 特定のEntityのComponentを削除する（型を問わない）
		virtual void EntityDestroyed(EntityID entityID) = 0;
	};

	/**
	 * @class ComponentArray
	 * @brief 特定のComponent型 T を保持する具象クラス
	 * * Component T のインスタンスを密集した配列（vector）として保持することで、
	 * システム処理時のキャッシュ効率を高める。
	 * Tのインスタンスと、EntityIDの対応付けを管理する。
	 * * @tparam T - 管理するComponentの型
	 */
	template<typename T>
	class ComponentArray : public IComponentArray
	{
	private:
		// Componentインスタンスの密な配列
		std::vector<T> m_componentArray;

		// EntityIDから配列インデックスへのマッピング (Componentの高速参照用)
		std::unordered_map<EntityID, size_t> m_entityToIndexMap;

		// 配列インデックスからEntityIDへのマッピング (Componentの高速削除用)
		std::unordered_map<size_t, EntityID> m_indexToEntityMap;

		// 現在の配列の要素数
		size_t m_size = 0;

	public:
		ComponentArray()
		{
			// MAX_ENTITIESに合わせて予め予約することで、リサイズによるポインタの無効化を防ぐ
			m_componentArray.resize(MAX_ENTITIES);
		}

		/// @brief EntityにComponentを追加し、初期値を設定する
		/// @tparam Args - コンポーネントTのコンストラクタ引数
		template<typename... Args>
		void AddComponent(EntityID entityID, Args&&... args)
		{
			// エンティティが既にコンポーネントを持っている場合は例外を発生させる
			if (m_entityToIndexMap.count(entityID))
			{
				// TODO: エラー処理を実装
				return;
			}

			// 配列の末尾に新しいComponentを配置
			size_t newIndex = m_size;

			// インプレースでコンストラクタ呼び出し
			m_componentArray[newIndex] = T(std::forward<Args>(args)...);

			// マッピングの更新
			m_entityToIndexMap[entityID] = newIndex;
			m_indexToEntityMap[newIndex] = entityID;

			m_size++;
		}

		/// @brief EntityからComponentを削除する
		void RemoveComponent(EntityID entityID)
		{
			// エンティティがコンポーネントを持っていない場合は例外を発生させる
			if (!m_entityToIndexMap.count(entityID))
			{
				// TODO: エラー処理を実装
				return;
			}

			// 削除対象のComponentのインデックスを取得
			size_t indexOfRemoved = m_entityToIndexMap[entityID];

			// 配列の末尾要素を削除対象の位置に移動させる（O(1)の削除を実現）
			// ただし、末尾要素が削除対象自身でない場合のみ
			if (indexOfRemoved != m_size - 1)
			{
				// 最後の要素を削除位置に移動
				EntityID lastEntityID = m_indexToEntityMap[m_size - 1];
				m_componentArray[indexOfRemoved] = m_componentArray[m_size - 1];

				// マッピングの更新
				m_entityToIndexMap[lastEntityID] = indexOfRemoved;
				m_indexToEntityMap[indexOfRemoved] = lastEntityID;
			}

			// 削除対象のEntityのマップ情報をクリア
			m_entityToIndexMap.erase(entityID);
			m_indexToEntityMap.erase(m_size - 1); // 末尾のインデックスを削除

			m_size--;
		}

		/// @brief EntityのComponentを参照として取得する
		T& GetComponent(EntityID entityID)
		{
			// エンティティがコンポーネントを持っていない場合は例外を発生させる
			if (!m_entityToIndexMap.count(entityID))
			{
				// TODO: エラー処理を実装
				// エラーログを出力してから、ダミーのComponentを返す、または例外を投げる
			}

			// EntityIDに対応する配列インデックスを取得し、Componentを参照
			return m_componentArray[m_entityToIndexMap[entityID]];
		}

		/// @brief IComponentArrayのインターフェースを実装 (Entityが破棄された際の処理)
		void EntityDestroyed(EntityID entityID) override
		{
			if (m_entityToIndexMap.count(entityID))
			{
				// エンティティがこのComponentを持っているなら削除する
				RemoveComponent(entityID);
			}
		}

		/// @brief ComponentArray全体へのポインタを取得 (Systemでの効率的なイテレーション用)
		T* GetArrayPointer()
		{
			return m_componentArray.data();
		}

		/// @brief Componentの数を取得
		size_t GetSize() const
		{
			return m_size;
		}

		/// @brief インデックスからEntityIDを取得（System処理中に使用）
		EntityID GetEntityIDFromIndex(size_t index) const
		{
			return m_indexToEntityMap.at(index);
		}
	};

	/**
	 * @class ComponentManager
	 * @brief ComponentArray のインスタンス（型別ストレージ）を管理するクラス
	 * * Coordinatorから呼び出され、Componentの登録、追加、削除の処理を行う。
	 * すべてのComponentArrayを std::map<ComponentTypeID, std::shared_ptr<IComponentArray>> で管理する。
	 */
	class ComponentManager
	{
	private:
		// Componentの型を識別するためのIDカウンター
		ComponentTypeID m_nextComponentTypeID = 0;

		// Componentの型IDと、その型別ストレージへのポインタ（抽象クラス）のマッピング
		std::unordered_map<ComponentTypeID, std::shared_ptr<IComponentArray>> m_componentArrays;

		// Componentの型情報（std::type_index）とComponentTypeIDのマッピング
		std::unordered_map<std::type_index, ComponentTypeID> m_componentTypes;

		// ヘルパー関数: ComponentTypeIDを生成/取得する
		template<typename T>
		ComponentTypeID GetTypeID()
		{
			// C++の標準機能を使って、型 T の一意な識別子を取得
			std::type_index type = std::type_index(typeid(T));

			if (m_componentTypes.find(type) == m_componentTypes.end())
			{
				// 新しい型ならIDを割り当てて登録
				ComponentTypeID newID = m_nextComponentTypeID++;
				m_componentTypes[type] = newID;
				return newID;
			}

			// 既存の型ならIDを返す
			return m_componentTypes[type];
		}

		// ヘルパー関数: 型安全なComponentArrayのポインタを取得
		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			ComponentTypeID typeID = GetTypeID<T>();

			// 存在しない場合はnullptrを返すべきだが、Coordinatorで必ず事前にRegisterComponentTypeが呼ばれる前提とする
			if (m_componentArrays.find(typeID) == m_componentArrays.end())
			{
				// TODO: エラー処理
				//throw std::runtime_error("ComponentType T is not registered!");
			}

			// 抽象ポインタから具象ポインタへのキャスト（ComponentArray<T>へのダウンキャスト）
			return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeID]);
		}

	public:
		ComponentManager() = default;

		/// @brief 新しいComponent型をシステムに登録する
		template<typename T>
		void RegisterComponentType()
		{
			ComponentTypeID typeID = GetTypeID<T>();

			if (m_componentArrays.count(typeID))
			{
				// 既に登録済み
				return;
			}

			// 新しいComponentArrayを生成し、抽象マップに格納
			m_componentArrays[typeID] = std::make_shared<ComponentArray<T>>();
		}

		/// @brief EntityにComponentを追加する
		template<typename T, typename... Args>
		void AddComponent(EntityID entityID, Args&&... args)
		{
			// ComponentArray<T> を取得し、AddComponentを呼び出す
			GetComponentArray<T>()->AddComponent(entityID, std::forward<Args>(args)...);
		}

		/// @brief EntityからComponentを削除する
		template<typename T>
		void RemoveComponent(EntityID entityID)
		{
			// ComponentArray<T> を取得し、RemoveComponentを呼び出す
			GetComponentArray<T>()->RemoveComponent(entityID);
		}

		/// @brief EntityのComponentを参照として取得する
		template<typename T>
		T& GetComponent(EntityID entityID)
		{
			// ComponentArray<T> を取得し、GetComponentを呼び出す
			return GetComponentArray<T>()->GetComponent(entityID);
		}

		/// @brief 特定のComponentのTypeIDを取得する
		template<typename T>
		ComponentTypeID GetComponentTypeID()
		{
			return GetTypeID<T>();
		}

		/// @brief Entityが破棄されたことを全てのComponentArrayに通知する
		void EntityDestroyed(EntityID entityID)
		{
			// 全てのComponentArrayに対してEntityIDを削除するよう通知
			for (auto const& pair : m_componentArrays)
			{
				pair.second->EntityDestroyed(entityID);
			}
		}
	};
}

#endif // !___COMPONENT_MANAGER_H___