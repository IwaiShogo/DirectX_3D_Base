//TitleScene.h

#ifndef ___TitleScene_H___
#define ___TitleScene_H___

#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"	//推移先

#include "vector"

class TitleScene :public Scene
{
public:
	TitleScene() = default;
	void Init()override;
	void Uninit() override;
	void Update(float deltaTime)override;
	void Draw()override;
private:
	//このシーンで作成したエンティティのIDリスト(Uninitで破棄するため)
	//std;;vector<ECS::EntityID> m_sceneEntities;

	std::shared_ptr<ECS::Coordinator> m_coordinator;

	

	
};
#endif //!___TitleScene_H___
