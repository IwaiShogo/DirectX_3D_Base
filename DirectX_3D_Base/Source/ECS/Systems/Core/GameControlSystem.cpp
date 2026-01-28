/*****************************************************************//**
 * @file    GameControlSystem.cpp
 * @brief   ゲーム本編の制御実装 (リンクエラー修正・サウンド完全対応版)
 *********************************************************************/

#include "ECS/Systems/Core/GameControlSystem.h"
#include "Scene/SceneManager.h"
#include "ECS/EntityFactory.h"
#include "ECS/ECSInitializer.h"
#include "Scene/ResultScene.h"
#include "Systems/Input.h"
#include "ECS/Components/Gimmick/TeleportComponent.h"  // ★追加: テレポーターのペア判定用
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace DirectX;
using namespace ECS;

// 定数定義
static const int TILE_COLS = 8;
static const int TILE_ROWS = 5;
static const float TILE_ANIM_DELAY = 0.05f;

// アイコンパス取得ヘルパー
std::string GetItemIconPath(const std::string& itemID)
{
    if (itemID == "Takara_Daiya")   return "ICO_TREASURE1";
    if (itemID == "Takara_Crystal") return "ICO_TREASURE2";
    if (itemID == "Takara_Yubiwa")  return "ICO_TREASURE3";
    if (itemID == "Takara_Kaiga1")  return "ICO_TREASURE4";
    if (itemID == "Takara_Kaiga2")  return "ICO_TREASURE5";
    if (itemID == "Takara_Kaiga3")  return "ICO_TREASURE6";
    if (itemID == "Takara_Doki")            return "ICO_TREASURE7";
    if (itemID == "Takara_Tubo_Blue")       return "ICO_TREASURE8";
    if (itemID == "Takara_Tubo_Gouyoku")    return "ICO_TREASURE9";
    if (itemID == "Takara_Dinosaur")        return "ICO_TREASURE10";
    if (itemID == "Takara_Ammonite")        return "ICO_TREASURE11";
    if (itemID == "Takara_Dinosaur_Foot")   return "ICO_TREASURE12";
    return "ICO_TREASURE";
}

// ---------------------------------------------------------
// サウンドヘルパー関数
// ---------------------------------------------------------

// BGM再生 (BGMタグのついた音を全て止めてから再生)
void GameControlSystem::PlayBGM(const std::string& assetID, float volume)
{
    StopBGM();
    // 音量 0.15
    ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, assetID, volume);
}

// 全ての音（BGMとSE含む）を停止
void GameControlSystem::StopBGM()
{
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<SoundComponent>(entity)) continue;
        auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
        sound.RequestStop();
    }
}

// 停止可能なSEを再生する（足音、アラート用）
// シーン遷移時にStopBGM()で止められるようにSoundComponentを使用
void GameControlSystem::PlayStopableSE(const std::string& assetID, float volume)
{
    // 重複再生を防ぐ（足音などが重なりすぎないようにする簡易制御）
    // 必要であればこのチェックは外してください
    /*
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<SoundComponent>(entity)) continue;
        auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
        if (sound.assetID == assetID && sound.isPlaying) return;
    }
    */

    EntityID entity = m_coordinator->CreateEntity(
        TagComponent("SE"),
        SoundComponent(assetID, SoundType::SE, volume, 0) // Loop=0 (1回再生)
    );
    // 再生要求
    m_coordinator->GetComponent<SoundComponent>(entity).RequestPlay(volume, 0);
}

// ==================================================================================
//  Update メインループ
// ==================================================================================
void GameControlSystem::Update(float deltaTime)
{
    EntityID controllerID = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == INVALID_ENTITY_ID) return;
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_START)) {
        TogglePauseRequest();
    }

    if (m_pauseState != PauseState::Hidden) {
        UpdatePauseSequence(deltaTime, controllerID);
        state.isPaused = true;
        return;
    }
    else {
        state.isPaused = false;
    }

    switch (state.sequenceState) {
    case GameSequenceState::Starting: UpdateMosaicSequence(deltaTime, controllerID); return;
    case GameSequenceState::Entering: UpdateEntranceSequence(deltaTime, controllerID); return;
    case GameSequenceState::Exiting:  UpdateExitSequence(deltaTime, controllerID); return;
    case GameSequenceState::Caught:   UpdateCaughtSequence(deltaTime, controllerID); return;
    default: break;
    }

    if (!m_uiInitialized) {
        InitGameUI();
        m_uiInitialized = true;
        if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    }

    CheckMapGimmickTrigger(controllerID);
    HandleInputAndStateSwitch(controllerID);
    UpdateTimerAndRules(deltaTime, controllerID);
    CheckSceneTransition(controllerID);

    if (!state.isGameOver && !state.isGameClear) {
        UpdateTopViewUI(controllerID);
        UpdateScanLine(deltaTime, controllerID);
        UpdateSonarEffect(deltaTime, controllerID);
        UpdateGameUI(deltaTime, controllerID);
        CheckDoorUnlock(controllerID);
        UpdateDecorations(deltaTime);
        UpdateLights();
        UpdateTeleportEffects(controllerID); // ★追加: テレポートエフェクト更新

        // 警備員の足音制御
        UpdateGuardFootsteps(deltaTime);
    }

    if (state.sequenceState == GameSequenceState::Playing && !state.isGameOver) {
        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        EntityID exitDoorID = FindExitDoor();
        if (playerID != INVALID_ENTITY_ID && exitDoorID != INVALID_ENTITY_ID) {
            auto& door = m_coordinator->GetComponent<DoorComponent>(exitDoorID);
            if (door.state == DoorState::Open) {
                auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
                auto& dTrans = m_coordinator->GetComponent<TransformComponent>(exitDoorID);
                float distSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&pTrans.position) - XMLoadFloat3(&dTrans.position)));
                if (distSq < 4.0f) {
                    state.sequenceState = GameSequenceState::Exiting;
                    state.sequenceTimer = 0.0f;
                    if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) {
                        XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);
                        float rad = dTrans.rotation.y;
                        XMVECTOR camPosVec = doorPos + (XMVectorSet(sin(rad), 0, cos(rad), 0) * 3.0f) + XMVectorSet(0, 2, 0, 0);
                        XMVECTOR lookAtVec = doorPos + XMVectorSet(0, 1.5f, 0, 0);
                        XMFLOAT3 camPos{};
                        XMFLOAT3 lookAt{};
                        ::DirectX::XMStoreFloat3(&camPos, camPosVec);
                        ::DirectX::XMStoreFloat3(&lookAt, lookAtVec);
                        camSys->SetFixedCamera(camPos, lookAt);
                    }
                    StopBGM();
                    // SE_CLEAR (0.4f)
                    EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_CLEAR", 0.4f);
                }
            }
        }
    }
}

// 警備員の足音制御
void GameControlSystem::UpdateGuardFootsteps(float deltaTime)
{
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;
    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
        if (m_coordinator->GetComponent<TagComponent>(entity).tag != "guard") continue;
        if (!m_coordinator->HasComponent<TransformComponent>(entity)) continue;
        if (!m_coordinator->HasComponent<RigidBodyComponent>(entity)) continue;

        auto& gTrans = m_coordinator->GetComponent<TransformComponent>(entity);
        auto& rb = m_coordinator->GetComponent<RigidBodyComponent>(entity);

        float speedSq = rb.velocity.x * rb.velocity.x + rb.velocity.z * rb.velocity.z;
        if (speedSq > 0.1f) { // 動いているか
            float distSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&pTrans.position) - XMLoadFloat3(&gTrans.position)));
            float maxDist = 15.0f;
            float dist = sqrt(distSq);
            if (dist < maxDist) {
                // 距離による音量減衰
                float volume = 1.0f - (dist / maxDist);
                volume = std::max(0.0f, volume * 0.5f); // 最大音量0.5

                // ランダム再生 (約1秒に1回)
                if (rand() % 60 == 0) {
                    // ★修正: 停止可能なSEとして再生 (シーン遷移で消えるように)
                    PlayStopableSE("SE_RUN", volume);
                }
            }
        }
    }
}

void GameControlSystem::TriggerCaughtSequence(ECS::EntityID guardID)
{
    EntityID controllerID = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == INVALID_ENTITY_ID) return;
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    if (state.sequenceState == GameSequenceState::Caught || state.isGameOver) return;

    state.sequenceState = GameSequenceState::Caught;
    state.sequenceTimer = 0.0f;
    m_catchingGuardID = guardID;
    m_caughtAnimPlayed = false;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID && guardID != INVALID_ENTITY_ID) {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& gTrans = m_coordinator->GetComponent<TransformComponent>(guardID);
        if (m_coordinator->HasComponent<AnimationComponent>(guardID))
            m_coordinator->GetComponent<AnimationComponent>(guardID).Play("A_GUARD_RUN");
        if (m_coordinator->HasComponent<AnimationComponent>(playerID))
            m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_IDLE");

        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) {
            XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
            XMVECTOR gPos = XMLoadFloat3(&gTrans.position);
            XMVECTOR midPoint = (pPos + gPos) * 0.5f;
            XMVECTOR camOffset = XMVectorSet(2.0f, 2.5f, -3.0f, 0.0f);
            XMVECTOR camPosVec = midPoint + camOffset;
            XMVECTOR lookAtVec = midPoint;
            XMFLOAT3 camPos{};
            XMFLOAT3 lookAt{};
            ::DirectX::XMStoreFloat3(&camPos, camPosVec);
            ::DirectX::XMStoreFloat3(&lookAt, lookAtVec);
            camSys->SetFixedCamera(camPos, lookAt);
        }

        // ★修正: 停止可能なSEとして再生 (SE_ALERT)
        PlayStopableSE("SE_ALERT", 0.5f);
    }
}

// ---------------------------------------------------------
// UI初期化
// ---------------------------------------------------------
void GameControlSystem::InitGameUI()
{
    for (auto id : m_timerDigits) m_coordinator->DestroyEntity(id);
    for (auto id : m_itemHUDs) m_coordinator->DestroyEntity(id);
    m_timerDigits.clear();
    m_itemHUDs.clear();

    m_uiBreathingTimer = 0.0f;
    m_uiAppearTimer = 0.0f;
    m_footstepTimer = 0.0f;
    m_prevCollectedCount = 0;
    m_lastHoveredID = INVALID_ENTITY_ID;

    float startX = 50.0f;
    float startY = 100.0f;
    float w = 30.0f;
    float h = 50.0f;

    for (int i = 0; i < 7; ++i) {
        EntityID digit = m_coordinator->CreateEntity(
            TransformComponent({ startX + i * w, startY, 0.0f }, { 0,0,0 }, { w, h, 1.0f }),
            UIImageComponent("UI_FONT", 0.0f, true, { 1, 1, 1, 1 })
        );
        m_timerDigits.push_back(digit);
    }

    InitVisualEffects();

    // BGM初期化 (トップビューBGM)
    PlayBGM("BGM_TOPVIEW", 0.7f);
}

