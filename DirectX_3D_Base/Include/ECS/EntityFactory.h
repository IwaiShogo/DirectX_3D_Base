/*****************************************************************//**
 * @file	EntityFactory.h
 * @brief	特定のエンティティ（プレイヤー、地面など）の生成ロジックを集約するヘルパークラス。
 *
 * @details
 * Scene::Init()のエンティティ生成コードを分離し、シーンの責務を軽減する。
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：エンティティ生成ロジックを分離するためのクラスを作成
 *
 * @update	2025/11/08	最終更新日
 * 			作業内容：	- 追加：警備員AIの追加
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ENTITY_FACTORY_H___
#define ___ENTITY_FACTORY_H___

 // ===== インクルード =====
#include "Coordinator.h"
#include "Types.h"
#include <DirectXMath.h> // コンポーネントの初期値設定に必要

namespace ECS
{
	/**
	 * @class EntityFactory
	 * @brief Coordinatorを受け取り、定義済みのエンティティ（プリセット）を生成する静的ヘルパー
	 */
	class EntityFactory final
	{
	public:
		/**
		 * @brief 全てのデモ用エンティティを生成し、ECSに登録する
		 * @param coordinator - エンティティの生成と登録を行うCoordinator
		 */
		static void CreateAllDemoEntities(Coordinator* coordinator);

		/**
		 * @brief プレイヤーエンティティを生成する
		 */
		static EntityID CreatePlayer(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		static EntityID CreateGameController(Coordinator* coordinator);

		static EntityID CreateCollectable(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		/**
		 * @brief ゲームワールドの静的な地面エンティティを生成する
		 */
		static EntityID CreateGround(Coordinator* coordinator, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale);

		static EntityID CreateWall(Coordinator* coordinator, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale, const float rotationY);

		static EntityID CreateGoal(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		/**
		* @brief 追跡エンティティを生成する
		*/
		static EntityID CreateGuard(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

	private:
		// 静的クラスのため、プライベートコンストラクタでインスタンス化を禁止
		EntityFactory() = delete;
	};
}

#endif // !___ENTITY_FACTORY_H___