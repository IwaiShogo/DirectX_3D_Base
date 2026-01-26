/*****************************************************************//**
 * @file    GameControlSystem.h
 * @brief   ゲーム本編の進行、演出、UI、ポーズ画面を統括するシステム
 *********************************************************************/

#ifndef ___GAME_CONTROL_SYSTEM_H___
#define ___GAME_CONTROL_SYSTEM_H___

 // ===== インクルード =====
#include "ECS/ECS.h"
#include "Scene/SceneManager.h" 
#include <unordered_map>
#include <DirectXMath.h>
#include <functional>
#include <vector>
#include <string>

/**
 * @class GameControlSystem
 * @brief ゲーム進行、モード切替、UI演出、ポーズ画面制御を行う
 */
class GameControlSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;

public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;

        // コンテナのクリア
        m_iconMap.clear();
        m_topViewEffects.clear();
        m_timerDigits.clear();
        m_itemHUDs.clear();
        m_mosaicTiles.clear();
        m_pauseUIEntities.clear();
        m_btnBgMap.clear();

        // 変数初期化
        m_sonarSpawnTimer = 0.0f;
        m_uiInitialized = false;
        m_catchingGuardID = ECS::INVALID_ENTITY_ID;
        m_caughtAnimPlayed = false;
        m_blackBackID = ECS::INVALID_ENTITY_ID;
        m_uiBreathingTimer = 0.0f;
        m_uiAppearTimer = 0.0f;

        // ポーズ状態リセット
        m_pauseState = PauseState::Hidden;
        m_pendingTransition = nullptr;
        m_pauseTimer = 0.0f;
        m_isDraggingSlider = false;

        // ポーズUI IDリセット
        m_pauseBgOverlayID = ECS::INVALID_ENTITY_ID;
        m_pauseDecoSlashID = ECS::INVALID_ENTITY_ID;
        m_pauseDecoLineID = ECS::INVALID_ENTITY_ID;
        m_pauseItems = {};

        // 演出リセット
        m_crosshair = {};
        m_crosshairSpread = 0.0f;
        m_cinemaBarTop = ECS::INVALID_ENTITY_ID;
        m_cinemaBarBottom = ECS::INVALID_ENTITY_ID;

        // ★追加: 制限フラグ初期化
        m_hasUsedTopView = false;

        // ★追加: サウンド用変数初期化
        m_footstepTimer = 0.0f;
        m_prevCollectedCount = 0;
        m_lastHoveredID = ECS::INVALID_ENTITY_ID;
        m_sliderSoundTimer = 0.0f;
    }

    void Update(float deltaTime) override;

    void UpdateGuardFootsteps(float deltaTime);

    // 外部から「見つかった」状態へ遷移させるトリガー
    void TriggerCaughtSequence(ECS::EntityID guardID);