void GameControlSystem::InitVisualEffects()
{
    auto SafeDestroy = [&](ECS::EntityID& id) {
        if (id != INVALID_ENTITY_ID && m_coordinator->HasComponent<TransformComponent>(id)) {
            m_coordinator->DestroyEntity(id);
        }
        id = INVALID_ENTITY_ID;
        };
    SafeDestroy(m_crosshair.top); SafeDestroy(m_crosshair.bottom);
    SafeDestroy(m_crosshair.left); SafeDestroy(m_crosshair.right);
    SafeDestroy(m_cinemaBarTop); SafeDestroy(m_cinemaBarBottom);

    // クロスヘア削除

    m_cinemaBarTop = m_coordinator->CreateEntity(
        TransformComponent({ 0,0,0 }, { 0,0,0 }, { 1,1,1 }),
        UIImageComponent("FADE_WHITE", 4.0f, true, { 0.0f, 0.0f, 0.0f, 1.0f })
    );
    m_cinemaBarBottom = m_coordinator->CreateEntity(
        TransformComponent({ 0,0,0 }, { 0,0,0 }, { 1,1,1 }),
        UIImageComponent("FADE_WHITE", 4.0f, true, { 0.0f, 0.0f, 0.0f, 1.0f })
    );
}

// ---------------------------------------------------------
// UI更新 (演出含む)
// ---------------------------------------------------------
void GameControlSystem::UpdateGameUI(float deltaTime, ECS::EntityID controllerID)
{
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    m_uiBreathingTimer += deltaTime;
    m_uiAppearTimer += deltaTime;

    float slideT = std::min(1.0f, m_uiAppearTimer / 0.6f);
    float slideEase = 1.0f - powf(1.0f - slideT, 3.0f);
    float slideOffset = (1.0f - slideEase) * -200.0f;

    float time = state.elapsedTime;
    int minutes = (int)(time / 60.0f);
    int seconds = (int)(time) % 60;
    int deciseconds = (int)((time - (int)time) * 10.0f);
    int indices[7];
    indices[0] = (minutes / 10) % 10; indices[1] = minutes % 10; indices[2] = 10;
    indices[3] = (seconds / 10) % 10; indices[4] = seconds % 10; indices[5] = 11;
    indices[6] = deciseconds % 10;

    const float UV_X = 1.0f / 5.0f;
    const float UV_Y = 1.0f / 3.0f;
    float startX = 50.0f, startY = 100.0f, w = 30.0f;

    for (int i = 0; i < 7; ++i) {
        if (i >= m_timerDigits.size()) break;
        if (!m_coordinator->HasComponent<UIImageComponent>(m_timerDigits[i])) continue;

        auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_timerDigits[i]);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(m_timerDigits[i]);
        int idx = indices[i];
        int row = (idx <= 9) ? idx / 5 : 2;
        int col = (idx <= 9) ? idx % 5 : (idx == 10 ? 1 : (idx == 11 ? 2 : 0));
        ui.uvPos = { col * UV_X, row * UV_Y }; ui.uvScale = { UV_X, UV_Y };

        bool isGlitch = (rand() % 100 < 1);
        float baseX = startX + i * w + slideOffset;
        if (isGlitch) {
            trans.position = { baseX + (float)(rand() % 5 - 2), startY + (float)(rand() % 5 - 2), 0.0f };
            ui.color = (rand() % 2 == 0) ? XMFLOAT4(0, 1, 1, 0.9f) : XMFLOAT4(1, 0, 0.5f, 0.9f);
        }
        else {
            trans.position = { baseX, startY, 0.0f };
            ui.color = { 1, 1, 1, 1 };
        }
    }

    if (m_coordinator->HasComponent<ItemTrackerComponent>(controllerID)) {
        auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
        if (tracker.collectedItems > m_prevCollectedCount) {
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_STEAL", 0.4f);
            m_prevCollectedCount = tracker.collectedItems;
        }

        size_t total = tracker.targetItemIDs.size();
        if (m_itemHUDs.size() < total) {
            float hX = SCREEN_WIDTH - 60.0f, hY = 100.0f, gY = 60.0f;
            for (size_t i = m_itemHUDs.size(); i < total; ++i) {
                std::string path = GetItemIconPath(tracker.targetItemIDs[i]);
                EntityID hud = m_coordinator->CreateEntity(
                    TransformComponent({ hX, hY + i * gY, 0.0f }, { 0,0,0 }, { 50, 50, 1.0f }),
                    UIImageComponent(path, 0.0f, true, { 1, 1, 1, 1 })
                );
                m_itemHUDs.push_back(hud);
            }
        }
        float hudSlide = (1.0f - slideEase) * 200.0f;
        float breath = 1.0f + 0.05f * sinf(m_uiBreathingTimer * 2.5f);
        for (size_t i = 0; i < m_itemHUDs.size(); ++i) {
            if (i >= tracker.targetItemIDs.size()) break;
            if (!m_coordinator->HasComponent<UIImageComponent>(m_itemHUDs[i])) continue;
            auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_itemHUDs[i]);
            auto& trans = m_coordinator->GetComponent<TransformComponent>(m_itemHUDs[i]);
            std::string targetID = tracker.targetItemIDs[i];
            bool collected = true;
            for (auto const& e : m_coordinator->GetActiveEntities()) {
                if (m_coordinator->HasComponent<CollectableComponent>(e)) {
                    auto& col = m_coordinator->GetComponent<CollectableComponent>(e);
                    if (col.itemID == targetID && !col.isCollected) { collected = false; break; }
                }
            }
            float bX = (SCREEN_WIDTH - 60.0f) + hudSlide;
            float bY = 100.0f + i * 60.0f;
            trans.position = { bX, bY, 0.0f };
            if (collected) {
                ui.color = { 1, 1, 1, 1 }; trans.scale = { 50 * breath, 50 * breath, 1 };
            }
            else {
                ui.color = { 0.3f, 0.3f, 0.3f, 0.5f }; trans.scale = { 45, 45, 1 };
                if (tracker.useOrderedCollection && (int)i == (tracker.currentTargetOrder - 1)) {
                    float flash = 0.7f + 0.3f * sinf(m_uiBreathingTimer * 12.0f);
                    ui.color = { 1, 1, 0.5f, flash };
                    trans.scale = { 50 * (1.0f + 0.15f * powf(sinf(m_uiBreathingTimer * 6.0f), 10.0f)), 50, 1 };
                }
            }
        }
    }

    UpdateVisualEffects(deltaTime, controllerID);
}

void GameControlSystem::UpdateVisualEffects(float deltaTime, ECS::EntityID controllerID)
{
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // 入力チェック
    bool isInputMoving = (IsKeyPress('W') || IsKeyPress('A') || IsKeyPress('S') || IsKeyPress('D'));
    XMFLOAT2 stick = GetLeftStick();
    if (abs(stick.x) > 0.1f || abs(stick.y) > 0.1f) isInputMoving = true;

    // 速度チェック
    bool isVelocityMoving = false;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID && m_coordinator->HasComponent<RigidBodyComponent>(playerID)) {
        auto& rb = m_coordinator->GetComponent<RigidBodyComponent>(playerID);
        float speedSq = rb.velocity.x * rb.velocity.x + rb.velocity.z * rb.velocity.z;
        if (speedSq > 0.1f) isVelocityMoving = true;
    }

    bool isAction = (state.currentMode == GameMode::ACTION_MODE) &&
        (state.sequenceState == GameSequenceState::Playing);

    if (isInputMoving && isVelocityMoving && isAction) {
        m_footstepTimer += deltaTime;
        if (m_footstepTimer > 0.4f) {
            // ★修正: 停止可能なSEとして再生 (SE_RUN)
            PlayStopableSE("SE_RUN", 0.3f);
            m_footstepTimer = 0.0f;
        }
    }
    else {
        m_footstepTimer = 0.4f;
    }

    float targetHeight = 0.0f;
    if (state.currentMode == GameMode::SCOUTING_MODE) {
        targetHeight = 0.0f;
    }
    else {
        targetHeight = isInputMoving ? 30.0f : 80.0f;
    }

    auto UpdateBar = [&](ECS::EntityID id, bool isTop) {
        if (id == INVALID_ENTITY_ID) return;
        if (!m_coordinator->HasComponent<TransformComponent>(id)) return;
        auto& trans = m_coordinator->GetComponent<TransformComponent>(id);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);

        float currentH = trans.scale.y;
        float newH = currentH + (targetHeight - currentH) * 5.0f * deltaTime;

        trans.scale = { (float)SCREEN_WIDTH, newH, 1.0f };
        if (isTop) trans.position = { SCREEN_WIDTH * 0.5f, newH * 0.5f, 0.0f };
        else       trans.position = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - (newH * 0.5f), 0.0f };

        ui.isVisible = (newH > 1.0f);
        };
    UpdateBar(m_cinemaBarTop, true);
    UpdateBar(m_cinemaBarBottom, false);
}

// ---------------------------------------------------------
// ポーズ関連
// ---------------------------------------------------------
void GameControlSystem::TogglePauseRequest() {
    if (m_pauseState == PauseState::Hidden) {
        m_pauseState = PauseState::AnimateIn; m_pauseTimer = 0.0f;
        CreateStylePauseUI();
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_POUSE", 0.5f);
    }
    else if (m_pauseState == PauseState::Active) {
        m_pauseState = PauseState::AnimateOut; m_pauseTimer = 0.0f;
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_BACK", 0.5f);
    }
}

