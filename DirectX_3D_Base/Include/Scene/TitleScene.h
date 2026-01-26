// TitleScene.h

#ifndef ___TitleScene_H___
#define ___TitleScene_H___

#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"

#include <vector>
#include <memory> // shared_ptr用

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
	ECS::EntityID m_transitionEntity = ECS::INVALID_ENTITY_ID; // タイトル→遷移用フェード(黒)
	enum class NextScene
	{
		None,
		Opening,
		StageSelect
	};
	std::vector<ECS::EntityID> m_loadingEntities;
	bool m_loadingRequested = false;
	bool m_loadingActive = false;
	float m_loadingTimer = 0.0f;
	NextScene m_nextScene = NextScene::None;

	void CreateLoadingUI();
	void SetLoadingVisible(bool visible);
	void StartLoadingTransition(NextScene nextScene, float durationSec);};
#endif //!___TitleScene_H___