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
#include "Scene/SceneManager.h"

void GameFlowSystem::Update()
{
    // GameController Entityを検索
    ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == ECS::INVALID_ENTITY_ID) return;

    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // シーン遷移を一度要求したら、ロジックを停止
    if (state.requestRestart || state.requestNextStage) return;

    // 1. ゲームオーバー時の処理
    if (state.isGameOver)
    {
        // 【ゲームオーバー時の処理】
        // ユーザー要件: 警備員に追いつかれるとゲームオーバー、その後は自動でステージリトライ。
        state.requestRestart = true;

        SceneManager::ChangeScene<GameScene>();
    }

    // 2. クリア時の処理
    if (state.isCleared)
    {
        // 【クリア時の処理】
        // ユーザー要件: ステージセレクト画面に戻り、次のステージが開放される。

        state.requestNextStage = true;

        SceneManager::ChangeScene<GameScene>();
    }

    // 【重要な注意】
    // 実際にこれらのフラグをセットするロジックは、タスク2-2と2-3の完了に依存します。
    // * state.isGameOver = true; は GuardAISystem::Update() で実装されます。
    // * state.isCleared = true; は CollectionSystem::Update() で実装されます。
}