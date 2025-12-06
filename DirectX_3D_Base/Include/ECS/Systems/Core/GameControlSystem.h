/*****************************************************************//**
 * @file	GameControlSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/05	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GAME_CONTROL_SYSTEM_H___
#define ___GAME_CONTROL_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Scene/SceneManager.h" // シーン遷移のため
#include <unordered_map>
#include <DirectXMath.h>

/**
 * @class GameControlSystem
 * @brief ゲームオーバー/クリア判定を受け取り、シーン遷移（リトライ/次ステージ）を実行する
 * 処理対象: GameStateComponent を持つ Entity
 */
class GameControlSystem
	: public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;

public:
	void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
        m_iconMap.clear();
        m_topViewEffects.clear();
        m_sonarSpawnTimer = 0.0f;
        m_uiInitialized = false;
        m_timerDigits.clear();
        m_itemHUDs.clear();
    }

	void Update(float deltaTime) override;

private:
    // --- 内部処理用ヘルパー関数 ---
    void UpdateTimerAndRules(float deltaTime, ECS::EntityID controllerID); // 時間・勝敗判定
    void HandleInputAndStateSwitch(ECS::EntityID controllerID);            // 入力・視点切替
    void CheckSceneTransition(ECS::EntityID controllerID);                 // シーン遷移
    void UpdateTopViewUI(ECS::EntityID controllerID);                      // UI更新

    void UpdateIcon(ECS::EntityID target, std::string iconAsset, DirectX::XMFLOAT4 color);
    void UpdateScanLine(float deltaTime, ECS::EntityID controllerID);
    void UpdateSonarEffect(float deltaTime, ECS::EntityID controllerID);
    void InitGameUI();
    void UpdateGameUI(float deltaTime, ECS::EntityID controllerID);

    // 座標変換ヘルパー
    DirectX::XMFLOAT3 GetScreenPosition(
        const DirectX::XMFLOAT3& worldPos,
        const DirectX::XMMATRIX& viewProj
    );

    // スキャンライン接触時のソナー発生処理
    void SpawnSmallSonar(
        const DirectX::XMFLOAT3& screenPos,
        DirectX::XMFLOAT4 color
    );

    // UIアイコン管理用マップ (対象EntityID -> アイコンEntityID)
    std::unordered_map<ECS::EntityID, ECS::EntityID> m_iconMap;
    // グリッドなどのエフェクト用Entity管理
    std::vector<ECS::EntityID> m_topViewEffects;
    float m_sonarSpawnTimer = 0.0f; // ソナー発生間隔管理
    const float SONAR_INTERVAL = 2.0f; // 1秒ごとに発生

    // UI管理用
    bool m_uiInitialized = false;
    std::vector<ECS::EntityID> m_timerDigits; // タイム表示用の桁Entityリスト
    std::vector<ECS::EntityID> m_itemHUDs;    // アイテムHUDのEntityリスト
};

#endif // !___GAME_CONTROL_SYSTEM_H___