ECS::EntityID GameControlSystem::CreateStyledButton(float targetX, float targetY, float w, float h, const std::string& assetID, std::function<void()> onClick) {
    float tiltRad = XMConvertToRadians(10.0f);
    float plateW = w + 120.0f;
    float plateH = h + 10.0f;
    ECS::EntityID bgID = m_coordinator->CreateEntity(
        TransformComponent({ targetX - 500.0f, targetY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { plateW, plateH, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.5f, true, { 0.1f, 0.1f, 0.1f, 0.9f })
    );
    m_pauseUIEntities.push_back(bgID);
    ECS::EntityID lineID = m_coordinator->CreateEntity(
        TransformComponent({ targetX - 500.0f, targetY + (h * 0.5f) + 5.0f, 0.0f }, { 0.0f, 0.0f, tiltRad }, { plateW - 20.0f, 3.0f, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.6f, true, { 0.5f, 0.5f, 0.5f, 0.0f })
    );
    m_pauseUIEntities.push_back(lineID);
    ECS::EntityID btnID = m_coordinator->CreateEntity(
        TransformComponent({ targetX - 600.0f, targetY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { w, h, 1.0f }),
        // ★修正: 色を {1,1,1,1} に設定して画像を表示
        UIImageComponent(assetID, 12.0f, true, { 1.0f, 1.0f, 1.0f, 1.0f }),
        UIButtonComponent(ButtonState::Normal, true, onClick)
    );
    m_pauseUIEntities.push_back(btnID);
    m_btnBgMap[btnID] = bgID;
    return btnID;
}

// ---------------------------------------------------------------------
// 1. CreateStylePauseUI (CAMERAラベル追加)
// ---------------------------------------------------------------------
void GameControlSystem::CreateStylePauseUI() {
    m_pauseUIEntities.clear(); m_btnBgMap.clear();
    m_lastHoveredID = INVALID_ENTITY_ID;

    float tiltRad = XMConvertToRadians(12.0f);

    // --- 1. 背景オーバーレイ ---
    m_pauseBgOverlayID = m_coordinator->CreateEntity(
        TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }),
        UIImageComponent("FADE_WHITE", 10.0f, true, { 0.0f, 0.0f, 0.0f, 0.0f })
    );
    m_pauseUIEntities.push_back(m_pauseBgOverlayID);

    // --- 2. 背景帯 ---
    m_pauseDecoSlashID = m_coordinator->CreateEntity(
        TransformComponent({ -SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0.0f, 0.0f, tiltRad }, { SCREEN_WIDTH * 0.55f, SCREEN_HEIGHT * 2.5f, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.0f, true, { 0.02f, 0.02f, 0.05f, 0.95f })
    );
    m_pauseUIEntities.push_back(m_pauseDecoSlashID);

    // --- 3. 幾何学模様 ---
    for (int i = 0; i < 15; ++i) {
        float rndW = (float)(rand() % 300 + 50);
        float rndH = (float)(rand() % 5 + 1);
        float rndY = (float)(rand() % SCREEN_HEIGHT);
        float rndX = (float)(rand() % 400 - 200);
        float alpha = (float)(rand() % 10) * 0.01f + 0.02f;
        ECS::EntityID pattern = m_coordinator->CreateEntity(
            TransformComponent({ rndX, rndY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { rndW, rndH, 1.0f }),
            UIImageComponent("FADE_WHITE", 11.05f, true, { 1.0f, 1.0f, 1.0f, alpha })
        );
        m_pauseUIEntities.push_back(pattern);
    }
    ECS::EntityID frame = m_coordinator->CreateEntity(
        TransformComponent({ SCREEN_WIDTH * 0.15f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 450.0f, SCREEN_HEIGHT * 0.9f, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.06f, true, { 0.0f, 1.0f, 1.0f, 0.05f })
    );
    m_pauseUIEntities.push_back(frame);

    // --- 4. アクセントライン ---
    m_pauseDecoLineID = m_coordinator->CreateEntity(
        TransformComponent({ -SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 4.0f, SCREEN_HEIGHT * 2.5f, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.1f, true, { 0.0f, 1.0f, 1.0f, 1.0f })
    );
    m_pauseUIEntities.push_back(m_pauseDecoLineID);

    // --- 5. 巨大ポーズシンボル "||" ---
    {
        float symH = SCREEN_HEIGHT * 0.6f;
        float symW = 60.0f;
        float symX = SCREEN_WIDTH * 0.1f;
        float symY = SCREEN_HEIGHT * 0.5f;

        ECS::EntityID bar1 = m_coordinator->CreateEntity(
            TransformComponent({ symX, symY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { symW, symH, 1.0f }),
            UIImageComponent("FADE_WHITE", 11.08f, true, { 1.0f, 1.0f, 1.0f, 0.08f })
        );
        m_pauseUIEntities.push_back(bar1);

        ECS::EntityID bar2 = m_coordinator->CreateEntity(
            TransformComponent({ symX + 90.0f, symY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { symW, symH, 1.0f }),
            UIImageComponent("FADE_WHITE", 11.08f, true, { 1.0f, 1.0f, 1.0f, 0.08f })
        );
        m_pauseUIEntities.push_back(bar2);
    }

    // --- 6. セレクター (黄色い長方形・回転演出あり) ---
    ECS::EntityID selector = m_coordinator->CreateEntity(
        TransformComponent({ -200.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 30.0f, 30.0f, 1.0f }),
        UIImageComponent("FADE_WHITE", 12.5f, true, { 1.0f, 1.0f, 0.0f, 1.0f })
    );
    ECS::EntityID selectorOuter = m_coordinator->CreateEntity(
        TransformComponent({ -200.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 40.0f, 40.0f, 1.0f }),
        UIImageComponent("FADE_WHITE", 12.4f, true, { 1.0f, 1.0f, 0.0f, 0.3f })
    );
    m_pauseUIEntities.push_back(selector);
    m_pauseUIEntities.push_back(selectorOuter);

    // --- 7. メニューボタン生成 ---
    float screenRefX = SCREEN_WIDTH * 0.22f;
    float baseY = SCREEN_HEIGHT * 0.25f;
    float gapY = 130.0f;

    auto GetXForY = [&](float y) {
        return screenRefX - ((y - SCREEN_HEIGHT * 0.5f) * tanf(tiltRad));
        };

    float baseH = 50.0f;

    // Buttons
    float w1 = baseH * 4.0f; float y1 = baseY;
    m_pauseItems.btnReverse = CreateStyledButton(GetXForY(y1), y1, w1, baseH, "BTN_BACK_POSE", [this]() {
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_BACK", 0.5f);
        m_pendingTransition = nullptr; m_pauseState = PauseState::AnimateOut;
        });

    float w2 = baseH * 4.5f; float y2 = baseY + gapY;
    m_pauseItems.btnRetry = CreateStyledButton(GetXForY(y2), y2, w2, baseH, "BTN_RETRY_POSE", [this]() {
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DECISION", 0.5f);
        m_pendingTransition = []() { SceneManager::ChangeScene<GameScene>(); }; m_pauseState = PauseState::AnimateOut;
        });

    float w3 = baseH * 4.5f; float y3 = baseY + gapY * 2.0f;
    m_pauseItems.btnStage = CreateStyledButton(GetXForY(y3), y3, w3, baseH, "BTN_SELECT_POSE", [this]() {
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DECISION", 0.5f);
        m_pendingTransition = []() { SceneManager::ChangeScene<StageSelectScene>(); }; m_pauseState = PauseState::AnimateOut;
        });

    // Slider
    float sliderY = baseY + gapY * 3.3f;
    float sliderX = GetXForY(sliderY);

    // ★追加: [CAMERA] ラベル
    // スライダーの少し上 (Y - 40) に配置
    {
        float lblY = sliderY - 40.0f;
        float lblX = GetXForY(lblY);
        // 画像サイズ 1700x320 -> アスペクト比 約 5.31
        // 幅を 130 くらいにすると 高さは 24 くらい
        ECS::EntityID camLabel = m_coordinator->CreateEntity(
            TransformComponent({ lblX, lblY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 130.0f, 24.0f, 1.0f }),
            UIImageComponent("UI_CAMERA_POSE", 12.2f, true, { 1.0f, 1.0f, 1.0f, 1.0f })
        );
        m_pauseUIEntities.push_back(camLabel);
    }

    m_pauseItems.sliderBar = m_coordinator->CreateEntity(
        TransformComponent({ sliderX, sliderY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 350, 6, 1 }),
        UIImageComponent("FADE_WHITE", 12.0f, true, { 0.4f, 0.4f, 0.4f, 1.0f })
    );
    m_pauseUIEntities.push_back(m_pauseItems.sliderBar);

    m_pauseItems.sliderKnob = m_coordinator->CreateEntity(
        TransformComponent({ sliderX, sliderY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 16, 28, 1 }),
        UIImageComponent("FADE_WHITE", 12.1f, true, { 0.0f, 1.0f, 1.0f, 1.0f })
    );
    m_pauseUIEntities.push_back(m_pauseItems.sliderKnob);

    // Cursor
    m_pauseItems.cursor = m_coordinator->CreateEntity(
        TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { 64, 64, 1 }),
        UIImageComponent("ICO_CURSOR", 20.0f, true, { 1, 1, 1, 1 }),
        UICursorComponent(8.0f)
    );
    m_pauseUIEntities.push_back(m_pauseItems.cursor);
}

// ---------------------------------------------------------------------
// 2. UpdatePauseSequence (ラベルのアニメーション対応)
// ---------------------------------------------------------------------
void GameControlSystem::UpdatePauseSequence(float deltaTime, ECS::EntityID controllerID) {
    m_pauseTimer += deltaTime;
    float tiltRad = XMConvertToRadians(12.0f);
    const float ANIM_IN_DURATION = 0.35f;
    const float ANIM_OUT_DURATION = 0.25f;

    ECS::EntityID selectorID = INVALID_ENTITY_ID;
    ECS::EntityID selectorOuterID = INVALID_ENTITY_ID;

    // UI_CAMERA_POSEを持つエンティティを探す（アニメーション用）
    ECS::EntityID camLabelID = INVALID_ENTITY_ID;

    for (auto id : m_pauseUIEntities) {
        if (!m_coordinator->HasComponent<TransformComponent>(id)) continue;
        auto& t = m_coordinator->GetComponent<TransformComponent>(id);

        // セレクター判定 (サイズ)
        if (t.scale.x == 30.0f && t.scale.y == 30.0f && t.rotation.z != tiltRad) selectorID = id;
        if (t.scale.x == 40.0f && t.scale.y == 40.0f && t.rotation.z != tiltRad) selectorOuterID = id;

        // ラベル判定 (アセットID)
        if (m_coordinator->HasComponent<UIImageComponent>(id)) {
            if (m_coordinator->GetComponent<UIImageComponent>(id).assetID == "UI_CAMERA_POSE") {
                camLabelID = id;
            }
        }
    }

    XMFLOAT2 mouseDelta = GetMouseDelta();
    static float paraX = 0.0f;
    paraX += mouseDelta.x * -0.03f;
    paraX = std::max(-10.0f, std::min(10.0f, paraX));
    paraX *= 0.9f;

    if (m_pauseDecoLineID != INVALID_ENTITY_ID) {
        float breath = 0.8f + 0.2f * sinf(m_pauseTimer * 3.0f);
        m_coordinator->GetComponent<UIImageComponent>(m_pauseDecoLineID).color.w = breath;
    }

    if (m_pauseState == PauseState::AnimateIn) {
        float t = std::min(1.0f, m_pauseTimer / ANIM_IN_DURATION);
        float ease = 1.0f - powf(1.0f - t, 4.0f);

        if (m_pauseBgOverlayID != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<UIImageComponent>(m_pauseBgOverlayID).color.w = t * 0.6f;

        float slashW = SCREEN_WIDTH * 0.5f;
        float startX = -slashW;
        float targetX = SCREEN_WIDTH * 0.05f;
        float curX = startX + (targetX - startX) * ease;

        if (m_pauseDecoSlashID != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<TransformComponent>(m_pauseDecoSlashID).position.x = curX + paraX;

        if (m_pauseDecoLineID != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<TransformComponent>(m_pauseDecoLineID).position.x = curX + slashW * 0.55f + paraX;

        // ★追加: camLabelID も一緒にスライドさせる
        std::vector<ECS::EntityID> items = {
            m_pauseItems.btnReverse, m_pauseItems.btnRetry, m_pauseItems.btnStage,
            camLabelID, // ここに追加
            m_pauseItems.sliderBar, m_pauseItems.sliderKnob
        };
        auto GetTargetX = [&](float y) { return (SCREEN_WIDTH * 0.22f) - ((y - SCREEN_HEIGHT * 0.5f) * tanf(tiltRad)); };

        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i] == INVALID_ENTITY_ID) continue;
            float delay = i * 0.04f;
            float itemT = std::max(0.0f, std::min(1.0f, (m_pauseTimer - delay) / (ANIM_IN_DURATION * 0.8f)));
            float itemEase = 1.0f + 2.0f * powf(itemT - 1.0f, 3.0f) + 1.0f * powf(itemT - 1.0f, 2.0f);

            auto& trans = m_coordinator->GetComponent<TransformComponent>(items[i]);
            float finalX = GetTargetX(trans.position.y);

            trans.position.x = -400.0f + (finalX - (-400.0f)) * itemEase + paraX * 0.5f;

            if (m_btnBgMap.count(items[i])) {
                ECS::EntityID bgID = m_btnBgMap[items[i]];
                m_coordinator->GetComponent<TransformComponent>(bgID).position.x = trans.position.x - 30.0f;
                EntityID lineID = bgID + 1;
                if (m_coordinator->HasComponent<TransformComponent>(lineID))
                    m_coordinator->GetComponent<TransformComponent>(lineID).position.x = trans.position.x - 30.0f;
            }
        }
        UpdatePauseSliderState();
        if (m_pauseTimer >= ANIM_IN_DURATION + 0.1f) m_pauseState = PauseState::Active;
    }
    else if (m_pauseState == PauseState::Active) {
        UpdatePauseSliderState();

        float selY = -9999.0f, selX = -9999.0f; bool anyHover = false;
        std::vector<ECS::EntityID> btns = { m_pauseItems.btnReverse, m_pauseItems.btnRetry, m_pauseItems.btnStage };
        auto GetBaseX = [&](float y) { return (SCREEN_WIDTH * 0.22f) - ((y - SCREEN_HEIGHT * 0.5f) * tanf(tiltRad)); };

        for (auto btnID : btns) {
            if (btnID == INVALID_ENTITY_ID) continue;
            auto& btn = m_coordinator->GetComponent<UIButtonComponent>(btnID);
            auto& trans = m_coordinator->GetComponent<TransformComponent>(btnID);
            auto& ui = m_coordinator->GetComponent<UIImageComponent>(btnID);

            ECS::EntityID bgID = m_btnBgMap[btnID];
            ECS::EntityID lineID = bgID + 1;
            auto& bgTrans = m_coordinator->GetComponent<TransformComponent>(bgID);
            auto& bgUI = m_coordinator->GetComponent<UIImageComponent>(bgID);

            float baseX = GetBaseX(trans.position.y);

            if (btn.state == ButtonState::Hover || btn.state == ButtonState::Pressed) {
                float slideDist = 15.0f;
                float targetX = baseX + slideDist * cosf(tiltRad) + paraX * 0.5f;
                trans.position.x += (targetX - trans.position.x) * 10.0f * deltaTime;

                float pulse = 0.8f + 0.2f * sinf(m_pauseTimer * 10.0f);
                ui.color = { 0.0f, pulse, pulse, 1.0f };

                bgUI.color = { 0.1f, 0.15f, 0.2f, 0.9f };
                float targetScale = 480.0f;
                bgTrans.scale.x += (targetScale - bgTrans.scale.x) * 15.0f * deltaTime;
                bgTrans.position.x = trans.position.x - 30.0f;

                if (m_coordinator->HasComponent<UIImageComponent>(lineID)) {
                    m_coordinator->GetComponent<UIImageComponent>(lineID).color = { 0, 1, 1, 0.5f + 0.5f * sinf(m_pauseTimer * 20.0f) };
                    m_coordinator->GetComponent<TransformComponent>(lineID).scale.x += (460.0f - m_coordinator->GetComponent<TransformComponent>(lineID).scale.x) * 20.0f * deltaTime;
                    m_coordinator->GetComponent<TransformComponent>(lineID).position.x = bgTrans.position.x;
                }

                anyHover = true;
                selY = trans.position.y + 10.0f;
                selX = trans.position.x - 220.0f;

                if (m_lastHoveredID != btnID) {
                    EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_CURSOR", 0.5f);
                    m_lastHoveredID = btnID;
                }
            }
            else {
                float targetX = baseX + paraX * 0.5f;
                trans.position.x += (targetX - trans.position.x) * 10.0f * deltaTime;

                ui.color = { 1.0f, 1.0f, 1.0f, 1.0f };
                bgUI.color = { 0.05f, 0.05f, 0.05f, 0.5f };
                float targetScale = 420.0f;
                bgTrans.scale.x += (targetScale - bgTrans.scale.x) * 10.0f * deltaTime;
                bgTrans.position.x = trans.position.x - 30.0f;

                if (m_coordinator->HasComponent<UIImageComponent>(lineID))
                    m_coordinator->GetComponent<UIImageComponent>(lineID).color.w = 0.0f;
            }
        }

        if (!anyHover) m_lastHoveredID = INVALID_ENTITY_ID;

        // --- セレクター (回転演出) ---
        if (selectorID != INVALID_ENTITY_ID) {
            auto& sTrans = m_coordinator->GetComponent<TransformComponent>(selectorID);
            auto& sUI = m_coordinator->GetComponent<UIImageComponent>(selectorID);

            // 常に回転
            sTrans.rotation.z += 3.0f * deltaTime;

            if (anyHover) {
                sUI.color.w += (1.0f - sUI.color.w) * 15.0f * deltaTime;
                sTrans.position.y += (selY - sTrans.position.y) * 20.0f * deltaTime;
                sTrans.position.x += (selX - sTrans.position.x) * 20.0f * deltaTime;
            }
            else {
                sUI.color.w += (0.0f - sUI.color.w) * 10.0f * deltaTime;
            }

            if (selectorOuterID != INVALID_ENTITY_ID) {
                auto& soTrans = m_coordinator->GetComponent<TransformComponent>(selectorOuterID);
                auto& soUI = m_coordinator->GetComponent<UIImageComponent>(selectorOuterID);
                soTrans.position = sTrans.position;
                soTrans.rotation.z -= 1.5f * deltaTime; // 逆回転
                soUI.color.w = sUI.color.w * 0.4f;
            }
        }
    }
    else if (m_pauseState == PauseState::AnimateOut) {
        float t = std::min(1.0f, m_pauseTimer / ANIM_OUT_DURATION);
        float ease = t * t;

        if (m_pauseBgOverlayID != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<UIImageComponent>(m_pauseBgOverlayID).color.w = 0.4f * (1.0f - t);

        float moveDist = 2500.0f * ease;

        for (auto id : m_pauseUIEntities) {
            if (id == m_pauseBgOverlayID) continue;
            if (m_coordinator->HasComponent<TransformComponent>(id)) {
                m_coordinator->GetComponent<TransformComponent>(id).position.x -= moveDist;
            }
        }

        if (m_pauseTimer >= ANIM_OUT_DURATION) {
            if (m_pendingTransition) {
                StopBGM();
                DestroyPauseUI();
                m_pauseState = PauseState::Hidden;
                m_pendingTransition();
                m_pendingTransition = nullptr;
            }
            else {
                DestroyPauseUI();
                m_pauseState = PauseState::Hidden;
            }
        }
    }
}

// ---------------------------------------------------------------------
// 3. UpdatePauseSliderState (スライダー位置の厳密な補正)
// ---------------------------------------------------------------------
void GameControlSystem::UpdatePauseSliderState() {
    if (m_pauseItems.sliderKnob == INVALID_ENTITY_ID || m_pauseItems.sliderBar == INVALID_ENTITY_ID) return;

    auto& knobTrans = m_coordinator->GetComponent<TransformComponent>(m_pauseItems.sliderKnob);
    auto& knobUI = m_coordinator->GetComponent<UIImageComponent>(m_pauseItems.sliderKnob);
    auto& barTrans = m_coordinator->GetComponent<TransformComponent>(m_pauseItems.sliderBar);

    float barW = barTrans.scale.x;
    // 傾きを考慮したローカル座標系での左端・右端 (バーの中心からのオフセット)
    float localLeft = -barW * 0.5f;
    float localRight = barW * 0.5f;

    float tiltRad = XMConvertToRadians(12.0f); // 共通の傾き
    float cosT = cosf(tiltRad);
    float sinT = sinf(tiltRad);

    // バーの中心座標
    float barCX = barTrans.position.x;
    float barCY = barTrans.position.y;

    // ワールド座標での左端と右端のX座標
    float worldLeftX = barCX + (localLeft * cosT);
    float worldRightX = barCX + (localRight * cosT);

    if (m_pauseState == PauseState::Active) {
        EntityID cursorID = m_pauseItems.cursor;
        if (cursorID != INVALID_ENTITY_ID) {
            auto& cTrans = m_coordinator->GetComponent<TransformComponent>(cursorID);
            auto& cComp = m_coordinator->GetComponent<UICursorComponent>(cursorID);
            bool isTrigger = cComp.isTriggered;
            bool isHold = IsMousePress(0) || IsButtonPress(BUTTON_A);

            if (isTrigger) {
                // 当たり判定も傾きを考慮して簡易的にX軸とY軸の距離で見る
                float cx = cTrans.position.x;
                float cy = cTrans.position.y;

                // バー上にあるか判定 (厳密には点と直線の距離だが、簡易的に近傍判定)
                float distY = std::abs(cy - (barCY - (cx - barCX) * tanf(tiltRad)));
                if (cx >= worldLeftX - 40 && cx <= worldRightX + 40 && distY < 60) {
                    m_isDraggingSlider = true;
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_SYSTEM_OPEN", 0.5f);
                }
            }
            if (!isHold) m_isDraggingSlider = false;

            if (m_isDraggingSlider) {
                float cx = cTrans.position.x;
                float newX = std::max(worldLeftX, std::min(worldRightX, cx));

                // 割合t (0.0 ~ 1.0)
                float t = (newX - worldLeftX) / (worldRightX - worldLeftX);

                float minSens = 0.001f, maxSens = 0.02f;
                float newSens = minSens + (maxSens - minSens) * t;
                if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
                    camSys->SetMouseSensitivity(newSens);

                m_sliderSoundTimer += 0.016f;
                if (m_sliderSoundTimer > 0.1f) {
                    EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_SLIDER", 0.3f);
                    m_sliderSoundTimer = 0.0f;
                }
            }
        }
    }

    // 現在値から位置を逆算
    float currentSens = 0.005f;
    if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
        currentSens = camSys->GetMouseSensitivity();

    float minSens = 0.001f, maxSens = 0.02f;
    float t = (currentSens - minSens) / (maxSens - minSens);
    t = std::max(0.0f, std::min(1.0f, t));

    // ノブの位置をバーの傾きに合わせて更新
    // ローカル座標でのXオフセット
    float localX = localLeft + (barW * t);

    // ワールド座標へ変換
    knobTrans.position.x = barCX + (localX * cosT);
    knobTrans.position.y = barCY - (localX * sinT);

    if (m_isDraggingSlider) {
        knobUI.color = { 1, 1, 0, 1 }; knobTrans.scale = { 20, 40, 1 };
    }
    else {
        knobUI.color = { 0.0f, 1.0f, 1.0f, 1.0f }; knobTrans.scale = { 16, 32, 1 };
    }
}

void GameControlSystem::DestroyPauseUI()
{
    for (auto id : m_pauseUIEntities) m_coordinator->DestroyEntity(id);
    m_pauseUIEntities.clear();
    m_btnBgMap.clear();
    m_pauseBgOverlayID = INVALID_ENTITY_ID;
    m_pauseDecoSlashID = INVALID_ENTITY_ID;
    m_pauseDecoLineID = INVALID_ENTITY_ID;
    m_pauseItems = {};
    m_isDraggingSlider = false;
}

void GameControlSystem::UpdateDecorations(float deltaTime)
{
    static float time = 0.0f;
    time += deltaTime;

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
        const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;

        if (tag == "propeller") {
            if (m_coordinator->HasComponent<TransformComponent>(entity)) {
                m_coordinator->GetComponent<TransformComponent>(entity).rotation.y += 5.0f * deltaTime;
            }
            // Propeller Light Flicker
            if (m_coordinator->HasComponent<PointLightComponent>(entity)) {
                auto& light = m_coordinator->GetComponent<PointLightComponent>(entity);
                float flicker = 1.0f + 0.05f * sinf(time * 5.0f);
                light.range = 20.0f * flicker;
            }
        }
        else if (tag == "security_camera") {
            if (m_coordinator->HasComponent<TransformComponent>(entity)) {
                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
                float baseRot = round(trans.rotation.y / DirectX::XM_PIDIV2) * DirectX::XM_PIDIV2;
                float swing = sinf(time * 0.8f + (float)entity) * 0.7f;
                float currentRotY = baseRot + swing;
                trans.rotation.y = currentRotY;

                if (m_coordinator->HasComponent<PointLightComponent>(entity)) {
                    auto& light = m_coordinator->GetComponent<PointLightComponent>(entity);
                    float dist = 2.0f;
                    float lx = sinf(currentRotY) * dist;
                    float lz = cosf(currentRotY) * dist;
                    light.offset = { lx, -1.5f, lz };

                    // Camera Light Noise
                    float noise = (float)(rand() % 100) / 100.0f;
                    float intensity = 1.0f;
                    if (noise > 0.95f) intensity = 0.2f;
                    if (noise < 0.05f) intensity = 1.5f;

                    light.range = 25.0f * intensity;
                }
            }
        }
        else if (tag == "painting") {
            // Painting Light Flicker
            if (m_coordinator->HasComponent<PointLightComponent>(entity)) {
                auto& light = m_coordinator->GetComponent<PointLightComponent>(entity);
                float flicker = 1.0f + 0.3f * sinf(time * 2.0f + (float)entity);
                light.range = 12.0f * flicker;
            }

            // Painting Shake
            if (m_coordinator->HasComponent<TransformComponent>(entity)) {
                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
                float r = (float)(rand() % 100);
                float shake = 0.0f;

                if (r < 3.0f) shake = (float)(rand() % 20 - 10) * 0.02f;
                else          shake = (float)(rand() % 10 - 5) * 0.005f;

                if (std::abs(sinf(trans.rotation.y)) < 0.7f) trans.rotation.z = shake;
                else                                         trans.rotation.z = shake;
            }
        }
    }
}

void GameControlSystem::UpdateLights()
{
    // Collect all PointLightComponents
    std::vector<PointLightData> lights;
    lights.reserve(MAX_LIGHTS);

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<PointLightComponent>(entity) &&
            m_coordinator->HasComponent<TransformComponent>(entity)) {

            auto& plc = m_coordinator->GetComponent<PointLightComponent>(entity);
            auto& tc = m_coordinator->GetComponent<TransformComponent>(entity);

            if (!plc.isActive) continue;

            PointLightData data;
            data.position.x = tc.position.x + plc.offset.x;
            data.position.y = tc.position.y + plc.offset.y;
            data.position.z = tc.position.z + plc.offset.z;
            data.position.w = plc.range;
            data.color = plc.color;

            lights.push_back(data);

            if (lights.size() >= MAX_LIGHTS) break;
        }
    }

    DirectX::XMFLOAT4 ambient = { 0.3f, 0.3f, 0.3f, 1.0f };
    ShaderList::SetPointLights(lights.data(), (int)lights.size(), ambient);
}

// ---------------------------------------------------------------------
// ★修正: UpdateTimerAndRules などのリンクエラーになっていた関数群の実装
// ---------------------------------------------------------------------

void GameControlSystem::UpdateTimerAndRules(float deltaTime, ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.isGameOver || state.isGameClear) return;
    state.elapsedTime += deltaTime;
#ifdef _DEBUG
    if (IsKeyTrigger('C')) { state.isGameClear = true; state.isGameOver = false; }
    if (IsKeyTrigger('O')) { state.isGameOver = true; state.isGameClear = false; }
#endif
}

void GameControlSystem::HandleInputAndStateSwitch(ECS::EntityID controllerID) {
    bool pressedSpace = IsKeyTrigger(VK_SPACE); bool pressedA = IsButtonTriggered(BUTTON_A);
    if (!(pressedSpace || pressedA)) return;
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.currentMode == GameMode::SCOUTING_MODE) {
        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TOPVIEWSTART", 0.4f); // 音量調整
        StartMosaicSequence(controllerID);
    }
    else if (state.currentMode == GameMode::ACTION_MODE) {
#ifndef _DEBUG
        if (m_hasUsedTopView) return;
#endif
        state.currentMode = GameMode::SCOUTING_MODE;
        m_hasUsedTopView = true;

        // ★修正: トップビュー遷移時は BGM_TOPVIEW を再生
        PlayBGM("BGM_TOPVIEW", 0.7f);

        ApplyModeVisuals(controllerID);
        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TOPVIEWSTART", 0.4f); // 音量調整
    }
    ApplyModeVisuals(controllerID);
}

void GameControlSystem::CheckSceneTransition(ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.isGameOver || state.isGameClear) {

        // ★修正: ゲーム終了時にBGM停止
        StopBGM();

        if (state.isGameOver) {
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_ARREST", 0.5f); // 音量調整
        }

        ResultData data; data.isCleared = state.isGameClear; data.clearTime = state.elapsedTime; data.clearedInTime = (state.elapsedTime <= state.timeLimitStar); data.timeLimitStar = state.timeLimitStar; data.wasSpotted = state.wasSpotted; data.stageID = GameScene::GetStageNo();
        if (m_coordinator->HasComponent<ItemTrackerComponent>(controllerID)) {
            auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
            data.collectedCount = tracker.collectedItems; data.totalItems = tracker.totalItems; data.collectedAllOrdered = tracker.useOrderedCollection;
            data.collectedItemIcons.clear(); data.orderedItemIcons.clear(); data.orderedItemCollected.clear();
            for (const auto& targetID : tracker.targetItemIDs) {
                bool isCollected = true;
                for (auto const& entity : m_coordinator->GetActiveEntities()) {
                    if (!m_coordinator->HasComponent<CollectableComponent>(entity)) continue;
                    auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
                    if (col.itemID != targetID) continue;
                    if (!col.isCollected) isCollected = false; break;
                }
                std::string iconName = GetItemIconPath(targetID);
                if (isCollected) data.collectedItemIcons.push_back(iconName);
                data.orderedItemIcons.push_back(iconName); data.orderedItemCollected.push_back(isCollected);
            }
        }
        ResultScene::SetResultData(data); SceneManager::ChangeScene<ResultScene>();
    }
}

void GameControlSystem::UpdateTopViewUI(ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool showIcons = (state.currentMode == GameMode::SCOUTING_MODE);

    // ★テレポーターのペアに色を割り当て（初回のみ）
    if (showIcons && m_teleportColorMap.empty()) {
        // 色のパレット（ペアごとに異なる色）
        std::vector<DirectX::XMFLOAT4> colorPalette = {
            {0.0f, 1.0f, 1.0f, 1.0f},    // シアン
            {1.0f, 0.0f, 1.0f, 1.0f},    // マゼンタ
            {1.0f, 1.0f, 0.0f, 1.0f},    // 黄色
            {0.0f, 1.0f, 0.0f, 1.0f},    // 緑
            {1.0f, 0.5f, 0.0f, 1.0f},    // オレンジ
            {0.5f, 0.0f, 1.0f, 1.0f},    // 紫
        };

        int colorIndex = 0;
        std::unordered_set<ECS::EntityID> processed;

        // 全テレポーターを収集
        std::vector<ECS::EntityID> teleporters;
        for (auto const& entity : m_coordinator->GetActiveEntities()) {
            if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "teleporter" && m_coordinator->HasComponent<TransformComponent>(entity)) {
                teleporters.push_back(entity);
            }
        }

        // ★修正: TeleportComponentのtargetEntityを基準にペアを判定
        for (size_t i = 0; i < teleporters.size(); ++i) {
            ECS::EntityID teleA = teleporters[i];
            if (processed.count(teleA)) continue;

            // このテレポーターに色を割り当て
            DirectX::XMFLOAT4 pairColor = colorPalette[colorIndex % colorPalette.size()];
            m_teleportColorMap[teleA] = pairColor;
            processed.insert(teleA);

            // ★TeleportComponentからtargetEntity（実際の接続先）を取得
            ECS::EntityID linkedTele = ECS::INVALID_ENTITY_ID;
            if (m_coordinator->HasComponent<TeleportComponent>(teleA)) {
                auto& teleComp = m_coordinator->GetComponent<TeleportComponent>(teleA);
                linkedTele = teleComp.targetEntity;
            }

            // ペアが見つかったら同じ色を割り当て
            if (linkedTele != ECS::INVALID_ENTITY_ID && !processed.count(linkedTele)) {
                m_teleportColorMap[linkedTele] = pairColor;
                processed.insert(linkedTele);
            }

            colorIndex++;
        }
    }

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID) {
        if (showIcons) UpdateIcon(playerID, "ICO_PLAYER", { 1, 1, 1, 1 });
        else if (m_iconMap.count(playerID)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[playerID]).isVisible = false;
    }
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<CollectableComponent>(entity)) {
            auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
            if (!col.isCollected && showIcons) { std::string asset = GetItemIconPath(col.itemID); UpdateIcon(entity, asset, { 1, 1, 1, 1 }); }
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
        }
        bool isGuard = false, isTeleporter = false, isStopTrap = false;
        if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "taser") isGuard = true; if (tag == "teleporter") isTeleporter = true; if (tag == "stop_trap") isStopTrap = true;
        }
        if (isGuard) { if (showIcons) UpdateIcon(entity, "ICO_TASER", { 1, 1, 1, 1 }); else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false; }
        if (isTeleporter) {
            // ★テレポーターの色を個別に設定
            DirectX::XMFLOAT4 teleportColor = { 0, 1, 1, 1 }; // デフォルト色
            if (m_teleportColorMap.count(entity)) {
                teleportColor = m_teleportColorMap[entity];
            }
            if (showIcons) UpdateIcon(entity, "UI_TITLE_LOGO", teleportColor);
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
        }
        if (isStopTrap) { if (showIcons) UpdateIcon(entity, "UI_ASHIATO_BLUE", { 0.8f, 0.0f, 0.8f, 1.0f }); else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false; }
    }
    if (!showIcons) return;
    EntityID cameraID = FindFirstEntityWithComponent<CameraComponent>(m_coordinator); if (cameraID == INVALID_ENTITY_ID) return;
    auto& camera = m_coordinator->GetComponent<CameraComponent>(cameraID);
    XMMATRIX view = XMMatrixTranspose(XMLoadFloat4x4(&camera.viewMatrix)); XMMATRIX proj = XMMatrixTranspose(XMLoadFloat4x4(&camera.projectionMatrix));
    XMMATRIX viewProj = view * proj;
    for (auto& pair : m_iconMap) {
        EntityID target = pair.first; EntityID icon = pair.second;
        auto& iconUI = m_coordinator->GetComponent<UIImageComponent>(icon); if (!iconUI.isVisible) continue;
        if (!m_coordinator->HasComponent<TransformComponent>(target)) { iconUI.isVisible = false; continue; }
        auto& targetTrans = m_coordinator->GetComponent<TransformComponent>(target);
        XMFLOAT3 screenPos = GetScreenPosition(targetTrans.position, viewProj);
        auto& iconTrans = m_coordinator->GetComponent<TransformComponent>(icon);
        iconTrans.position = { screenPos.x, screenPos.y, 0.0f };
        if (screenPos.z < 0.0f || screenPos.z > 1.0f) iconUI.isVisible = false;
    }
}
void GameControlSystem::UpdateCaughtSequence(float deltaTime, ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); state.sequenceTimer += deltaTime;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID || m_catchingGuardID == INVALID_ENTITY_ID) { state.isGameOver = true; CheckSceneTransition(controllerID); return; }
    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID); auto& gTrans = m_coordinator->GetComponent<TransformComponent>(m_catchingGuardID);
    if (!m_caughtAnimPlayed) {
        XMVECTOR pPos = XMLoadFloat3(&pTrans.position); XMVECTOR gPos = XMLoadFloat3(&gTrans.position);
        XMVECTOR dirVec = XMVectorSubtract(pPos, gPos); dirVec = XMVectorSetY(dirVec, 0.0f);
        float distance = XMVectorGetX(XMVector3Length(dirVec)); dirVec = XMVector3Normalize(dirVec);
        if (distance > 1.5f) {
            const float moveSpeed = 3.5f * deltaTime;
            const XMVECTOR newPos = gPos + (dirVec * moveSpeed);
            const float originalY = gTrans.position.y;
            ::DirectX::XMStoreFloat3(&gTrans.position, newPos);
            gTrans.position.y = originalY;
            const float dx = XMVectorGetX(dirVec);
            const float dz = XMVectorGetZ(dirVec);
            gTrans.rotation.y = atan2(dx, dz); pTrans.rotation.y = atan2(-dx, -dz);
        }
        else {
            m_caughtAnimPlayed = true; state.sequenceTimer = 0.0f;
            if (m_coordinator->HasComponent<AnimationComponent>(m_catchingGuardID)) m_coordinator->GetComponent<AnimationComponent>(m_catchingGuardID).Play("A_GUARD_ATTACK", false);
            if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_CAUGHT", false);
            // 音量調整 (1.0 -> 0.5)
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_HIT", 0.5f);
        }
    }
    else { if (state.sequenceTimer > 2.0f) { state.isGameOver = true; CheckSceneTransition(controllerID); } }
}

