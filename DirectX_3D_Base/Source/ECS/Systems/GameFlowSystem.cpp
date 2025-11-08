/*****************************************************************//**
 * @file	GameFlowSystem.cpp
 * @brief	ゲームが終了した後の具体的なシーン処理
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/GameFlowSystem.h"

void GameFlowSystem::Update()
{
    // GameController Entityを検索
    ECS::EntityID controllerID;
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (m_coordinator->m_entityManager->GetSignature(entity).test(m_coordinator->GetComponentTypeID<GameStateComponent>()))
        {
            controllerID = entity;
            break;
        }
    }
    if (controllerID == ECS::INVALID_ENTITY_ID) return;

    GameStateComponent& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // 1. ゲームオーバー時の処理
    if (state.isGameOver)
    {
        // 【ゲームオーバー時の処理】
        // ユーザー要件: 警備員に追いつかれるとゲームオーバー、その後は自動でステージリトライ。

        // リトライ要求がまだ出ていなければ、要求を出す
        if (!state.requestRestart)
        {
            state.requestRestart = true;
            // TODO: リトライ要求後にフェードアウト演出などを挟む
        }

        // 仮に即時リトライロジックを実装
        // SceneManagerまたはGameSceneのUninit/Initを呼び出し、ステージをリロード
        // GameScene::s_coordinator->Uninit(); // 既存のECSインスタンスを破棄
        // GameScene::s_coordinator->Init();   // 新しいECSインスタンスでリロード

        // ここではSceneManagerにリトライを指示する関数があると仮定
        // SceneManager::RequestSceneReload(SceneID::GAME_SCENE); 

        // 動作確認のため、シンプルに状態をリセットする処理を仮に記述
        // state.isGameOver = false; 
        // state.currentMode = GameMode::SCOUTING_MODE;
    }

    // 2. クリア時の処理
    if (state.isCleared)
    {
        // 【クリア時の処理】
        // ユーザー要件: ステージセレクト画面に戻り、次のステージが開放される。

        if (!state.requestNextStage)
        {
            state.requestNextStage = true;
            // TODO: クリア演出後に次のステージへの遷移を要求

            // SceneManager::RequestSceneLoad(SceneID::STAGE_SELECT_SCENE);
        }
    }

    // 【重要な注意】
    // 実際にこれらのフラグをセットするロジックは、タスク2-2と2-3の完了に依存します。
    // * state.isGameOver = true; は GuardAISystem::Update() で実装されます。
    // * state.isCleared = true; は CollectionSystem::Update() で実装されます。
}