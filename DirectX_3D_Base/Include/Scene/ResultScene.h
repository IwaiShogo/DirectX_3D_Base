// ResultScene.h
#pragma once
#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/TitleScene.h" // ‘JˆÚæ


class ResultScene : public Scene
{
public:
	ResultScene() = default;
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

private:

};