void GameControlSystem::UpdateIcon(ECS::EntityID target, std::string iconAsset, DirectX::XMFLOAT4 color) {
    if (m_iconMap.find(target) == m_iconMap.end()) {
        EntityID icon = m_coordinator->CreateEntity(TransformComponent({ 0, 0, 0 }, { 0, 0, 0 }, { 32, 32, 1 }), UIImageComponent(iconAsset, 1.0f, true, color));
        m_iconMap[target] = icon;
    }
    if (m_coordinator->HasComponent<UIImageComponent>(m_iconMap[target]))
        m_coordinator->GetComponent<UIImageComponent>(m_iconMap[target]).isVisible = true;
}

void GameControlSystem::UpdateScanLine(float deltaTime, ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);
    EntityID cameraID = FindFirstEntityWithComponent<CameraComponent>(m_coordinator); XMMATRIX viewProj = XMMatrixIdentity();
    if (cameraID != INVALID_ENTITY_ID) { auto& camera = m_coordinator->GetComponent<CameraComponent>(cameraID); viewProj = XMMatrixTranspose(XMLoadFloat4x4(&camera.viewMatrix)) * XMMatrixTranspose(XMLoadFloat4x4(&camera.projectionMatrix)); }
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<ScanLineComponent>(entity)) continue;
        auto& scan = m_coordinator->GetComponent<ScanLineComponent>(entity); auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity); auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
        ui.isVisible = isScouting; if (!isScouting) continue;
        float prevY = trans.position.y; float move = scan.speed * deltaTime;
        if (scan.movingDown) { trans.position.y += move; if (trans.position.y >= scan.endY) { trans.position.y = scan.endY; scan.movingDown = false; } }
        else { trans.position.y -= move; if (trans.position.y <= scan.startY) { trans.position.y = scan.startY; scan.movingDown = true; } }
        float currY = trans.position.y; trans.position.x = SCREEN_WIDTH * 0.5f;
        for (auto& pair : m_iconMap) {
            EntityID target = pair.first; EntityID icon = pair.second;
            if (!m_coordinator->HasComponent<UIImageComponent>(icon)) continue;
            if (!m_coordinator->GetComponent<UIImageComponent>(icon).isVisible) continue;
            if (!m_coordinator->HasComponent<TransformComponent>(target)) continue;
            auto& targetTrans = m_coordinator->GetComponent<TransformComponent>(target);
            XMFLOAT3 sPos = GetScreenPosition(targetTrans.position, viewProj);
            float minY = std::min(prevY, currY); float maxY = std::max(prevY, currY);
            if (sPos.z >= 0.0f && sPos.z <= 1.0f) {
                if (sPos.y >= minY && sPos.y <= maxY) {
                    XMFLOAT4 color = { 1, 1, 1, 1 };
                    if (m_coordinator->HasComponent<CollectableComponent>(target)) color = { 1, 1, 0, 1 };
                    else if (m_coordinator->GetComponent<TagComponent>(target).tag == "taser") color = { 1, 0, 0, 1 };
                    SpawnSmallSonar(sPos, color);
                }
            }
        }
    }
}

