/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/30	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "Scene/GameScene.h"

#include"Scene/StageinformationScene.h"
#include "Ecs/Components/ScoreManager.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Gameplay/CollectionSystem.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "Systems/Input.h"
#include "ECS/Systems/Core/GameSceneSystem.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
#include <sstream> 

// ===== 静的メンバー変数の定義 =====u
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* GameScene::s_coordinator = nullptr;

using namespace DirectX;
std::string GameScene::s_StageNo = "";
// ===== GameScene メンバー関数の実装 =====

void GameScene::Init()
{
	m_coordinator = std::make_shared<ECS::Coordinator>();

	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

    {
        auto system = m_coordinator->RegisterSystem<GameSceneSystem>();
        ECS::Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<GameSceneComponent>());
        m_coordinator->SetSystemSignature<GameSceneSystem>(signature);
        system->Init(m_coordinator.get());
    }

	// --- 3. JSONコンフィグを使って一撃生成！ ---
	// あなたが作った「一撃関数」に、IDとCoordinatorを渡します
	// ※関数名は実際のコードに合わせて書き換えてください
	ECS::EntityFactory::GenerateStageFromConfig(m_coordinator.get(), s_StageNo);

	// --- 4. その他の共通Entityの作成 ---
	ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());
	ECS::EntityFactory::CreateGameSceneEntity(m_coordinator.get());
}

void GameScene::Uninit()
{
	// 1. ECS System縺ｮ髱咏噪繝ｪ繧ｽ繝ｼ繧ｹ繧定ｧ｣謾ｾ
	ECS::ECSInitializer::UninitECS();

	// Coordinator縺ｮ遐ｴ譽・ｼ・nique_ptr縺瑚・蜍慕噪縺ｫdelete繧貞ｮ溯｡鯉ｼ・
	m_coordinator.reset();

	// 髱咏噪繝昴う繝ｳ繧ｿ繧偵け繝ｪ繧｢
	s_coordinator = nullptr;

	std::cout << "GameScene::Uninit() - ECS Destroyed." << std::endl;
}

void GameScene::Update(float deltaTime)
{

	m_elapsedTime += deltaTime;

	// 2. ゴール判定（とりあえずデバッグ用に 'G' キーでゴール扱いにします）
	// ※後で「プレイヤーがゴールに当たったら true」になるように書き換えてください
	bool isGoal = IsKeyTrigger('G');

	// 3. ゴールした時の処理
	if (isGoal)
	{
		std::cout << "GOAL! Time: " << m_elapsedTime << std::endl;

		// ベストタイムを保存 (現在のステージ番号と、クリアタイム)
		//ScoreManager::SaveBestTime(s_StageNo, m_elapsedTime);

		// リザルト画面（StageinformationScene）へ遷移
		SceneManager::ChangeScene<StageinformationScene>();
		return;
	}

	if (IsKeyTrigger('Q') || IsButtonTriggered(BUTTON_A))
	{
		SceneManager::ChangeScene<GameScene>();
	}

	
	m_coordinator->UpdateSystems(deltaTime);

	if (IsKeyTrigger(VK_SPACE))
	{
		ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST");
	}

}

void GameScene::Draw()
{
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
}