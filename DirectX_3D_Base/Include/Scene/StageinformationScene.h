/*****************************************************************//**
 * @file	StageSelectScene.h
 * @brief	ステージセレクトのメインロジックを含むシーンクラス
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Rituki
 * ------------------------------------------------------------
 *
 * @date	2025/11/13	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___STAGE__INFORMATION_SCENE___
#define ___STAGE__INFORMATION_SCENE___

 // ===== インクルード =====
#include "Scene/Scene.h"
#include "ECS/Coordinator.h"

#include <memory>

/**
 * @class	StageSelectScene
 * @brief	ステージセレクトロジックとECSを管理するシーン
 */
class StageinformationScene
	: public Scene
{
public:
	StageinformationScene()
		: m_coordinator(nullptr)
	{
	}
	~StageinformationScene()override {}



	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void UpdateTimeDisplay(float time);
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }

private:
	// ECSの中心となるコーディネーター (シーンがECSのライフサイクルを管理)
	std::shared_ptr<ECS::Coordinator> m_coordinator;

	ECS::EntityID m_cursorEntity = ECS::INVALID_ENTITY_ID;


	ECS::EntityID m_MapImage = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_BestTime = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_Treasure = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_Security = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_OK = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_BACK = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectA = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectB = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_timeDigits[5] = { ECS::INVALID_ENTITY_ID };

	ECS::EntityID m_bg = ECS::INVALID_ENTITY_ID;

	// ECSのグローバルアクセス用 (SystemなどがECS操作を行うための窓口)
	static ECS::Coordinator* s_coordinator;
};

#endif // !___STAGE__INFORMATION_SCENE___