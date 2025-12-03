//ResultScene.h
#pragma once
#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
//#include "Scene/TitleScene.h"	//‘JˆÚæ
//#include "Scene/GameScene.h"
#include "ECS/Coordinator.h"
#include <memory>

class ResultScene :public Scene
{
public:
	ResultScene() = default;
	void Init()override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	static bool isClear;
	static bool isCaught;// Œx”õˆõ‚É•ß‚Ü‚Á‚½‚©‚Ìƒtƒ‰ƒO
	static int finalItenCount;
	static ECS::Coordinator* GetCoordinator() { return s_coordinator;}

private:

	std::shared_ptr<ECS::Coordinator> m_coordinator;
	static ECS::Coordinator* s_coordinator;
	

};