void GameControlSystem::UpdateSonarEffect(float deltaTime, ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);
    if (isScouting) {
        m_sonarSpawnTimer += deltaTime;
        if (m_sonarSpawnTimer >= 1.0f) {
            m_sonarSpawnTimer = 0.0f; EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
            if (playerID != INVALID_ENTITY_ID && m_iconMap.count(playerID)) {
                auto& iconTrans = m_coordinator->GetComponent<TransformComponent>(m_iconMap[playerID]);
                m_coordinator->CreateEntity(TransformComponent(iconTrans.position, { 0, 0, 0 }, { 0, 0, 1 }), UIImageComponent("UI_SONAR", 4.0f, true, { 0.0f, 1.0f, 0.5f, 1.0f }), SonarComponent(1.5f, 0.0f, 500.0f));
            }
        }
    }
    std::vector<EntityID> toDestroy;
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<SonarComponent>(entity)) continue;
        if (!isScouting) { toDestroy.push_back(entity); continue; }
        auto& sonar = m_coordinator->GetComponent<SonarComponent>(entity); auto& trans = m_coordinator->GetComponent<TransformComponent>(entity); auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);
        sonar.timer += deltaTime; float progress = sonar.timer / sonar.maxTime;
        if (progress >= 1.0f) { toDestroy.push_back(entity); continue; }
        float scale = sonar.startScale + (sonar.maxScale - sonar.startScale) * progress;
        trans.scale = { scale, scale, 1.0f }; ui.color.w = 0.5f - (progress * progress);
    }
    for (auto id : toDestroy) m_coordinator->DestroyEntity(id);
}

