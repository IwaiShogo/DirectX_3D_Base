#ifndef ___OPENINGSCENE_H___
#define ___OPENINGSCENE_H___

#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "include/ECS/Systems/Core/OpeningControlSystem.h"
#include <vector>
#include <memory> // shared_ptr—p

class OpeningScene : public Scene
{
public:
	OpeningScene() = default;
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

private:
	std::shared_ptr<ECS::Coordinator> m_coordinator;
};
#endif //!___OpeningScene_H___