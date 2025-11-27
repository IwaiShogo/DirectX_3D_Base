/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	ゲームのメインロジックを含むシーンクラスの実装。
 * 
 * @details	
 * ECSの初期化と実行、デモEntityの作成ロジックを内包する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：ECSのライフサイクルとデモロジックを管理する `GameScene` クラスの実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- 警備員AIの追加：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/GameScene.h"

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Gameplay/CollectionSystem.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "Systems/Input.h"

#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerからのRenderSystem取得に使用
 
// ===== 静的メンバー変数の定義 =====u
// 他のシステムからECSにアクセスするための静的ポインタ
ECS::Coordinator* GameScene::s_coordinator = nullptr;

using namespace DirectX;

// ===== GameScene メンバー関数の実装 =====

void GameScene::Init()
{
	// --- 1. ECS Coordinatorの初期化 ---
	m_coordinator = std::make_shared<ECS::Coordinator>();

	// 静的ポインタに現在のCoordinatorを設定
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

	// --- 4. デモ用Entityの作成 ---
	ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());
}

void GameScene::Uninit()
{
	// 1. ECS Systemの静的リソースを解放
	ECS::ECSInitializer::UninitECS();

	// Coordinatorの破棄（unique_ptrが自動的にdeleteを実行）
	m_coordinator.reset();

	// 静的ポインタをクリア
	s_coordinator = nullptr;

	std::cout << "GameScene::Uninit() - ECS Destroyed." << std::endl;
}

