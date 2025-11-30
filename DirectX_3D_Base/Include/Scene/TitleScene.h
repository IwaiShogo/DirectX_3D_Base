// TitleScene.h

#ifndef ___TitleScene_H___
#define ___TitleScene_H___

#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"

#include <vector>
#include <memory> // shared_ptr—p

class TitleScene : public Scene
{
public:
	TitleScene() = default;
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

private:
	std::shared_ptr<ECS::Coordinator> m_coordinator;

	ECS::EntityID m_startButton = ECS::INVALID_ENTITY_ID;
};
#endif //!___TitleScene_H___