DirectX::XMFLOAT3 GameControlSystem::GetScreenPosition(const DirectX::XMFLOAT3& worldPos, const DirectX::XMMATRIX& viewProj) {
    XMVECTOR wPos = XMLoadFloat3(&worldPos); XMVECTOR clipPos = XMVector3TransformCoord(wPos, viewProj);
    XMFLOAT3 ndc{};
    ::DirectX::XMStoreFloat3(&ndc, clipPos);
    float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH; float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;
    return XMFLOAT3(screenX, screenY, ndc.z);
}

void GameControlSystem::SpawnSmallSonar(const XMFLOAT3& screenPos, XMFLOAT4 color) {
    m_coordinator->CreateEntity(TransformComponent({ screenPos.x, screenPos.y, 0.0f }, { 0,0,0 }, { 0,0,1 }), UIImageComponent("UI_SONAR", 4.0f, true, color), SonarComponent(1.0f, 0.0f, 200.0f));
}

void GameControlSystem::StartEntranceSequence(EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceState = GameSequenceState::Entering;
    state.sequenceTimer = 0.0f;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    EntityID doorID = FindEntranceDoor();

    if (playerID != INVALID_ENTITY_ID && doorID != INVALID_ENTITY_ID) {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& dTrans = m_coordinator->GetComponent<TransformComponent>(doorID);
        float rad = dTrans.rotation.y;
        float startDist = 5.0f;
        pTrans.position.x = dTrans.position.x - sin(rad) * startDist;
        pTrans.position.z = dTrans.position.z - cos(rad) * startDist;
        pTrans.rotation.y = dTrans.rotation.y;

        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) {
            XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);
            XMVECTOR doorDir = XMVectorSet(sin(rad), 0.0f, cos(rad), 0.0f);
            XMVECTOR camPosVec = doorPos + (doorDir * 7.5f) + XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f);
            XMVECTOR lookAtVec = doorPos;
            XMFLOAT3 camPos{};
            XMFLOAT3 lookAt{};
            ::DirectX::XMStoreFloat3(&camPos, camPosVec);
            ::DirectX::XMStoreFloat3(&lookAt, lookAtVec);
            camSys->SetFixedCamera(camPos, lookAt);
        }
        if (m_coordinator->HasComponent<AnimationComponent>(doorID)) m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_OPEN", false);
        if (m_coordinator->HasComponent<CollisionComponent>(doorID)) m_coordinator->GetComponent<CollisionComponent>(doorID).type = COLLIDER_TRIGGER;
        // ★修正: SE_DOOR_OPEN (停止可能SE)
        PlayStopableSE("SE_DOOR", 0.5f);
    }
}

