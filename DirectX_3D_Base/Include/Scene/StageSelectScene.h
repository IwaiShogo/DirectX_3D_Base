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

#include "Scene/Scene.h"
#include "ECS/Coordinator.h"
#include <memory>

class StageSelectScene : public Scene
{
private:
    std::shared_ptr<ECS::Coordinator> m_coordinator;
    static ECS::Coordinator* s_coordinator;

public:
    StageSelectScene() : m_coordinator(nullptr) {}
    ~StageSelectScene() override {}

    void Init() override;
    void Uninit() override;
    void Update(float deltaTime) override;
    void Draw() override;

    static ECS::Coordinator* GetCoordinator() { return s_coordinator; }
};

#endif