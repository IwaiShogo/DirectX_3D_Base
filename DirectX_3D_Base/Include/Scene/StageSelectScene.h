/*****************************************************************//**
 * @file	StageSelectScene.h
 * @brief	ステージセレクトのメインロジックを含むシーンクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
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

#ifndef ___STAGE_SELECT_SCENE_H___
#define ___STAGE_SELECT_SCENE_H___

// ===== インクルード =====
#include "Scene/Scene.h"
#include "ECS/Coordinator.h"

#include <memory>

/**
 * @class	StageSelectScene
 * @brief	ステージセレクトロジックとECSを管理するシーン
 */
class StageSelectScene
	: public Scene
{
private:
	// ECSの中心となるコーディネーター (シーンがECSのライフサイクルを管理)
	std::shared_ptr<ECS::Coordinator> m_coordinator;

	ECS::EntityID m_selectEntity1 = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_selectEntity2 = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_selectEntity3 = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_selectEntity4 = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_selectEntity5 = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_selectEntity6 = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_cursorEntity = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectA = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_selectB = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectbg = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectcork = ECS::INVALID_ENTITY_ID;

	// ECSのグローバルアクセス用 (SystemなどがECS操作を行うための窓口)
	static ECS::Coordinator* s_coordinator;

public:
	// コンストラクタとデストラクタ（Sceneを継承しているため仮想デストラクタはScene側で定義済みと仮定）
	StageSelectScene()
		: m_coordinator(nullptr)
	{
	}
	~StageSelectScene() override {} // 仮想デストラクタを実装

	// Sceneインターフェースの実装
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	/**
	 * @brief Coordinatorインスタンスへのポインタを取得する静的アクセサ
	 * @return ECS::Coordinator* - 現在アクティブなシーンのCoordinator
	 */
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }
};

#endif // !___STAGE_SELECT_SCENE_H___