private:
    // --- 内部処理用ヘルパー関数 ---
    void UpdateTimerAndRules(float deltaTime, ECS::EntityID controllerID); // 時間・勝敗判定
    void HandleInputAndStateSwitch(ECS::EntityID controllerID);            // 入力・視点切替
    void CheckSceneTransition(ECS::EntityID controllerID);                 // シーン遷移
    void UpdateTopViewUI(ECS::EntityID controllerID);                      // トップビューUI更新
    void UpdateCaughtSequence(float deltaTime, ECS::EntityID controllerID);// 捕獲演出

    void UpdateIcon(ECS::EntityID target, std::string iconAsset, DirectX::XMFLOAT4 color);
    void UpdateScanLine(float deltaTime, ECS::EntityID controllerID);
    void UpdateSonarEffect(float deltaTime, ECS::EntityID controllerID);

    // UI関連
    void InitGameUI();
    void UpdateGameUI(float deltaTime, ECS::EntityID controllerID); // ゲーム中UI (TPS)

    // --- MapGimmick (touch to open TopView map) ---
    void CheckMapGimmickTrigger(ECS::EntityID controllerID);
    void ApplyModeVisuals(ECS::EntityID controllerID);
    bool IsAABBOverlap(ECS::EntityID a, ECS::EntityID b);

    // --- ポーズ画面用ステート管理 ---
    enum class PauseState {
        Hidden,     // 非表示
        AnimateIn,  // 出現演出
        Active,     // 操作可能
        AnimateOut, // 退場演出
    };
    PauseState m_pauseState = PauseState::Hidden;
    std::function<void()> m_pendingTransition = nullptr; // 遷移先予約
    float m_pauseTimer = 0.0f;

    // --- ポーズUIエレメントID ---
    ECS::EntityID m_pauseBgOverlayID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID m_pauseDecoSlashID = ECS::INVALID_ENTITY_ID;
    ECS::EntityID m_pauseDecoLineID = ECS::INVALID_ENTITY_ID;

    // メニュー項目
    struct PauseMenuItems {
        ECS::EntityID btnReverse = ECS::INVALID_ENTITY_ID;
        ECS::EntityID btnStage = ECS::INVALID_ENTITY_ID;
        ECS::EntityID btnRetry = ECS::INVALID_ENTITY_ID;
        ECS::EntityID sliderBar = ECS::INVALID_ENTITY_ID;
        ECS::EntityID sliderKnob = ECS::INVALID_ENTITY_ID;
        ECS::EntityID cursor = ECS::INVALID_ENTITY_ID;
    } m_pauseItems;

    // ボタンIDと装飾IDのマップ
    std::unordered_map<ECS::EntityID, ECS::EntityID> m_btnBgMap;
    // ポーズUI全リスト
    std::vector<ECS::EntityID> m_pauseUIEntities;

    bool m_isDraggingSlider = false;

    // --- ポーズ関連関数 ---
    void TogglePauseRequest();
    void UpdatePauseSequence(float deltaTime, ECS::EntityID controllerID);
    void CreateStylePauseUI();
    void DestroyPauseUI();
    void UpdatePauseSliderState();
    ECS::EntityID CreateStyledButton(float x, float y, float w, float h, const std::string& assetID, std::function<void()> onClick);

    // --- 座標変換・演出ヘルパー ---
    DirectX::XMFLOAT3 GetScreenPosition(const DirectX::XMFLOAT3& worldPos, const DirectX::XMMATRIX& viewProj);
    void SpawnSmallSonar(const DirectX::XMFLOAT3& screenPos, DirectX::XMFLOAT4 color);

    // --- 各種シーケンス ---
    void StartEntranceSequence(ECS::EntityID controllerID);
    void UpdateEntranceSequence(float deltaTime, ECS::EntityID controllerID);
    void CheckDoorUnlock(ECS::EntityID controllerID);
    void UpdateExitSequence(float deltaTime, ECS::EntityID controllerID);
    void StartMosaicSequence(ECS::EntityID controllerID);
    void UpdateMosaicSequence(float deltaTime, ECS::EntityID controllerID);

    ECS::EntityID FindEntranceDoor();
    ECS::EntityID FindExitDoor();

    // --- メンバ変数 ---
    std::unordered_map<ECS::EntityID, ECS::EntityID> m_iconMap;
    std::vector<ECS::EntityID> m_topViewEffects;

    float m_sonarSpawnTimer = 0.0f;
    const float SONAR_INTERVAL = 2.0f;

    bool m_uiInitialized = false;
    std::vector<ECS::EntityID> m_timerDigits;
    std::vector<ECS::EntityID> m_itemHUDs;

    ECS::EntityID m_catchingGuardID = ECS::INVALID_ENTITY_ID;
    bool m_caughtAnimPlayed = false;

    std::vector<ECS::EntityID> m_mosaicTiles;
    ECS::EntityID m_blackBackID = ECS::INVALID_ENTITY_ID;

    // --- 演出用変数 (Juice) ---
    float m_uiBreathingTimer = 0.0f; // UIの呼吸/グリッチ用
    float m_uiAppearTimer = 0.0f;    // UI登場アニメーション用

    // 1. タクティカル・クロスヘア (4つのパーツ)
    struct CrosshairParts {
        ECS::EntityID top, bottom, left, right;
    } m_crosshair;
    float m_crosshairSpread = 0.0f; // 現在の広がり具合

    // 2. シネマティック・フレーム (上下の帯 2つだけ)
    ECS::EntityID m_cinemaBarTop = ECS::INVALID_ENTITY_ID;
    ECS::EntityID m_cinemaBarBottom = ECS::INVALID_ENTITY_ID;

    // 制限フラグ
    bool m_hasUsedTopView = false;

    // --- ★追加: 音声制御用変数 ---
    float m_footstepTimer = 0.0f;       // 歩行音の間隔
    int m_prevCollectedCount = 0;       // アイテム取得音判定用
    ECS::EntityID m_lastHoveredID = ECS::INVALID_ENTITY_ID; // カーソル音用
    float m_sliderSoundTimer = 0.0f;    // スライダー音の間隔

    // 関数
    void InitVisualEffects(); // 演出初期化
    void UpdateVisualEffects(float deltaTime, ECS::EntityID controllerID); // 演出更新
    void UpdateDecorations(float deltaTime); // 賑やかしアニメーション
    void UpdateLights(); // ポイントライト更新

    // BGM管理用
    void PlayBGM(const std::string& assetID);
    void StopBGM();
    void PlayStopableSE(const std::string& assetID, float volume);
};

#endif // !___GAME_CONTROL_SYSTEM_H___