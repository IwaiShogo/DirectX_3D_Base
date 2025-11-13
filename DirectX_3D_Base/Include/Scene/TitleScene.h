// TitleScene.h
#pragma once

#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h" // 遷移先



class TitleScene : public Scene
{
public:
    TitleScene() = default;
    void Init() override;
    void Uninit() override;
    void Update(float deltaTime) override;
    void Draw() override;

private:
    // このシーンで作成したエンティティのIDリスト（Uninitで破棄するため）
    // std::vector<ECS::EntityID> m_sceneEntities; 
};