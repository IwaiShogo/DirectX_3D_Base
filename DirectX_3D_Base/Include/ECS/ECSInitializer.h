/*****************************************************************//**
 * @file	ECSInitializer.h
 * @brief	ECSシステム全体の初期化を集約し、シーンのInit()から責務を分離するためのヘルパークラス。
 * 
 * @details	
 * ECSシステム全体の初期化→（コンポーネント / システム登録、シグネチャ設定）
 * このクラスは静的メソッドのみを持ち、インスタンス化されません。
 * どのシーンでも一貫したECS構造を構築するために利用されます。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：ECSの初期化ロジックを分離するためのクラスを作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ECS_INITIALIZER_H___
#define ___ECS_INITIALIZER_H___

// ===== インクルード =====
#include "ECS/Coordinator.h"

#include <memory>

#include <unordered_map>
#include <typeindex>

namespace ECS
{
	/**
	 * @class ECSInitializer
	 * @brief Coordinatorを受け取り、ECSの全体構造を設定する静的ヘルパー
	 */
	class ECSInitializer final
	{
	public:
		// 外部からCoordinatorポインタを受け取り、初期化を実行
		static void InitECS(std::shared_ptr<Coordinator>& coordinator);

		// 外部からCoordinatorポインタを受け取り、破棄処理を実行 (Scene::Uninit()から呼ばれる)
		static void UninitECS();

		/**
		 * @brief 登録済みSystemインスタンスへのSharedPtrを取得する。
		 * @tparam T - 取得したいSystemの型
		 * @return std::shared_ptr<T> - SystemインスタンスのSharedPtr
		 */
		template<typename T>
		static std::shared_ptr<T> GetSystem()
		{
			// 型がSystemのサブクラスであることを保証するアサーションなどを入れると良い
			auto it = s_systems.find(std::type_index(typeid(T)));
			if (it != s_systems.end())
			{
				// マップから取得したSystem基底クラスのSharedPtrを、目的の型にダウンキャストして返す
				return std::static_pointer_cast<T>(it->second);
			}
			return nullptr;



			


		}

	private:
		// コンポーネントの登録のみを行う
		static void RegisterComponents(Coordinator* coordinator);

		// システム登録時、SharedPtrを静的マップに格納するように変更
		static void RegisterSystemsAndSetSignatures(Coordinator* coordinator);

	private:
		// 登録された全システムインスタンスを保持する静的マップ
		// Systemの型をキーとし、SharedPtrを値とする
		static std::unordered_map<std::type_index, std::shared_ptr<System>> s_systems;


	private:

		

	};



}

#define REGISTER_SYSTEM_AND_INIT(coordPtr, TSystem, ...) \
    auto system_##TSystem##_##__LINE__ = (coordPtr)->RegisterSystemWithSignature<TSystem, __VA_ARGS__>(); \
    (system_##TSystem##_##__LINE__)->Init(coordPtr); \
    ECSInitializer::s_systems[std::type_index(typeid(TSystem))] = system_##TSystem##_##__LINE__;

#endif // !___ECS_INITIALIZER_H___