void GameControlSystem::UpdateEntranceSequence(float deltaTime, EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); state.sequenceTimer += deltaTime;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator); if (playerID == INVALID_ENTITY_ID) return;
    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
    if (state.sequenceTimer < 2.5f) {
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN", true);
        float speed = 4.0f * deltaTime; float rad = pTrans.rotation.y;
        pTrans.position.x += sin(rad) * speed; pTrans.position.z += cos(rad) * speed;
    }
    else if (state.sequenceTimer < 4.5f) {
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_IDLE");
        if (state.sequenceTimer - deltaTime < 2.5f) {
            EntityID doorID = FindEntranceDoor();
            if (doorID != INVALID_ENTITY_ID) {
                if (m_coordinator->HasComponent<AnimationComponent>(doorID)) m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_CLOSE", false);
                if (m_coordinator->HasComponent<CollisionComponent>(doorID)) m_coordinator->GetComponent<CollisionComponent>(doorID).type = COLLIDER_STATIC;
                // ★修正: SE_DOOR_CLOSE (停止可能SE)
                PlayStopableSE("SE_DOOR", 0.5f);
            }
        }
    }
    else {
        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) { camSys->ReleaseFixedCamera(); camSys->ResetCameraAngle(pTrans.rotation.y, 0.6f); }
        state.sequenceState = GameSequenceState::Playing;
    }
}

void GameControlSystem::CheckDoorUnlock(EntityID controllerID) {
    if (!m_coordinator->HasComponent<ItemTrackerComponent>(controllerID)) return;
    auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
    if (tracker.collectedItems >= tracker.totalItems) {
        EntityID exitDoor = FindExitDoor();
        if (exitDoor != INVALID_ENTITY_ID) {
            auto& door = m_coordinator->GetComponent<DoorComponent>(exitDoor);
            if (door.isLocked) {
                door.isLocked = false; door.state = DoorState::Open;
                if (m_coordinator->HasComponent<AnimationComponent>(exitDoor)) m_coordinator->GetComponent<AnimationComponent>(exitDoor).Play("A_DOOR_OPEN", false);
                if (m_coordinator->HasComponent<CollisionComponent>(exitDoor)) m_coordinator->GetComponent<CollisionComponent>(exitDoor).type = COLLIDER_TRIGGER;
                for (auto const& entity : m_coordinator->GetActiveEntities()) {
                    if (!m_coordinator->HasComponent<SoundComponent>(entity)) continue;
                    auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
                    if (sound.assetID == "BGM_TEST" || sound.assetID == "BGM_TEST2") sound.RequestStop();
                }
                // 音量調整 (1.0 -> 0.5)
                PlayStopableSE("SE_DOOR", 0.5f);
                PlayBGM("BGM_GAME_2");
            }
        }
    }
}

void GameControlSystem::UpdateExitSequence(float deltaTime, EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); state.sequenceTimer += deltaTime;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator); EntityID exitDoorID = FindExitDoor();
    if (playerID != INVALID_ENTITY_ID && exitDoorID != INVALID_ENTITY_ID) {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID); auto& dTrans = m_coordinator->GetComponent<TransformComponent>(exitDoorID);
        if (state.sequenceTimer < 4.0f) {
            if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN");
            float targetRot = dTrans.rotation.y + XM_PI; float currentRot = pTrans.rotation.y;
            float diff = targetRot - currentRot; while (diff > XM_PI) diff -= XM_2PI; while (diff < -XM_PI) diff += XM_2PI;
            pTrans.rotation.y += diff * 5.0f * deltaTime; float speed = 2.0f * deltaTime;
            pTrans.position.x += sin(pTrans.rotation.y) * speed; pTrans.position.z += cos(pTrans.rotation.y) * speed;
        }
        if (state.sequenceTimer > 2.5f) {
            bool isOpen = false;
            if (m_coordinator->HasComponent<CollisionComponent>(exitDoorID)) if (m_coordinator->GetComponent<CollisionComponent>(exitDoorID).type == COLLIDER_TRIGGER) isOpen = true;
            if (isOpen) {
                if (m_coordinator->HasComponent<AnimationComponent>(exitDoorID)) m_coordinator->GetComponent<AnimationComponent>(exitDoorID).Play("A_DOOR_CLOSE", false);
                if (m_coordinator->HasComponent<CollisionComponent>(exitDoorID)) m_coordinator->GetComponent<CollisionComponent>(exitDoorID).type = COLLIDER_STATIC;
                // SE_DOOR_CLOSE
                PlayStopableSE("SE_DOOR", 0.5f);
            }
        }
    }
    if (state.sequenceTimer > 5.0f) { state.isGameClear = true; CheckSceneTransition(controllerID); }
}

EntityID GameControlSystem::FindEntranceDoor() {
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<DoorComponent>(entity)) if (m_coordinator->GetComponent<DoorComponent>(entity).isEntrance) return entity;
    }
    return INVALID_ENTITY_ID;
}

EntityID GameControlSystem::FindExitDoor() { return FindEntranceDoor(); }

bool GameControlSystem::IsAABBOverlap(ECS::EntityID a, ECS::EntityID b) {
    if (a == INVALID_ENTITY_ID || b == INVALID_ENTITY_ID) return false;
    if (!m_coordinator->HasComponent<TransformComponent>(a) || !m_coordinator->HasComponent<TransformComponent>(b)) return false;
    if (!m_coordinator->HasComponent<CollisionComponent>(a) || !m_coordinator->HasComponent<CollisionComponent>(b)) return false;
    auto makeMinMax = [](const TransformComponent& t, const CollisionComponent& c, XMFLOAT3& min, XMFLOAT3& max) {
        float cx = t.position.x + c.offset.x; float cy = t.position.y + c.offset.y; float cz = t.position.z + c.offset.z;
        min = { cx - c.size.x, cy - c.size.y, cz - c.size.z }; max = { cx + c.size.x, cy + c.size.y, cz + c.size.z };
        };
    const auto& ta = m_coordinator->GetComponent<TransformComponent>(a); const auto& ca = m_coordinator->GetComponent<CollisionComponent>(a);
    const auto& tb = m_coordinator->GetComponent<TransformComponent>(b); const auto& cb = m_coordinator->GetComponent<CollisionComponent>(b);
    XMFLOAT3 amin, amax, bmin, bmax; makeMinMax(ta, ca, amin, amax); makeMinMax(tb, cb, bmin, bmax);
    return (amin.x <= bmax.x && amax.x >= bmin.x && amin.y <= bmax.y && amax.y >= bmin.y && amin.z <= bmax.z && amax.z >= bmin.z);
}

void GameControlSystem::CheckMapGimmickTrigger(ECS::EntityID controllerID) {
    // 自動遷移は無効化 (ユーザー指示)
}

void GameControlSystem::StartMosaicSequence(ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); state.sequenceState = GameSequenceState::Starting; state.sequenceTimer = 0.0f;
    m_blackBackID = m_coordinator->CreateEntity(TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH * 1.1f, SCREEN_HEIGHT * 1.1f, 1.0f }), UIImageComponent("FADE_WHITE", 100000.0f, true, { 0, 0, 0, 0 }));
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID && m_coordinator->HasComponent<RenderComponent>(playerID)) m_coordinator->GetComponent<RenderComponent>(playerID).type = MESH_NONE;
    for (auto& pair : m_iconMap) { if (m_coordinator->HasComponent<UIImageComponent>(pair.second)) m_coordinator->GetComponent<UIImageComponent>(pair.second).isVisible = false; }
    m_mosaicTiles.clear(); float tileW = (float)SCREEN_WIDTH / TILE_COLS; float tileH = (float)SCREEN_HEIGHT / TILE_ROWS; float uvUnitX = 1.0f / TILE_COLS; float uvUnitY = 1.0f / TILE_ROWS;
    for (int y = 0; y < TILE_ROWS; ++y) {
        for (int x = 0; x < TILE_COLS; ++x) {
            float tX = (x * tileW) + (tileW * 0.5f); float tY = (y * tileH) + (tileH * 0.5f); float startY = tY - SCREEN_HEIGHT - 200.0f - (x * 50.0f) - (y * 50.0f);
            EntityID tile = m_coordinator->CreateEntity(TransformComponent({ tX, startY, 0.0f }, { 0,0,0 }, { tileW + 2.0f, tileH + 2.0f, 1.0f }), UIImageComponent("UI_GAME_START", 100001.0f, true, { 1, 1, 1, 1 }));
            auto& ui = m_coordinator->GetComponent<UIImageComponent>(tile); ui.uvPos = { x * uvUnitX, y * uvUnitY }; ui.uvScale = { uvUnitX, uvUnitY };
            m_coordinator->GetComponent<TransformComponent>(tile).rotation.x = XM_PIDIV2; m_mosaicTiles.push_back(tile);
        }
    }
}

