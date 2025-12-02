/*****************************************************************//**
 * @file	StageSelectScene.h
 * @brief	ステージセレクトのメインロジックを含むシーンクラス
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Rituki
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

#ifndef ___STAGE__INFORMATION_SCENE___
#define ___STAGE__INFORMATION_SCENE___

#include "Scene/Scene.h"
#include "ECS/Coordinator.h"
#include <memory>

class StageinformationScene : public Scene
{
private:
    std::shared_ptr<ECS::Coordinator> m_coordinator;
    static ECS::Coordinator* s_coordinator;

public:
    StageinformationScene() : m_coordinator(nullptr) {}
    ~StageinformationScene() override {}

    void Init() override;
    void Uninit() override;
    void Update(float deltaTime) override;
    void Draw() override;

    static ECS::Coordinator* GetCoordinator() { return s_coordinator; }
};

#endif