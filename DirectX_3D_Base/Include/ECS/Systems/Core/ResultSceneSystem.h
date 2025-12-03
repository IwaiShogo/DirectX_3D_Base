#ifndef ___RESULT_SCENE_SYSTEM_H___
#define ___RESULT_SCENE_SYSTEM_H___

#include "ECS/ECS.h"
#include <ECS/Components/UI/UIButtonComponent.h>
#include "ECS/SystemManager.h"

class ResultSceneSystem : public ECS::System
{

private:
	ECS::Coordinator* m_coordinator = nullptr;

	bool m_isTransitioning = false; // 遷移中フラグ
	float m_transitionTimer = 0.0f; // タイマー
	std::string m_nextSceneTag = ""; // 次のシーン名 (判別用)

	std::map<ECS::EntityID, bool> m_lastHoverStates;


public:

	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}


	void Update(float deltaTime) override;
	std::vector<ECS::EntityID> m_buttons;


};



#endif // !___RESULT_SCENE_SYSTEM_H___