void GameControlSystem::UpdateMosaicSequence(float deltaTime, ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); state.sequenceTimer += deltaTime;
    const float FALL_DURATION = 0.6f, STAY_DURATION = 1.2f, END_DURATION = 0.5f; const float TILE_DELAY_X = 0.08f, TILE_DELAY_Y = 0.04f;
    float finishTime = (TILE_COLS * TILE_DELAY_X) + (TILE_ROWS * TILE_DELAY_Y) + FALL_DURATION + STAY_DURATION + END_DURATION;
    if (m_blackBackID != INVALID_ENTITY_ID) {
        auto& bgUI = m_coordinator->GetComponent<UIImageComponent>(m_blackBackID); float bgAlpha = state.sequenceTimer / 0.8f; if (bgAlpha > 1.0f) bgAlpha = 1.0f;
        if (state.sequenceTimer > finishTime - END_DURATION) { float outT = (state.sequenceTimer - (finishTime - END_DURATION)) / END_DURATION; bgAlpha = 1.0f - outT; }
        bgUI.color.w = bgAlpha;
    }
    if (state.currentMode == GameMode::SCOUTING_MODE && state.sequenceTimer > 2.0f) {
        state.currentMode = GameMode::ACTION_MODE;
        if (m_blackBackID != INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_blackBackID).color.w = 1.0f;

        // 音関連
        for (auto const& e : m_coordinator->GetActiveEntities()) {
            if (!m_coordinator->HasComponent<SoundComponent>(e)) continue;
            auto& snd = m_coordinator->GetComponent<SoundComponent>(e);
            if (snd.assetID == "BGM_TOPVIEW" || snd.assetID == "BGM_TEST") snd.RequestStop();
            if (snd.assetID == "BGM_ACTION" || snd.assetID == "BGM_TEST2") snd.RequestStop();
        }
        ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_ACTION", 0.5f);

        // UI非表示処理
        for (auto& pair : m_iconMap) { if (m_coordinator->HasComponent<UIImageComponent>(pair.second)) m_coordinator->GetComponent<UIImageComponent>(pair.second).isVisible = false; }
        for (auto const& e : m_coordinator->GetActiveEntities()) {
            if (m_coordinator->HasComponent<ScanLineComponent>(e) || m_coordinator->HasComponent<SonarComponent>(e)) {
                if (m_coordinator->HasComponent<UIImageComponent>(e)) m_coordinator->GetComponent<UIImageComponent>(e).isVisible = false;
            }
        }
        ApplyModeVisuals(controllerID);

        // --- ★ここからカメラ位置修正 ---
        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        EntityID doorID = FindEntranceDoor();

        if (playerID != INVALID_ENTITY_ID && doorID != INVALID_ENTITY_ID) {
            auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
            auto& dTrans = m_coordinator->GetComponent<TransformComponent>(doorID);

            // プレイヤー位置をドア前に再配置
            float rad = dTrans.rotation.y;
            float startDist = 5.0f;
            pTrans.position.x = dTrans.position.x - sin(rad) * startDist;
            pTrans.position.z = dTrans.position.z - cos(rad) * startDist;
            pTrans.rotation.y = dTrans.rotation.y;

            if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) {
                XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);
                XMVECTOR doorDir = XMVectorSet(sin(rad), 0.0f, cos(rad), 0.0f);

                // カメラ位置計算 (StartEntranceSequenceと同じ計算式)
                XMVECTOR camPosVec = doorPos + (doorDir * 7.5f) + XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f);
                XMVECTOR lookAtVec = doorPos;

                XMFLOAT3 camPos{};
                XMFLOAT3 lookAt{};
                ::DirectX::XMStoreFloat3(&camPos, camPosVec);
                ::DirectX::XMStoreFloat3(&lookAt, lookAtVec);

                // 1. システムに固定カメラ設定を通知
                camSys->SetFixedCamera(camPos, lookAt);

                // 2. ★重要: カメラエンティティの座標を直接書き換えてワープを防止
                EntityID camEntity = FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
                if (camEntity != INVALID_ENTITY_ID) {
                    m_coordinator->GetComponent<TransformComponent>(camEntity).position = camPos;
                }
            }
        }
        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TOPVIEWSTART", 0.4f);
    }
    float tileH = (float)SCREEN_HEIGHT / TILE_ROWS;
    for (int i = 0; i < m_mosaicTiles.size(); ++i) {
        EntityID tile = m_mosaicTiles[i]; int col = i % TILE_COLS; int row = i / TILE_COLS;
        float startTime = (col * TILE_DELAY_X) + (row * TILE_DELAY_Y); float t = state.sequenceTimer - startTime;
        auto& trans = m_coordinator->GetComponent<TransformComponent>(tile); auto& ui = m_coordinator->GetComponent<UIImageComponent>(tile); float targetY = (row * tileH) + (tileH * 0.5f);
        if (t < FALL_DURATION) {
            if (t < 0.0f) { trans.position.y = -2000.0f; continue; }
            float p = t / FALL_DURATION; float ease = 1.0f - pow(1.0f - p, 4.0f);
            float startY = targetY - 400.0f - (col * 50.0f); trans.position.y = startY + (targetY - startY) * ease; trans.rotation.x = XM_PIDIV2 * (1.0f - ease); ui.color.w = 1.0f;
        }
        else if (t < FALL_DURATION + STAY_DURATION) { trans.position.y = targetY; trans.rotation.x = 0.0f; }
        else if (t < FALL_DURATION + STAY_DURATION + END_DURATION) { float outT = (t - (FALL_DURATION + STAY_DURATION)) / END_DURATION; trans.position.y = targetY + (outT * 300.0f); trans.rotation.x = -XM_PIDIV2 * outT; ui.color.w = 1.0f - outT; }
        else { ui.color.w = 0.0f; }
    }
    if (state.sequenceTimer >= finishTime) {
        if (m_blackBackID != INVALID_ENTITY_ID) m_coordinator->DestroyEntity(m_blackBackID); for (auto id : m_mosaicTiles) m_coordinator->DestroyEntity(id); m_mosaicTiles.clear(); m_blackBackID = INVALID_ENTITY_ID;
        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        if (playerID != INVALID_ENTITY_ID && m_coordinator->HasComponent<RenderComponent>(playerID)) m_coordinator->GetComponent<RenderComponent>(playerID).type = MESH_MODEL;

        if (state.sequenceState == GameSequenceState::Starting) {
            StartEntranceSequence(controllerID);
            PlayBGM("BGM_GAME_1");
        }
        else {
            state.sequenceState = GameSequenceState::Playing;
            if (state.currentMode == GameMode::ACTION_MODE) {
                PlayBGM("BGM_GAME_1");
            }
        }
    }
}

void GameControlSystem::ApplyModeVisuals(ECS::EntityID controllerID) {
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID); bool isScoutingVisual = (state.currentMode == GameMode::SCOUTING_MODE);
    if (state.topviewBgID != INVALID_ENTITY_ID && state.tpsBgID != INVALID_ENTITY_ID) {
        auto& nUI = m_coordinator->GetComponent<UIImageComponent>(state.topviewBgID); auto& tUI = m_coordinator->GetComponent<UIImageComponent>(state.tpsBgID); nUI.isVisible = isScoutingVisual; tUI.isVisible = !isScoutingVisual;
    }
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<RenderComponent>(entity)) continue; auto& render = m_coordinator->GetComponent<RenderComponent>(entity); MeshType actionType = render.type; MeshType scoutType = MESH_NONE;
        if (m_coordinator->HasComponent<PlayerControlComponent>(entity) || m_coordinator->HasComponent<CollectableComponent>(entity)) { actionType = MESH_MODEL; scoutType = MESH_NONE; }
        else if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "guard") { actionType = MESH_MODEL; scoutType = MESH_NONE; }
            else if (tag == "taser" || tag == "map_gimmick") { actionType = MESH_NONE; scoutType = MESH_NONE; }
            else if (tag == "ground" || tag == "wall") { actionType = MESH_MODEL; scoutType = MESH_BOX; }
            else if (tag == "door") { actionType = MESH_MODEL; scoutType = MESH_MODEL; }
            else if (tag == "propeller" || tag == "security_camera" || tag == "painting") {
                actionType = MESH_MODEL;
                scoutType = MESH_NONE;
            }
        }
        render.type = isScoutingVisual ? scoutType : actionType;
    }
}

// ==================================================================================
// テレポートエフェクトの更新
// ==================================================================================
void GameControlSystem::UpdateTeleportEffects(ECS::EntityID controllerID)
{
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    bool shouldShowEffects = (state.currentMode == GameMode::ACTION_MODE);
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;
    auto& playerTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
        const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
        if (tag != "teleporter") continue;
        if (!m_coordinator->HasComponent<TransformComponent>(entity)) continue;
        auto& teleTrans = m_coordinator->GetComponent<TransformComponent>(entity);

        float distSq = XMVectorGetX(XMVector3LengthSq(
            XMLoadFloat3(&playerTrans.position) - XMLoadFloat3(&teleTrans.position)
        ));

        // プレイヤーがテレポートを踏んだ場合
        if (distSq < 9.0f) {
            if (m_teleportEffectMap.count(entity)) {
                EntityID effectID = m_teleportEffectMap[entity];
                if (m_coordinator->HasComponent<EffectComponent>(effectID)) {
                    m_coordinator->DestroyEntity(effectID);
                }
                m_teleportEffectMap.erase(entity);
            }
            m_usedTeleporters.insert(entity);
        }
        // まだ使用されていないテレポート
        else if (shouldShowEffects && !m_usedTeleporters.count(entity)) {
            if (!m_teleportEffectMap.count(entity)) {
                // ★トップビューで記録した色を取得
                XMFLOAT4 effectColor = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f); // デフォルトはシアン
                if (m_teleportColorMap.count(entity)) {
                    effectColor = m_teleportColorMap[entity];
                }

                // エフェクトを生成（loop=trueで継続再生）
                EntityID effectID = m_coordinator->CreateEntity(
                    TransformComponent(
                        teleTrans.position,
                        XMFLOAT3(0.0f, 0.0f, 0.0f),
                        XMFLOAT3(1.0f, 1.0f, 1.0f)
                    ),
                    EffectComponent(
                        "EFK_TREASURE_GLOW",
                        true,      // isActive: 生成時に即再生
                        true,      // loop: ループ再生
                        XMFLOAT3(0.0f, 0.0f, 0.0f),  // offset
                        0.3f       // scale
                    )
                );

                // マップに登録
                m_teleportEffectMap[entity] = effectID;

                // ★色を設定
                if (m_coordinator->HasComponent<RenderComponent>(effectID)) {
                    auto& render = m_coordinator->GetComponent<RenderComponent>(effectID);
                    render.color = effectColor;
                }
            }
            // エフェクトが既に存在する場合は何もしない（ループ再生が継続中）
        }
        // トップビューモードに切り替わった場合
        else if (!shouldShowEffects && m_teleportEffectMap.count(entity)) {
            EntityID effectID = m_teleportEffectMap[entity];
            if (m_coordinator->HasComponent<EffectComponent>(effectID)) {
                m_coordinator->DestroyEntity(effectID);
            }
            m_teleportEffectMap.erase(entity);
        }
    }
}
