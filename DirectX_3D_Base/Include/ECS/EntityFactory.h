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

		static EntityID CreatePlayer(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		static EntityID CreateCollectable(Coordinator* coordinator, const DirectX::XMFLOAT3& position, int orderIndex = 0, const std::string& itemID = "");

		static EntityID CreateGround(Coordinator* coordinator, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale);

		static EntityID CreateGuard(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		static EntityID CreateWall(Coordinator* coordinator, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale, const float rotationY);

		static EntityID CreateTaser(Coordinator* coordinator, const DirectX::XMFLOAT3& position);
		static EntityID CreateMapGimmick(Coordinator* coordinator, const DirectX::XMFLOAT3& position);



		static EntityID CreateOneShotSoundEntity(Coordinator* coordinator, const std::string& assetID, float volume = 1.0f);

		static EntityID CreateLoopSoundEntity(Coordinator* coordinator, const std::string& assetID, float volume = 1.0f);

		static EntityID CreateTitleSceneEntity(Coordinator* coordinator);

		static void GenerateStageFromConfig(Coordinator* coordinator, const std::string& stageId);

		static EntityID CreateBasicCamera(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		static EntityID CreateOneShotEffect(Coordinator* coordinator, const std::string& assetID, const DirectX::XMFLOAT3& position, float duration, float scale = 1.0f);

		static EntityID CreateDoor(Coordinator* coordinator, const DirectX::XMFLOAT3& position, float rotationY, bool isEntrance);

		static EntityID CreateEnemySpawner(Coordinator* coordinator, const DirectX::XMFLOAT3& position, float delayTime);

		static EntityID CreateTeleporter(ECS::Coordinator* coordinator, DirectX::XMFLOAT3 position);

		static EntityID CreateTopViewTrigger(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		static ECS::EntityID CreateStopTrap(ECS::Coordinator* coordinator, const DirectX::XMFLOAT3& position, float duration);
		static ECS::EntityID CreateCeilingFan(ECS::Coordinator* coordinator, const DirectX::XMFLOAT3& position);
		static ECS::EntityID CreateSecurityCamera(ECS::Coordinator* coordinator, const DirectX::XMFLOAT3& position, float rotationY);
		static ECS::EntityID CreateWallPainting(ECS::Coordinator* coordinator, const DirectX::XMFLOAT3& position, float rotationY, const std::string& modelName);
	private:
		// 静的クラスのため、プライベートコンストラクタでインスタンス化を禁止
		EntityFactory() = delete;
	};
}

#endif // !___ENTITY_FACTORY_H___