void GameScene::Update(float deltaTime)
{
	if (IsKeyTrigger('Q') || IsButtonTriggered(BUTTON_A))
	{
		SceneManager::ChangeScene<GameScene>();
	}

	// ECSの更新
	m_coordinator->UpdateSystems(deltaTime);

	if (IsKeyTrigger(VK_SPACE))
	{
		ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST");
	}

    
    ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator.get());
    
    if (controllerID == ECS::INVALID_ENTITY_ID) return;

    const auto& gameState = m_coordinator->GetComponent<GameStateComponent>(controllerID);
   
    int currentItems = 0;

    if (m_coordinator->m_entityManager->GetSignature(controllerID).test(m_coordinator->GetComponentTypeID<ItemTrackerComponent>()))
    {
        currentItems = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID).collectedItems;
    }

    //BGM管理
    //エンティティ生成
    if (m_bgmScoutID == ECS::INVALID_ENTITY_ID)
    {
        m_bgmScoutID = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST", 0.5f);
        m_coordinator->GetComponent<SoundComponent>(m_bgmScoutID).RequestStop(); // 最初は止めておく
    }
    if (m_bgmActionID == ECS::INVALID_ENTITY_ID)
    {
        m_bgmActionID = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST2", 0.5f);
        m_coordinator->GetComponent<SoundComponent>(m_bgmActionID).RequestStop();
    }

    if (m_bgmCompleteID == ECS::INVALID_ENTITY_ID)
    {
        m_bgmCompleteID = ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TEST3", 0.5f);
        m_coordinator->GetComponent<SoundComponent>(m_bgmCompleteID).RequestStop();
    }

    //再生すべきBGMの決定
    ECS::EntityID targetBGM = ECS::INVALID_ENTITY_ID;

    if (gameState.currentMode == GameMode::SCOUTING_MODE)
    {
        targetBGM = m_bgmScoutID; // 偵察中
    }
    else // ACTION_MODE
    {
        if (currentItems >= 3)
        {
            targetBGM = m_bgmCompleteID; // コンプリート！
        }
        else
        {
            targetBGM = m_bgmActionID; // 通常アクション
        }
    }

    //Scout BGM
    auto& scoutSound = m_coordinator->GetComponent<SoundComponent>(m_bgmScoutID);
    if (targetBGM == m_bgmScoutID) {
        if (!m_isScoutPlaying) { scoutSound.RequestPlay(0.5f, XAUDIO2_LOOP_INFINITE); m_isScoutPlaying = true; }
    }
    else {
        if (m_isScoutPlaying) { scoutSound.RequestStop(); m_isScoutPlaying = false; }
    }

    //Action BGM
    auto& actionSound = m_coordinator->GetComponent<SoundComponent>(m_bgmActionID);
    if (targetBGM == m_bgmActionID) {
        if (!m_isActionPlaying) { actionSound.RequestPlay(0.5f, XAUDIO2_LOOP_INFINITE); m_isActionPlaying = true; }
    }
    else {
        if (m_isActionPlaying) { actionSound.RequestStop(); m_isActionPlaying = false; }
    }

    //Complete BGM
    auto& completeSound = m_coordinator->GetComponent<SoundComponent>(m_bgmCompleteID);
    if (targetBGM == m_bgmCompleteID) {
        if (!m_isCompletePlaying) { completeSound.RequestPlay(0.5f, XAUDIO2_LOOP_INFINITE); m_isCompletePlaying = true; }
    }
    else {
        if (m_isCompletePlaying) { completeSound.RequestStop(); m_isCompletePlaying = false; }
    }

    //アイテムUI表示
    if (controllerID != ECS::INVALID_ENTITY_ID)
    {  
        // ACTION_MODE のとき
        if (gameState.currentMode == GameMode::ACTION_MODE)
        {
            // 画像IDのリスト定義
            const std::vector<std::string> onIDs = { "UI_TEST1", "UI_TEST2", "UI_TEST3" };
            const std::vector<std::string> offIDs = { "UI_TEST1_OFF", "UI_TEST2_OFF", "UI_TEST3_OFF" };

            //UI生成
            if (m_uiEntities.empty())
            {
                for (const auto& assetID : onIDs)
                {
                    ECS::EntityID id = ECS::EntityFactory::CreateUITestEntity(
                        m_coordinator.get(),
                        DirectX::XMFLOAT2(0.0f, 0.0f),
                        DirectX::XMFLOAT2(0.1f, 0.2f),
                        assetID
                    );
                    m_uiEntities.push_back(id);
                }
                std::cout << "All UIs Created!" << std::endl;
            }

             // UI演出
            if (m_coordinator->m_entityManager->GetSignature(controllerID).test(m_coordinator->GetComponentTypeID<ItemTrackerComponent>()))
            {
                ItemTrackerComponent& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
                int currentIndex = tracker.collectedItems;

                // 配置設定
                const float CENTER_X = 0.75f;
                const float SPACING = 0.15f;
                const float BASE_Y = 0.9f;

                // IDリスト
                const std::vector<std::string> onIDs = { "UI_TEST1", "UI_TEST2", "UI_TEST3" };
                const std::vector<std::string> offIDs = { "UI_TEST1_OFF", "UI_TEST2_OFF", "UI_TEST3_OFF" };
                const int totalItems = static_cast<int>(onIDs.size());

                //3つ全部取ったか？
                bool isCompleted = (currentIndex >= onIDs.size());

                for (int i = 0; i < m_uiEntities.size(); ++i)
                {
                    ECS::EntityID uiEntity = m_uiEntities[i];
                    auto& transform = m_coordinator->GetComponent<TransformComponent>(uiEntity);
                    auto& uiImage = m_coordinator->GetComponent<UIImageComponent>(uiEntity);

                    // 安全対策
                    if (i >= onIDs.size()) continue;

                    
                    // 全部取り終わった場合
                    if (isCompleted)
                    {
                        // 画像をOFFに設定
                        uiImage.assetID = offIDs[i];

                        float posX = CENTER_X + ((i - 1) * SPACING);

                        transform.position = DirectX::XMFLOAT3(posX, BASE_Y, 0.0f);
                        transform.scale = DirectX::XMFLOAT3(0.1f, 0.2f, 1.0f);
                        uiImage.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                        uiImage.depth = 0.0f;
                        continue;
                    }

                    int offset = i - currentIndex;

                    // 前後1つより離れている場合は非表示にしてスキップ
                    if (offset < -1 || offset > 1)
                    {
                        transform.scale = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
                        continue;
                    }

                    // 画像切り替え
                    if (i < currentIndex)
                        uiImage.assetID = offIDs[i];
                    else
                        uiImage.assetID = onIDs[i];

                    // 位置計算
                    float targetX = CENTER_X + (offset * SPACING);
                    transform.position = DirectX::XMFLOAT3(targetX, BASE_Y, 0.0f);

                    // サイズ演出
                    if (offset == 0)
                    {
                        transform.scale = DirectX::XMFLOAT3(0.15f, 0.3f, 1.0f); // 大
                        uiImage.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                    }
                    else
                    {
                        transform.scale = DirectX::XMFLOAT3(0.1f, 0.2f, 1.0f); // 小
                        uiImage.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                    }
                }

                if (isCompleted)
                {
                    if (m_completeUIEntity == ECS::INVALID_ENTITY_ID)
                    {

                        m_completeUIEntity = ECS::EntityFactory::CreateUITestEntity(
                            m_coordinator.get(),
                            DirectX::XMFLOAT2(CENTER_X, BASE_Y),
                            DirectX::XMFLOAT2(0.5f, 0.25f),
                            "UI_TEST4"
                        );

                        auto& completeImage = m_coordinator->GetComponent<UIImageComponent>(m_completeUIEntity);
                        completeImage.depth = -0.1f;

                        std::cout << "Complete UI Created!" << std::endl;
                    }
                }
                else
                {
                    if (m_completeUIEntity != ECS::INVALID_ENTITY_ID)
                    {
                        m_coordinator->DestroyEntity(m_completeUIEntity);
                        m_completeUIEntity = ECS::INVALID_ENTITY_ID;
                    }
                }
            }
        }
        //UIを消去
        else
        {
            if (!m_uiEntities.empty())
            {
                for (ECS::EntityID entityID : m_uiEntities)
                {
                    m_coordinator->DestroyEntity(entityID);
                }
                m_uiEntities.clear();

                std::cout << "All UIs Destroyed." << std::endl;
            }
        }
    }
}

void GameScene::Draw()
{
	
	// エンティティの描画
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	// UIの描画
	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
}