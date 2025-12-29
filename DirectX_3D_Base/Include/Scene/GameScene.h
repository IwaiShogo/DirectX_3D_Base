/*****************************************************************//**
 * @file	GameScene.h
 * @brief	Q[̃CWbN܂ރV[NXB
 *
 * @details
 * ECS̏AǗASystem̎s̃V[NXŊB
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date   2025/10/21	쐬
 * 			ƓeF	- ǉFECS::Coordinator̃CX^XRenderSystemւ̃|C^o[ɒǉB
 *						- ǉF̃VXeCoordinatorɃANZX邽߂̐ÓIANZT֐`B
 *
 * @update	2025/xx/xx	ŏIXV
 * 			ƓeF	- XXF
 *
 * @note	iȗj
 *********************************************************************/

#ifndef ___GAME_SCENE_H___
#define ___GAME_SCENE_H___

 // ===== CN[h =====
#include "Scene.h"
#include "ECS/Coordinator.h"

#include <memory>

 /**
  * @class GameScene
  * @brief ۂ̃Q[WbNECSǗV[
  */
class GameScene
	: public Scene
{
private:
	// ECS̒SƂȂR[fBl[^[ (V[ECS̃CtTCNǗ)
	std::shared_ptr<ECS::Coordinator> m_coordinator;

	// ECS̃O[oANZXp (SystemȂǂECSs߂̑)
	static ECS::Coordinator* s_coordinator;

	//UIpϐ
	ECS::EntityID m_completeUIEntity = ECS::INVALID_ENTITY_ID;
	std::vector<ECS::EntityID> m_uiEntities;

	//BGMpϐ
	ECS::EntityID m_bgmScoutID = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_bgmActionID = ECS::INVALID_ENTITY_ID;
	ECS::EntityID m_bgmCompleteID = ECS::INVALID_ENTITY_ID;

	// tF[hCiʏ\j
	bool m_isFadeIn = true;
	float m_fadeTimer = 0.0f;
	float m_fadeInDuration = 0.45f; // フェードイン時間（秒）
	ECS::EntityID m_fadeEntity = ECS::INVALID_ENTITY_ID;


public:
	// RXgN^ƃfXgN^iScenepĂ邽߉zfXgN^SceneŒ`ς݂Ɖj
	GameScene()
		: m_coordinator(nullptr)
	{
	}
	~GameScene() override {} // zfXgN^

	// SceneC^[tF[X̎
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	void UpdateFadeIn(float deltaTime);


	static std::string s_StageNo;
	static void SetStageNo(std::string no) { s_StageNo = no; }
	static std::string GetStageNo() { return s_StageNo; }

	/**
	 * @brief CoordinatorCX^Xւ̃|C^擾ÓIANZT
	 * @return ECS::Coordinator* - ݃ANeBuȃV[Coordinator
	 */
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }
};

#endif // !___GAME_SCENE_H___