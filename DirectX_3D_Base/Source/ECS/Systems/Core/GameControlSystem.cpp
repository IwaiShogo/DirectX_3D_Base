/*****************************************************************//**
 * @file    GameControlSystem.cpp
 * @brief   ゲーム本編の制御実装 (UI残留バグ修正・演出強化版)
 *********************************************************************/

#include "ECS/Systems/Core/GameControlSystem.h"
#include "Scene/SceneManager.h"
#include "ECS/EntityFactory.h"
#include "ECS/ECSInitializer.h"
#include "Scene/ResultScene.h"
#include <cmath>
#include <algorithm>

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
    return "ICO_TREASURE";
}

// ==================================================================================
//  Update メインループ
// ==================================================================================
void GameControlSystem::Update(float deltaTime)
{
    // 1. コントローラー取得
    EntityID controllerID = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == INVALID_ENTITY_ID) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // 2. ポーズ入力チェック
    if (IsKeyTrigger(VK_ESCAPE) || IsButtonTriggered(BUTTON_START))
    {
        TogglePauseRequest();
    }

    // 3. ポーズ処理
    if (m_pauseState != PauseState::Hidden)
    {
        UpdatePauseSequence(deltaTime, controllerID);
        state.isPaused = true;
        return; // ポーズ中はゲーム更新停止
    }
    else
    {
        state.isPaused = false;
    }

    // 4. シーケンス別更新 (演出中は専用処理のみ)
    switch (state.sequenceState)
    {
    case GameSequenceState::Starting:
        UpdateMosaicSequence(deltaTime, controllerID);
        return;
    case GameSequenceState::Entering:
        UpdateEntranceSequence(deltaTime, controllerID);
        return;
    case GameSequenceState::Exiting:
        UpdateExitSequence(deltaTime, controllerID);
        return;
    case GameSequenceState::Caught:
        UpdateCaughtSequence(deltaTime, controllerID);
        return;
    default:
        break; // Playing
    }

    // 5. UI初期化 (初回のみ)
    if (!m_uiInitialized)
    {
        InitGameUI();
        m_uiInitialized = true;
    }

    // 6. ゲームロジック実行
    CheckMapGimmickTrigger(controllerID);       // ギミック踏んだら強制TopView
    HandleInputAndStateSwitch(controllerID);    // 視点切替入力
    UpdateTimerAndRules(deltaTime, controllerID); // 時間・クリア判定
    CheckSceneTransition(controllerID);         // シーン遷移チェック

    // 7. ゲーム中のUI更新
    if (!state.isGameOver && !state.isGameClear)
    {
        UpdateTopViewUI(controllerID);
        UpdateScanLine(deltaTime, controllerID);
        UpdateSonarEffect(deltaTime, controllerID);
        UpdateGameUI(deltaTime, controllerID); // 演出付きUI更新
        CheckDoorUnlock(controllerID);
    }

    // 8. ゴール（脱出）判定
    if (state.sequenceState == GameSequenceState::Playing && !state.isGameOver)
    {
        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        EntityID exitDoorID = FindExitDoor();

        if (playerID != INVALID_ENTITY_ID && exitDoorID != INVALID_ENTITY_ID)
        {
            auto& door = m_coordinator->GetComponent<DoorComponent>(exitDoorID);
            if (door.state == DoorState::Open)
            {
                auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
                auto& dTrans = m_coordinator->GetComponent<TransformComponent>(exitDoorID);

                float distSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&pTrans.position) - XMLoadFloat3(&dTrans.position)));
                if (distSq < 4.0f) // 2m以内
                {
                    // 脱出シーケンス開始
                    state.sequenceState = GameSequenceState::Exiting;
                    state.sequenceTimer = 0.0f;

                    // 固定カメラ設定
                    if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
                    {
                        XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);
                        float rad = dTrans.rotation.y;
                        XMVECTOR doorDir = XMVectorSet(sin(rad), 0.0f, cos(rad), 0.0f);
                        XMVECTOR camPosVec = doorPos + (doorDir * 3.0f) + XMVectorSet(0.0f, 2.0f, 0.0f, 0.0f);
                        XMVECTOR lookAtVec = doorPos + XMVectorSet(0.0f, 1.5f, 0.0f, 0.0f);

                        XMFLOAT3 camPos, lookAt;
                        XMStoreFloat3(&camPos, camPosVec);
                        XMStoreFloat3(&lookAt, lookAtVec);
                        camSys->SetFixedCamera(camPos, lookAt);
                    }

                    // BGM停止
                    for (auto const& entity : m_coordinator->GetActiveEntities())
                    {
                        if (!m_coordinator->HasComponent<SoundComponent>(entity)) continue;
                        auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
                        if (sound.assetID == "BGM_ACTION" || sound.assetID == "BGM_ALLGET") {
                            sound.RequestStop();
                        }
                    }
                    EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_CLEAR", 0.8f);
                }
            }
        }
    }
}

// ---------------------------------------------------------
// 捕獲シーケンス開始トリガー
// ---------------------------------------------------------
void GameControlSystem::TriggerCaughtSequence(ECS::EntityID guardID)
{
    EntityID controllerID = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == INVALID_ENTITY_ID) return;

    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    if (state.sequenceState == GameSequenceState::Caught || state.isGameOver) return;

    state.sequenceState = GameSequenceState::Caught;
    state.sequenceTimer = 0.0f;
    m_catchingGuardID = guardID;
    m_caughtAnimPlayed = false;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);

    if (playerID != INVALID_ENTITY_ID && guardID != INVALID_ENTITY_ID)
    {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& gTrans = m_coordinator->GetComponent<TransformComponent>(guardID);

        // 1. アニメーション
        if (m_coordinator->HasComponent<AnimationComponent>(guardID)) {
            m_coordinator->GetComponent<AnimationComponent>(guardID).Play("A_GUARD_RUN");
        }
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
            m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_IDLE");
        }

        // 2. カメラ設定
        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
        {
            XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
            XMVECTOR gPos = XMLoadFloat3(&gTrans.position);
            XMVECTOR midPoint = (pPos + gPos) * 0.5f;

            XMVECTOR camOffset = XMVectorSet(2.0f, 2.5f, -3.0f, 0.0f);
            XMVECTOR camPosVec = midPoint + camOffset;
            XMVECTOR lookAtVec = midPoint;

            XMFLOAT3 camPos, lookAt;
            XMStoreFloat3(&camPos, camPosVec);
            XMStoreFloat3(&lookAt, lookAtVec);

            camSys->SetFixedCamera(camPos, lookAt);
        }

        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_FOUND");
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

    // タイム表示の位置を下げる (50.0f -> 100.0f)
    float startX = 50.0f;
    float startY = 100.0f; // 黒帯(最大80px)を避けるため下げる
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
}

void GameControlSystem::InitVisualEffects()
{
    // クロスヘア (4パーツ) + シネマバー (2パーツ) = 合計6個
    auto SafeDestroy = [&](ECS::EntityID& id) {
        if (id != INVALID_ENTITY_ID && m_coordinator->HasComponent<TransformComponent>(id)) {
            m_coordinator->DestroyEntity(id);
        }
        id = INVALID_ENTITY_ID;
        };
    SafeDestroy(m_crosshair.top); SafeDestroy(m_crosshair.bottom);
    SafeDestroy(m_crosshair.left); SafeDestroy(m_crosshair.right);
    SafeDestroy(m_cinemaBarTop); SafeDestroy(m_cinemaBarBottom);

    float thickness = 2.0f;
    float length = 12.0f;

    // 白くて薄い棒を作る
    auto CreateBar = [&](float w, float h) {
        return m_coordinator->CreateEntity(
            TransformComponent({ 0,0,0 }, { 0,0,0 }, { w, h, 1.0f }),
            UIImageComponent("FADE_WHITE", 3.0f, true, { 1.0f, 1.0f, 1.0f, 0.8f })
        );
        };

    m_crosshair.top = CreateBar(thickness, length);
    m_crosshair.bottom = CreateBar(thickness, length);
    m_crosshair.left = CreateBar(length, thickness);
    m_crosshair.right = CreateBar(length, thickness);

    // シネマティックバー (上下の黒帯)
    m_cinemaBarTop = m_coordinator->CreateEntity(
        TransformComponent({ 0,0,0 }, { 0,0,0 }, { 1,1,1 }),
        UIImageComponent("FADE_WHITE", 4.0f, true, { 0.0f, 0.0f, 0.0f, 1.0f }) // 黒
    );
    m_cinemaBarBottom = m_coordinator->CreateEntity(
        TransformComponent({ 0,0,0 }, { 0,0,0 }, { 1,1,1 }),
        UIImageComponent("FADE_WHITE", 4.0f, true, { 0.0f, 0.0f, 0.0f, 1.0f }) // 黒
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

    // スライドイン演出
    float slideT = std::min(1.0f, m_uiAppearTimer / 0.6f);
    float slideEase = 1.0f - powf(1.0f - slideT, 3.0f);
    float slideOffset = (1.0f - slideEase) * -200.0f;

    // タイム表示更新
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
        if (!m_coordinator->HasComponent<UIImageComponent>(m_timerDigits[i])) continue; // 安全策

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

    // アイテムHUD更新
    if (m_coordinator->HasComponent<ItemTrackerComponent>(controllerID)) {
        auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
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

    // ★演出更新呼び出し
    UpdateVisualEffects(deltaTime, controllerID);
}

// ---------------------------------------------------------
// ★ 新演出更新 (レティクル & シネマティックバー)
// ---------------------------------------------------------
void GameControlSystem::UpdateVisualEffects(float deltaTime, ECS::EntityID controllerID)
{
    if (!m_coordinator->HasComponent<GameStateComponent>(controllerID)) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool isAction = (state.currentMode == GameMode::ACTION_MODE) &&
        (state.sequenceState == GameSequenceState::Playing);

    // 入力判定
    bool isMoving = (IsKeyPress('W') || IsKeyPress('A') || IsKeyPress('S') || IsKeyPress('D'));
    XMFLOAT2 stick = GetLeftStick();
    if (abs(stick.x) > 0.1f || abs(stick.y) > 0.1f) isMoving = true;

    // --- 1. シネマティックバー (映画的な黒帯) ---
    float targetHeight = 0.0f;
    if (state.currentMode == GameMode::SCOUTING_MODE) {
        targetHeight = 0.0f; // トップビューでは帯なし
    }
    else {
        // 移動中は視界確保のため細く(30px)、潜伏中は緊張感のため太く(80px)
        if (isMoving) targetHeight = 30.0f;
        else          targetHeight = 80.0f;
    }

    auto UpdateBar = [&](ECS::EntityID id, bool isTop) {
        if (id == INVALID_ENTITY_ID) return;
        if (!m_coordinator->HasComponent<TransformComponent>(id)) return;
        auto& trans = m_coordinator->GetComponent<TransformComponent>(id);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(id);

        // 滑らかに伸縮
        float currentH = trans.scale.y;
        float newH = currentH + (targetHeight - currentH) * 5.0f * deltaTime;

        trans.scale = { (float)SCREEN_WIDTH, newH, 1.0f };
        if (isTop) trans.position = { SCREEN_WIDTH * 0.5f, newH * 0.5f, 0.0f };
        else       trans.position = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - (newH * 0.5f), 0.0f };

        ui.isVisible = (newH > 1.0f);
        };
    UpdateBar(m_cinemaBarTop, true);
    UpdateBar(m_cinemaBarBottom, false);


    // --- 2. タクティカル・クロスヘア (動的レティクル) ---
    bool showCrosshair = isAction;

    auto SetVis = [&](ECS::EntityID id) {
        if (id != INVALID_ENTITY_ID && m_coordinator->HasComponent<UIImageComponent>(id))
            m_coordinator->GetComponent<UIImageComponent>(id).isVisible = showCrosshair;
        };
    SetVis(m_crosshair.top); SetVis(m_crosshair.bottom);
    SetVis(m_crosshair.left); SetVis(m_crosshair.right);

    if (showCrosshair) {
        // 広がりの計算
        float targetSpread = (isMoving) ? 25.0f : 8.0f; // 移動:広がる / 停止:締まる
        m_crosshairSpread += (targetSpread - m_crosshairSpread) * 15.0f * deltaTime;

        float cx = SCREEN_WIDTH * 0.5f;
        float cy = SCREEN_HEIGHT * 0.5f;

        // 座標更新
        if (m_crosshair.top != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<TransformComponent>(m_crosshair.top).position = { cx, cy - m_crosshairSpread, 0.0f };

        if (m_crosshair.bottom != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<TransformComponent>(m_crosshair.bottom).position = { cx, cy + m_crosshairSpread, 0.0f };

        if (m_crosshair.left != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<TransformComponent>(m_crosshair.left).position = { cx - m_crosshairSpread, cy, 0.0f };

        if (m_crosshair.right != INVALID_ENTITY_ID)
            m_coordinator->GetComponent<TransformComponent>(m_crosshair.right).position = { cx + m_crosshairSpread, cy, 0.0f };
    }
}

// ---------------------------------------------------------
// ポーズ関連 (フェード撤廃・安全版)
// ---------------------------------------------------------
void GameControlSystem::TogglePauseRequest()
{
    if (m_pauseState == PauseState::Hidden) {
        m_pauseState = PauseState::AnimateIn;
        m_pauseTimer = 0.0f;
        CreateStylePauseUI();
        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_SYSTEM_OPEN");
    }
    else if (m_pauseState == PauseState::Active) {
        m_pauseState = PauseState::AnimateOut;
        m_pauseTimer = 0.0f;
    }
}

ECS::EntityID GameControlSystem::CreateStyledButton(float targetX, float targetY, float w, float h, const std::string& assetID, std::function<void()> onClick) {
    float tiltRad = XMConvertToRadians(10.0f);

    // 背景プレート (ボタンサイズに合わせて調整)
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

    // ボタン本体
    ECS::EntityID btnID = m_coordinator->CreateEntity(
        TransformComponent({ targetX - 600.0f, targetY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { w, h, 1.0f }),
        UIImageComponent(assetID, 12.0f, true, { 0.7f, 0.7f, 0.7f, 1.0f }), // Normal: グレー
        UIButtonComponent(ButtonState::Normal, true, onClick)
    );
    m_pauseUIEntities.push_back(btnID);
    m_btnBgMap[btnID] = bgID;
    return btnID;
}

void GameControlSystem::CreateStylePauseUI() {
    m_pauseUIEntities.clear(); m_btnBgMap.clear();
    float tiltRad = XMConvertToRadians(10.0f);
    m_pauseBgOverlayID = m_coordinator->CreateEntity(
        TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }),
        UIImageComponent("FADE_WHITE", 10.0f, true, { 0.0f, 0.0f, 0.0f, 0.0f })
    );
    m_pauseUIEntities.push_back(m_pauseBgOverlayID);
    m_pauseDecoSlashID = m_coordinator->CreateEntity(
        TransformComponent({ -SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0.0f, 0.0f, tiltRad }, { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 2.5f, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.0f, true, { 0.02f, 0.02f, 0.02f, 0.90f })
    );
    m_pauseUIEntities.push_back(m_pauseDecoSlashID);
    m_pauseDecoLineID = m_coordinator->CreateEntity(
        TransformComponent({ -SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 8.0f, SCREEN_HEIGHT * 2.5f, 1.0f }),
        UIImageComponent("FADE_WHITE", 11.1f, true, { 1.0f, 0.2f, 0.4f, 1.0f })
    );
    m_pauseUIEntities.push_back(m_pauseDecoLineID);

    ECS::EntityID selector = m_coordinator->CreateEntity(
        TransformComponent({ -200.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, XM_PIDIV4 }, { 20.0f, 20.0f, 1.0f }),
        UIImageComponent("FADE_WHITE", 12.5f, true, { 0.0f, 1.0f, 1.0f, 0.0f })
    );
    m_pauseUIEntities.push_back(selector);

    float baseY = SCREEN_HEIGHT * 0.35f, gapY = 110.0f;
    auto GetXForY = [&](float y) { return (SCREEN_WIDTH * 0.25f) - ((y - SCREEN_HEIGHT * 0.5f) * tanf(tiltRad)); };

    // ★修正: アスペクト比計算 (14.5:4, 17:4)
    float baseH = 56.0f;

    // BTN_BACK_POSE (14.5 : 4) -> 3.625
    float w1 = baseH * 3.625f; // ~203
    float y1 = baseY;
    m_pauseItems.btnReverse = CreateStyledButton(GetXForY(y1), y1, w1, baseH, "BTN_BACK_POSE", [this]() {
        m_pendingTransition = nullptr;
        m_pauseState = PauseState::AnimateOut;
        });

    // BTN_RETRY_POSE (17 : 4) -> 4.25
    float w2 = baseH * 4.25f; // ~238
    float y2 = baseY + gapY;
    m_pauseItems.btnRetry = CreateStyledButton(GetXForY(y2), y2, w2, baseH, "BTN_RETRY_POSE", [this]() {
        m_pendingTransition = []() { SceneManager::ChangeScene<GameScene>(); };
        m_pauseState = PauseState::AnimateOut;
        });

    // BTN_SELECT_POSE (17 : 4) -> 4.25
    float w3 = baseH * 4.25f;
    float y3 = baseY + gapY * 2.0f;
    m_pauseItems.btnStage = CreateStyledButton(GetXForY(y3), y3, w3, baseH, "BTN_SELECT_POSE", [this]() {
        m_pendingTransition = []() { SceneManager::ChangeScene<StageSelectScene>(); };
        m_pauseState = PauseState::AnimateOut;
        });

    float sliderY = baseY + gapY * 3.2f;
    m_pauseItems.sliderBar = m_coordinator->CreateEntity(
        TransformComponent({ -500.0f, sliderY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 400, 6, 1 }),
        UIImageComponent("FADE_WHITE", 12.0f, true, { 0.3f, 0.3f, 0.3f, 1.0f })
    );
    m_pauseUIEntities.push_back(m_pauseItems.sliderBar);
    m_pauseItems.sliderKnob = m_coordinator->CreateEntity(
        TransformComponent({ -500.0f, sliderY, 0.0f }, { 0.0f, 0.0f, tiltRad }, { 16, 32, 1 }),
        UIImageComponent("FADE_WHITE", 12.1f, true, { 1.0f, 1.0f, 1.0f, 1.0f })
    );
    m_pauseUIEntities.push_back(m_pauseItems.sliderKnob);
    m_pauseItems.cursor = m_coordinator->CreateEntity(
        TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { 64, 64, 1 }),
        UIImageComponent("ICO_CURSOR", 20.0f, true, { 1, 1, 1, 1 }),
        UICursorComponent(8.0f)
    );
    m_pauseUIEntities.push_back(m_pauseItems.cursor);
}

void GameControlSystem::UpdatePauseSequence(float deltaTime, ECS::EntityID controllerID) {
    m_pauseTimer += deltaTime;
    float tiltRad = XMConvertToRadians(10.0f); float tanTilt = tanf(tiltRad);
    const float ANIM_IN_DURATION = 0.35f; const float ANIM_OUT_DURATION = 0.25f;
    ECS::EntityID selectorID = (m_pauseUIEntities.size() > 3) ? m_pauseUIEntities[3] : INVALID_ENTITY_ID;

    if (m_pauseDecoLineID != INVALID_ENTITY_ID) {
        float breath = 0.5f + 0.3f * sinf(m_pauseTimer * 2.0f);
        m_coordinator->GetComponent<UIImageComponent>(m_pauseDecoLineID).color.w = breath;
    }

    if (m_pauseState == PauseState::AnimateIn) {
        float t = std::min(1.0f, m_pauseTimer / ANIM_IN_DURATION); float ease = 1.0f - powf(1.0f - t, 4.0f);
        if (m_pauseBgOverlayID != INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_pauseBgOverlayID).color.w = t * 0.4f;
        float slashW = SCREEN_WIDTH * 0.5f; float startX = -slashW; float targetX = SCREEN_WIDTH * 0.1f; float curX = startX + (targetX - startX) * ease;
        if (m_pauseDecoSlashID != INVALID_ENTITY_ID) m_coordinator->GetComponent<TransformComponent>(m_pauseDecoSlashID).position.x = curX;
        if (m_pauseDecoLineID != INVALID_ENTITY_ID) m_coordinator->GetComponent<TransformComponent>(m_pauseDecoLineID).position.x = curX + slashW * 0.5f;
        std::vector<ECS::EntityID> items = { m_pauseItems.btnReverse, m_pauseItems.btnRetry, m_pauseItems.btnStage, m_pauseItems.sliderBar };
        auto GetTargetX = [&](float y) { return (SCREEN_WIDTH * 0.25f) - ((y - SCREEN_HEIGHT * 0.5f) * tanTilt); };
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i] == INVALID_ENTITY_ID) continue;
            float delay = i * 0.05f; float itemT = std::max(0.0f, std::min(1.0f, (m_pauseTimer - delay) / (ANIM_IN_DURATION * 0.8f)));
            float itemEase = 1.0f + 2.7f * powf(itemT - 1.0f, 3.0f) + 1.7f * powf(itemT - 1.0f, 2.0f);
            auto& trans = m_coordinator->GetComponent<TransformComponent>(items[i]);
            float finalX = GetTargetX(trans.position.y);
            trans.position.x = -600.0f + (finalX - (-600.0f)) * itemEase;
            if (m_btnBgMap.count(items[i])) {
                m_coordinator->GetComponent<TransformComponent>(m_btnBgMap[items[i]]).position.x = trans.position.x - 30.0f;
                EntityID lineID = m_btnBgMap[items[i]] + 1;
                if (m_coordinator->HasComponent<TransformComponent>(lineID)) m_coordinator->GetComponent<TransformComponent>(lineID).position.x = trans.position.x - 30.0f;
            }
        }
        UpdatePauseSliderState();
        if (m_pauseTimer >= ANIM_IN_DURATION + 0.2f) m_pauseState = PauseState::Active;
    }
    else if (m_pauseState == PauseState::Active) {
        UpdatePauseSliderState();
        float selY = -9999.0f, selX = -9999.0f; bool anyHover = false;
        std::vector<ECS::EntityID> btns = { m_pauseItems.btnReverse, m_pauseItems.btnRetry, m_pauseItems.btnStage };
        auto GetBaseX = [&](float y) { return (SCREEN_WIDTH * 0.25f) - ((y - SCREEN_HEIGHT * 0.5f) * tanTilt); };

        for (auto btnID : btns) {
            if (btnID == INVALID_ENTITY_ID) continue;
            auto& btn = m_coordinator->GetComponent<UIButtonComponent>(btnID); auto& trans = m_coordinator->GetComponent<TransformComponent>(btnID); auto& ui = m_coordinator->GetComponent<UIImageComponent>(btnID);
            ECS::EntityID bgID = m_btnBgMap[btnID]; ECS::EntityID lineID = bgID + 1;
            auto& bgTrans = m_coordinator->GetComponent<TransformComponent>(bgID); auto& bgUI = m_coordinator->GetComponent<UIImageComponent>(bgID);
            float baseX = GetBaseX(trans.position.y);

            if (btn.state == ButtonState::Hover || btn.state == ButtonState::Pressed) {
                float slideDist = 20.0f;
                trans.position.x += ((baseX + slideDist * cosf(tiltRad)) - trans.position.x) * 15.0f * deltaTime;

                float pulse = 0.7f + 0.3f * sinf(m_pauseTimer * 10.0f);
                ui.color = { 0.0f, pulse, pulse, 1.0f }; // Cyan Pulse

                bgUI.color = { 0.1f, 0.1f, 0.1f, 1.0f }; bgTrans.scale.x += (460.0f - bgTrans.scale.x) * 20.0f * deltaTime; bgTrans.position.x = trans.position.x - 30.0f;
                if (m_coordinator->HasComponent<UIImageComponent>(lineID)) {
                    m_coordinator->GetComponent<UIImageComponent>(lineID).color = { 0, 1, 1, 0.5f + 0.5f * sinf(m_pauseTimer * 20.0f) };
                    m_coordinator->GetComponent<TransformComponent>(lineID).scale.x += (460.0f - m_coordinator->GetComponent<TransformComponent>(lineID).scale.x) * 20.0f * deltaTime;
                    m_coordinator->GetComponent<TransformComponent>(lineID).position.x = bgTrans.position.x;
                }
                anyHover = true; selY = trans.position.y + 15.0f; selX = trans.position.x - 200.0f;
            }
            else {
                trans.position.x += (baseX - trans.position.x) * 15.0f * deltaTime;
                ui.color = { 0.7f, 0.7f, 0.7f, 1.0f };
                bgUI.color = { 0.1f, 0.1f, 0.1f, 0.7f }; bgTrans.scale.x += (420.0f - bgTrans.scale.x) * 15.0f * deltaTime; bgTrans.position.x = trans.position.x - 30.0f;
                if (m_coordinator->HasComponent<UIImageComponent>(lineID)) m_coordinator->GetComponent<UIImageComponent>(lineID).color.w = 0.0f;
            }
        }
        if (selectorID != INVALID_ENTITY_ID) {
            auto& sTrans = m_coordinator->GetComponent<TransformComponent>(selectorID); auto& sUI = m_coordinator->GetComponent<UIImageComponent>(selectorID);
            sTrans.rotation.z += 2.0f * deltaTime;
            if (anyHover) { sUI.color.w += (1.0f - sUI.color.w) * 10.0f * deltaTime; sTrans.position.y += (selY - sTrans.position.y) * 10.0f * deltaTime; sTrans.position.x += (selX - sTrans.position.x) * 10.0f * deltaTime; }
            else { sUI.color.w += (0.0f - sUI.color.w) * 10.0f * deltaTime; }
        }
    }
    else if (m_pauseState == PauseState::AnimateOut) {
        float t = std::min(1.0f, m_pauseTimer / ANIM_OUT_DURATION); float ease = t * t * t;
        if (m_pauseBgOverlayID != INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_pauseBgOverlayID).color.w = 0.4f * (1.0f - t);
        float moveDist = 2000.0f * ease;
        if (m_pauseDecoSlashID != INVALID_ENTITY_ID) m_coordinator->GetComponent<TransformComponent>(m_pauseDecoSlashID).position.x -= moveDist;
        if (m_pauseDecoLineID != INVALID_ENTITY_ID) m_coordinator->GetComponent<TransformComponent>(m_pauseDecoLineID).position.x -= moveDist * 1.2f;
        if (selectorID != INVALID_ENTITY_ID) m_coordinator->GetComponent<TransformComponent>(selectorID).position.x -= moveDist;
        std::vector<ECS::EntityID> items = { m_pauseItems.btnReverse, m_pauseItems.btnRetry, m_pauseItems.btnStage, m_pauseItems.sliderBar, m_pauseItems.sliderKnob };
        for (auto id : items) {
            if (id == INVALID_ENTITY_ID) continue;
            m_coordinator->GetComponent<TransformComponent>(id).position.x -= moveDist;
            if (m_btnBgMap.count(id)) {
                m_coordinator->GetComponent<TransformComponent>(m_btnBgMap[id]).position.x -= moveDist;
                EntityID lineID = m_btnBgMap[id] + 1;
                if (m_coordinator->HasComponent<TransformComponent>(lineID)) m_coordinator->GetComponent<TransformComponent>(lineID).position.x -= moveDist;
            }
        }
        if (m_pauseTimer >= ANIM_OUT_DURATION) {
            if (m_pendingTransition) { DestroyPauseUI(); m_pauseState = PauseState::Hidden; m_pendingTransition(); m_pendingTransition = nullptr; }
            else { DestroyPauseUI(); m_pauseState = PauseState::Hidden; }
        }
    }
}

void GameControlSystem::UpdatePauseSliderState()
{
    if (m_pauseItems.sliderKnob == INVALID_ENTITY_ID || m_pauseItems.sliderBar == INVALID_ENTITY_ID) return;

    auto& knobTrans = m_coordinator->GetComponent<TransformComponent>(m_pauseItems.sliderKnob);
    auto& knobUI = m_coordinator->GetComponent<UIImageComponent>(m_pauseItems.sliderKnob);
    auto& barTrans = m_coordinator->GetComponent<TransformComponent>(m_pauseItems.sliderBar);

    float barW = barTrans.scale.x;
    float barLeft = barTrans.position.x - barW * 0.5f;
    float barRight = barTrans.position.x + barW * 0.5f;
    float tiltRad = XMConvertToRadians(10.0f);
    float tanTilt = tanf(tiltRad);

    if (m_pauseState == PauseState::Active) {
        EntityID cursorID = m_pauseItems.cursor;
        if (cursorID != INVALID_ENTITY_ID) {
            auto& cTrans = m_coordinator->GetComponent<TransformComponent>(cursorID);
            auto& cComp = m_coordinator->GetComponent<UICursorComponent>(cursorID);
            bool isTrigger = cComp.isTriggered;
            bool isHold = IsMousePress(0) || IsButtonPress(BUTTON_A);

            if (isTrigger) {
                float cx = cTrans.position.x;
                float cy = cTrans.position.y;
                if (cx >= barLeft - 40 && cx <= barRight + 40 && std::abs(cy - barTrans.position.y) < 60) {
                    m_isDraggingSlider = true;
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_SYSTEM_OPEN", 1.0f);
                }
            }
            if (!isHold) m_isDraggingSlider = false;

            if (m_isDraggingSlider) {
                float cx = cTrans.position.x;
                float newX = std::max(barLeft, std::min(barRight, cx));
                float t = (newX - barLeft) / barW;
                float minSens = 0.001f, maxSens = 0.02f;
                float newSens = minSens + (maxSens - minSens) * t;
                if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
                    camSys->SetMouseSensitivity(newSens);
            }
        }
    }

    float currentSens = 0.005f;
    if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
        currentSens = camSys->GetMouseSensitivity();

    float minSens = 0.001f, maxSens = 0.02f;
    float t = (currentSens - minSens) / (maxSens - minSens);
    t = std::max(0.0f, std::min(1.0f, t));

    knobTrans.position.x = barLeft + (barW * t);
    float diffX = knobTrans.position.x - barTrans.position.x;
    float diffY = diffX * tanTilt;
    knobTrans.position.y = barTrans.position.y - diffY + 5.0f;
    knobTrans.position.x += 3.0f;

    if (m_isDraggingSlider) {
        knobUI.color = { 1, 1, 0, 1 }; knobTrans.scale = { 20, 40, 1 };
    }
    else {
        knobUI.color = { 1, 1, 1, 1 }; knobTrans.scale = { 16, 32, 1 };
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

// ---------------------------------------------------------
// その他のヘルパー
// ---------------------------------------------------------
void GameControlSystem::UpdateTimerAndRules(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.isGameOver || state.isGameClear) return;
    state.elapsedTime += deltaTime;
#ifdef _DEBUG
    if (IsKeyTrigger('C')) { state.isGameClear = true; state.isGameOver = false; }
    if (IsKeyTrigger('O')) { state.isGameOver = true; state.isGameClear = false; }
#endif
}

void GameControlSystem::HandleInputAndStateSwitch(ECS::EntityID controllerID)
{
    bool pressedSpace = IsKeyTrigger(VK_SPACE);
    bool pressedA = IsButtonTriggered(BUTTON_A);
    if (!(pressedSpace || pressedA)) return;

    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.currentMode == GameMode::SCOUTING_MODE) {
        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TOPVIEWSTART", 0.8f);
        StartMosaicSequence(controllerID);
    }
    else if (state.currentMode == GameMode::ACTION_MODE) {
        state.currentMode = GameMode::SCOUTING_MODE;
        ApplyModeVisuals(controllerID);
        for (auto const& e : m_coordinator->GetActiveEntities()) {
            if (!m_coordinator->HasComponent<SoundComponent>(e)) continue;
            auto& snd = m_coordinator->GetComponent<SoundComponent>(e);
            if (snd.assetID == "BGM_TOPVIEW" || snd.assetID == "BGM_TEST") snd.RequestStop();
            if (snd.assetID == "BGM_ACTION" || snd.assetID == "BGM_TEST2") snd.RequestStop();
        }
        ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_ACTION", 0.5f);
    }
    ApplyModeVisuals(controllerID);
}

void GameControlSystem::CheckSceneTransition(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.isGameOver || state.isGameClear) {
        ResultData data;
        data.isCleared = state.isGameClear;
        data.clearTime = state.elapsedTime;
        data.clearedInTime = (state.elapsedTime <= state.timeLimitStar);
        data.wasSpotted = state.wasSpotted;
        data.stageID = GameScene::GetStageNo();

        if (m_coordinator->HasComponent<ItemTrackerComponent>(controllerID)) {
            auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
            data.collectedCount = tracker.collectedItems;
            data.totalItems = tracker.totalItems;
            data.collectedAllOrdered = tracker.useOrderedCollection;
            data.collectedItemIcons.clear();
            data.orderedItemIcons.clear();
            data.orderedItemCollected.clear();

            for (const auto& targetID : tracker.targetItemIDs) {
                bool isCollected = true;
                for (auto const& entity : m_coordinator->GetActiveEntities()) {
                    if (!m_coordinator->HasComponent<CollectableComponent>(entity)) continue;
                    auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
                    if (col.itemID != targetID) continue;
                    if (!col.isCollected) isCollected = false;
                    break;
                }
                std::string iconName = GetItemIconPath(targetID);
                if (isCollected) data.collectedItemIcons.push_back(iconName);
                data.orderedItemIcons.push_back(iconName);
                data.orderedItemCollected.push_back(isCollected);
            }
        }
        ResultScene::SetResultData(data);
        SceneManager::ChangeScene<ResultScene>();
    }
}

void GameControlSystem::UpdateTopViewUI(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool showIcons = (state.currentMode == GameMode::SCOUTING_MODE);

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID) {
        if (showIcons) UpdateIcon(playerID, "ICO_PLAYER", { 1, 1, 1, 1 });
        else if (m_iconMap.count(playerID)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[playerID]).isVisible = false;
    }

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<CollectableComponent>(entity)) {
            auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
            if (!col.isCollected && showIcons) {
                std::string asset = GetItemIconPath(col.itemID);
                UpdateIcon(entity, asset, { 1, 1, 1, 1 });
            }
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
        }
        bool isGuard = false, isTeleporter = false, isStopTrap = false;
        if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "taser") isGuard = true;
            if (tag == "teleporter") isTeleporter = true;
            if (tag == "stop_trap") isStopTrap = true;
        }
        if (isGuard) {
            if (showIcons) UpdateIcon(entity, "ICO_TASER", { 1, 1, 1, 1 });
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
        }
        if (isTeleporter) {
            if (showIcons) UpdateIcon(entity, "UI_TITLE_LOGO", { 0, 1, 1, 1 });
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
        }
        if (isStopTrap) {
            if (showIcons) UpdateIcon(entity, "UI_ASHIATO_BLUE", { 0.8f, 0.0f, 0.8f, 1.0f });
            else if (m_iconMap.count(entity))  m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
            
        }
    }

    if (!showIcons) return;
    EntityID cameraID = FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    if (cameraID == INVALID_ENTITY_ID) return;
    auto& camera = m_coordinator->GetComponent<CameraComponent>(cameraID);
    XMMATRIX view = XMMatrixTranspose(XMLoadFloat4x4(&camera.viewMatrix));
    XMMATRIX proj = XMMatrixTranspose(XMLoadFloat4x4(&camera.projectionMatrix));
    XMMATRIX viewProj = view * proj;

    for (auto& pair : m_iconMap) {
        EntityID target = pair.first;
        EntityID icon = pair.second;
        auto& iconUI = m_coordinator->GetComponent<UIImageComponent>(icon);
        if (!iconUI.isVisible) continue;
        if (!m_coordinator->HasComponent<TransformComponent>(target)) { iconUI.isVisible = false; continue; }

        auto& targetTrans = m_coordinator->GetComponent<TransformComponent>(target);
        XMFLOAT3 screenPos = GetScreenPosition(targetTrans.position, viewProj);
        auto& iconTrans = m_coordinator->GetComponent<TransformComponent>(icon);
        iconTrans.position = { screenPos.x, screenPos.y, 0.0f };
        if (screenPos.z < 0.0f || screenPos.z > 1.0f) iconUI.isVisible = false;
    }
}

void GameControlSystem::UpdateCaughtSequence(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID || m_catchingGuardID == INVALID_ENTITY_ID) {
        state.isGameOver = true; CheckSceneTransition(controllerID); return;
    }
    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
    auto& gTrans = m_coordinator->GetComponent<TransformComponent>(m_catchingGuardID);

    if (!m_caughtAnimPlayed) {
        XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
        XMVECTOR gPos = XMLoadFloat3(&gTrans.position);
        XMVECTOR dirVec = XMVectorSubtract(pPos, gPos);
        dirVec = XMVectorSetY(dirVec, 0.0f);
        float distance = XMVectorGetX(XMVector3Length(dirVec));
        dirVec = XMVector3Normalize(dirVec);

        if (distance > 1.5f) {
            float moveSpeed = 3.5f * deltaTime;
            XMVECTOR newPos = gPos + (dirVec * moveSpeed);
            float originalY = gTrans.position.y;
            XMStoreFloat3(&gTrans.position, newPos);
            gTrans.position.y = originalY;
            float dx = XMVectorGetX(dirVec); float dz = XMVectorGetZ(dirVec);
            gTrans.rotation.y = atan2(dx, dz);
            pTrans.rotation.y = atan2(-dx, -dz);
        }
        else {
            m_caughtAnimPlayed = true;
            state.sequenceTimer = 0.0f;
            if (m_coordinator->HasComponent<AnimationComponent>(m_catchingGuardID))
                m_coordinator->GetComponent<AnimationComponent>(m_catchingGuardID).Play("A_GUARD_ATTACK", false);
            if (m_coordinator->HasComponent<AnimationComponent>(playerID))
                m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_CAUGHT", false);
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_HIT");
        }
    }
    else {
        if (state.sequenceTimer > 2.0f) {
            state.isGameOver = true; CheckSceneTransition(controllerID);
        }
    }
}

void GameControlSystem::UpdateIcon(ECS::EntityID target, std::string iconAsset, DirectX::XMFLOAT4 color)
{
    if (m_iconMap.find(target) == m_iconMap.end()) {
        EntityID icon = m_coordinator->CreateEntity(
            TransformComponent({ 0, 0, 0 }, { 0, 0, 0 }, { 32, 32, 1 }),
            UIImageComponent(iconAsset, 1.0f, true, color)
        );
        m_iconMap[target] = icon;
    }
    EntityID icon = m_iconMap[target];
    m_coordinator->GetComponent<UIImageComponent>(icon).isVisible = true;
}

void GameControlSystem::UpdateScanLine(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);
    EntityID cameraID = FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    XMMATRIX viewProj = XMMatrixIdentity();
    if (cameraID != INVALID_ENTITY_ID) {
        auto& camera = m_coordinator->GetComponent<CameraComponent>(cameraID);
        viewProj = XMMatrixTranspose(XMLoadFloat4x4(&camera.viewMatrix)) * XMMatrixTranspose(XMLoadFloat4x4(&camera.projectionMatrix));
    }

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<ScanLineComponent>(entity)) continue;
        auto& scan = m_coordinator->GetComponent<ScanLineComponent>(entity);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
        ui.isVisible = isScouting;
        if (!isScouting) continue;

        float prevY = trans.position.y;
        float move = scan.speed * deltaTime;
        if (scan.movingDown) {
            trans.position.y += move;
            if (trans.position.y >= scan.endY) { trans.position.y = scan.endY; scan.movingDown = false; }
        }
        else {
            trans.position.y -= move;
            if (trans.position.y <= scan.startY) { trans.position.y = scan.startY; scan.movingDown = true; }
        }
        float currY = trans.position.y;
        trans.position.x = SCREEN_WIDTH * 0.5f;

        for (auto& pair : m_iconMap) {
            EntityID target = pair.first; EntityID icon = pair.second;
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

void GameControlSystem::UpdateSonarEffect(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);

    if (isScouting) {
        m_sonarSpawnTimer += deltaTime;
        if (m_sonarSpawnTimer >= 1.0f) {
            m_sonarSpawnTimer = 0.0f;
            EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
            if (playerID != INVALID_ENTITY_ID && m_iconMap.count(playerID)) {
                auto& iconTrans = m_coordinator->GetComponent<TransformComponent>(m_iconMap[playerID]);
                m_coordinator->CreateEntity(
                    TransformComponent(iconTrans.position, { 0, 0, 0 }, { 0, 0, 1 }),
                    UIImageComponent("UI_SONAR", 4.0f, true, { 0.0f, 1.0f, 0.5f, 1.0f }),
                    SonarComponent(1.5f, 0.0f, 500.0f)
                );
            }
        }
    }
    std::vector<EntityID> toDestroy;
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<SonarComponent>(entity)) continue;
        if (!isScouting) { toDestroy.push_back(entity); continue; }
        auto& sonar = m_coordinator->GetComponent<SonarComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);
        sonar.timer += deltaTime;
        float progress = sonar.timer / sonar.maxTime;
        if (progress >= 1.0f) { toDestroy.push_back(entity); continue; }
        float scale = sonar.startScale + (sonar.maxScale - sonar.startScale) * progress;
        trans.scale = { scale, scale, 1.0f };
        ui.color.w = 0.5f - (progress * progress);
    }
    for (auto id : toDestroy) m_coordinator->DestroyEntity(id);
}

DirectX::XMFLOAT3 GameControlSystem::GetScreenPosition(const DirectX::XMFLOAT3& worldPos, const DirectX::XMMATRIX& viewProj)
{
    XMVECTOR wPos = XMLoadFloat3(&worldPos);
    XMVECTOR clipPos = XMVector3TransformCoord(wPos, viewProj);
    XMFLOAT3 ndc; XMStoreFloat3(&ndc, clipPos);
    float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
    float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;
    return XMFLOAT3(screenX, screenY, ndc.z);
}

void GameControlSystem::SpawnSmallSonar(const XMFLOAT3& screenPos, XMFLOAT4 color)
{
    m_coordinator->CreateEntity(
        TransformComponent({ screenPos.x, screenPos.y, 0.0f }, { 0,0,0 }, { 0,0,1 }),
        UIImageComponent("UI_SONAR", 4.0f, true, color),
        SonarComponent(1.0f, 0.0f, 200.0f)
    );
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
            XMFLOAT3 camPos, lookAt;
            XMStoreFloat3(&camPos, camPosVec); XMStoreFloat3(&lookAt, lookAtVec);
            camSys->SetFixedCamera(camPos, lookAt);
        }
        if (m_coordinator->HasComponent<AnimationComponent>(doorID)) m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_OPEN", false);
        if (m_coordinator->HasComponent<CollisionComponent>(doorID)) m_coordinator->GetComponent<CollisionComponent>(doorID).type = COLLIDER_TRIGGER;
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DOOR_OPEN");
    }
}

void GameControlSystem::UpdateEntranceSequence(float deltaTime, EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;
    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    if (state.sequenceTimer < 2.5f) {
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN", true);
        float speed = 4.0f * deltaTime;
        float rad = pTrans.rotation.y;
        pTrans.position.x += sin(rad) * speed; pTrans.position.z += cos(rad) * speed;
    }
    else if (state.sequenceTimer < 4.5f) {
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_IDLE");
        if (state.sequenceTimer - deltaTime < 2.5f) {
            EntityID doorID = FindEntranceDoor();
            if (doorID != INVALID_ENTITY_ID) {
                if (m_coordinator->HasComponent<AnimationComponent>(doorID)) m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_CLOSE", false);
                if (m_coordinator->HasComponent<CollisionComponent>(doorID)) m_coordinator->GetComponent<CollisionComponent>(doorID).type = COLLIDER_STATIC;
                EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DOOR_CLOSE");
            }
        }
    }
    else {
        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) {
            camSys->ReleaseFixedCamera();
            camSys->ResetCameraAngle(pTrans.rotation.y, 0.6f);
        }
        state.sequenceState = GameSequenceState::Playing;
    }
}

void GameControlSystem::CheckDoorUnlock(EntityID controllerID)
{
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
                ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_TEST3", 0.5f);
            }
        }
    }
}

void GameControlSystem::UpdateExitSequence(float deltaTime, EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    EntityID exitDoorID = FindExitDoor();

    if (playerID != INVALID_ENTITY_ID && exitDoorID != INVALID_ENTITY_ID) {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& dTrans = m_coordinator->GetComponent<TransformComponent>(exitDoorID);

        if (state.sequenceTimer < 4.0f) {
            if (m_coordinator->HasComponent<AnimationComponent>(playerID)) m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN");
            float targetRot = dTrans.rotation.y + XM_PI;
            float currentRot = pTrans.rotation.y;
            float diff = targetRot - currentRot;
            while (diff > XM_PI) diff -= XM_2PI; while (diff < -XM_PI) diff += XM_2PI;
            pTrans.rotation.y += diff * 5.0f * deltaTime;
            float speed = 2.0f * deltaTime;
            pTrans.position.x += sin(pTrans.rotation.y) * speed;
            pTrans.position.z += cos(pTrans.rotation.y) * speed;
        }
        if (state.sequenceTimer > 2.5f) {
            bool isOpen = false;
            if (m_coordinator->HasComponent<CollisionComponent>(exitDoorID))
                if (m_coordinator->GetComponent<CollisionComponent>(exitDoorID).type == COLLIDER_TRIGGER) isOpen = true;
            if (isOpen) {
                if (m_coordinator->HasComponent<AnimationComponent>(exitDoorID)) m_coordinator->GetComponent<AnimationComponent>(exitDoorID).Play("A_DOOR_CLOSE", false);
                if (m_coordinator->HasComponent<CollisionComponent>(exitDoorID)) m_coordinator->GetComponent<CollisionComponent>(exitDoorID).type = COLLIDER_STATIC;
                EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DOOR_CLOSE");
            }
        }
    }
    if (state.sequenceTimer > 5.0f) {
        state.isGameClear = true; CheckSceneTransition(controllerID);
    }
}

EntityID GameControlSystem::FindEntranceDoor()
{
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<DoorComponent>(entity))
            if (m_coordinator->GetComponent<DoorComponent>(entity).isEntrance) return entity;
    }
    return INVALID_ENTITY_ID;
}

EntityID GameControlSystem::FindExitDoor()
{
    return FindEntranceDoor();
}

bool GameControlSystem::IsAABBOverlap(ECS::EntityID a, ECS::EntityID b)
{
    if (a == INVALID_ENTITY_ID || b == INVALID_ENTITY_ID) return false;
    if (!m_coordinator->HasComponent<TransformComponent>(a) || !m_coordinator->HasComponent<TransformComponent>(b)) return false;
    if (!m_coordinator->HasComponent<CollisionComponent>(a) || !m_coordinator->HasComponent<CollisionComponent>(b)) return false;

    auto makeMinMax = [](const TransformComponent& t, const CollisionComponent& c, XMFLOAT3& min, XMFLOAT3& max) {
        float cx = t.position.x + c.offset.x; float cy = t.position.y + c.offset.y; float cz = t.position.z + c.offset.z;
        min = { cx - c.size.x, cy - c.size.y, cz - c.size.z }; max = { cx + c.size.x, cy + c.size.y, cz + c.size.z };
        };
    const auto& ta = m_coordinator->GetComponent<TransformComponent>(a); const auto& ca = m_coordinator->GetComponent<CollisionComponent>(a);
    const auto& tb = m_coordinator->GetComponent<TransformComponent>(b); const auto& cb = m_coordinator->GetComponent<CollisionComponent>(b);
    XMFLOAT3 amin, amax, bmin, bmax;
    makeMinMax(ta, ca, amin, amax); makeMinMax(tb, cb, bmin, bmax);
    return (amin.x <= bmax.x && amax.x >= bmin.x && amin.y <= bmax.y && amax.y >= bmin.y && amin.z <= bmax.z && amax.z >= bmin.z);
}

void GameControlSystem::CheckMapGimmickTrigger(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    if (state.currentMode != GameMode::ACTION_MODE) return;
    if (state.sequenceState != GameSequenceState::Playing) return;
    ECS::EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == ECS::INVALID_ENTITY_ID) return;

    for (auto const& e : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<TagComponent>(e)) continue;
        auto& tag = m_coordinator->GetComponent<TagComponent>(e).tag;
        if (tag != "map_gimmick") continue;
        if (!IsAABBOverlap(playerID, e)) continue;

        state.currentMode = GameMode::SCOUTING_MODE;
        ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TOPVIEWSTART", 0.8f);
        ApplyModeVisuals(controllerID);
        tag = "map_gimmick_used";
        if (m_coordinator->HasComponent<CollisionComponent>(e)) m_coordinator->GetComponent<CollisionComponent>(e).size = { 0,0,0 };
        break;
    }
}

void GameControlSystem::StartMosaicSequence(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceState = GameSequenceState::Starting;
    state.sequenceTimer = 0.0f;

    m_blackBackID = m_coordinator->CreateEntity(
        TransformComponent({ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f }, { 0,0,0 }, { SCREEN_WIDTH * 1.1f, SCREEN_HEIGHT * 1.1f, 1.0f }),
        UIImageComponent("FADE_WHITE", 100000.0f, true, { 0, 0, 0, 0 })
    );

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID && m_coordinator->HasComponent<RenderComponent>(playerID))
        m_coordinator->GetComponent<RenderComponent>(playerID).type = MESH_NONE;

    for (auto& pair : m_iconMap) {
        if (m_coordinator->HasComponent<UIImageComponent>(pair.second))
            m_coordinator->GetComponent<UIImageComponent>(pair.second).isVisible = false;
    }

    m_mosaicTiles.clear();
    float tileW = (float)SCREEN_WIDTH / TILE_COLS; float tileH = (float)SCREEN_HEIGHT / TILE_ROWS;
    float uvUnitX = 1.0f / TILE_COLS; float uvUnitY = 1.0f / TILE_ROWS;

    for (int y = 0; y < TILE_ROWS; ++y) {
        for (int x = 0; x < TILE_COLS; ++x) {
            float tX = (x * tileW) + (tileW * 0.5f); float tY = (y * tileH) + (tileH * 0.5f);
            float startY = tY - SCREEN_HEIGHT - 200.0f - (x * 50.0f) - (y * 50.0f);
            EntityID tile = m_coordinator->CreateEntity(
                TransformComponent({ tX, startY, 0.0f }, { 0,0,0 }, { tileW + 2.0f, tileH + 2.0f, 1.0f }),
                UIImageComponent("UI_GAME_START", 100001.0f, true, { 1, 1, 1, 1 })
            );
            auto& ui = m_coordinator->GetComponent<UIImageComponent>(tile);
            ui.uvPos = { x * uvUnitX, y * uvUnitY }; ui.uvScale = { uvUnitX, uvUnitY };
            m_coordinator->GetComponent<TransformComponent>(tile).rotation.x = XM_PIDIV2;
            m_mosaicTiles.push_back(tile);
        }
    }
}

void GameControlSystem::UpdateMosaicSequence(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;
    const float FALL_DURATION = 0.6f, STAY_DURATION = 1.2f, END_DURATION = 0.5f;
    const float TILE_DELAY_X = 0.08f, TILE_DELAY_Y = 0.04f;
    float finishTime = (TILE_COLS * TILE_DELAY_X) + (TILE_ROWS * TILE_DELAY_Y) + FALL_DURATION + STAY_DURATION + END_DURATION;

    if (m_blackBackID != INVALID_ENTITY_ID) {
        auto& bgUI = m_coordinator->GetComponent<UIImageComponent>(m_blackBackID);
        float bgAlpha = state.sequenceTimer / 0.8f; if (bgAlpha > 1.0f) bgAlpha = 1.0f;
        if (state.sequenceTimer > finishTime - END_DURATION) {
            float outT = (state.sequenceTimer - (finishTime - END_DURATION)) / END_DURATION;
            bgAlpha = 1.0f - outT;
        }
        bgUI.color.w = bgAlpha;
    }

    if (state.currentMode == GameMode::SCOUTING_MODE && state.sequenceTimer > 2.0f) {
        state.currentMode = GameMode::ACTION_MODE;
        if (m_blackBackID != INVALID_ENTITY_ID) m_coordinator->GetComponent<UIImageComponent>(m_blackBackID).color.w = 1.0f;
        for (auto const& e : m_coordinator->GetActiveEntities()) {
            if (!m_coordinator->HasComponent<SoundComponent>(e)) continue;
            auto& snd = m_coordinator->GetComponent<SoundComponent>(e);
            if (snd.assetID == "BGM_TOPVIEW" || snd.assetID == "BGM_TEST") snd.RequestStop();
        }
        ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator, "BGM_ACTION", 0.5f);

        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        EntityID doorID = FindEntranceDoor();
        if (playerID != INVALID_ENTITY_ID && doorID != INVALID_ENTITY_ID) {
            auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
            auto& dTrans = m_coordinator->GetComponent<TransformComponent>(doorID);
            float rad = dTrans.rotation.y; float startDist = 5.0f;
            pTrans.position.x = dTrans.position.x - sin(rad) * startDist;
            pTrans.position.z = dTrans.position.z - cos(rad) * startDist;
            pTrans.rotation.y = dTrans.rotation.y;
            if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>()) {
                XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);
                XMVECTOR doorDir = XMVectorSet(sin(rad), 0.0f, cos(rad), 0.0f);
                XMVECTOR camPosVec = doorPos + (doorDir * 7.5f) + XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f);
                XMFLOAT3 camPos; XMStoreFloat3(&camPos, camPosVec);
                camSys->SetFixedCamera(camPos, dTrans.position);
            }
        }

        // ★修正: ここでソナーやスキャンラインを強制非表示にする
        for (auto& pair : m_iconMap) {
            if (m_coordinator->HasComponent<UIImageComponent>(pair.second))
                m_coordinator->GetComponent<UIImageComponent>(pair.second).isVisible = false;
        }
        // ここが重要：ScanLineやSonarを持つエンティティを探して隠す
        for (auto const& e : m_coordinator->GetActiveEntities()) {
            if (m_coordinator->HasComponent<ScanLineComponent>(e) ||
                m_coordinator->HasComponent<SonarComponent>(e)) {
                if (m_coordinator->HasComponent<UIImageComponent>(e)) {
                    m_coordinator->GetComponent<UIImageComponent>(e).isVisible = false;
                }
            }
        }

        ApplyModeVisuals(controllerID);
    }

    float tileH = (float)SCREEN_HEIGHT / TILE_ROWS;
    for (int i = 0; i < m_mosaicTiles.size(); ++i) {
        EntityID tile = m_mosaicTiles[i];
        int col = i % TILE_COLS; int row = i / TILE_COLS;
        float startTime = (col * TILE_DELAY_X) + (row * TILE_DELAY_Y);
        float t = state.sequenceTimer - startTime;
        auto& trans = m_coordinator->GetComponent<TransformComponent>(tile);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(tile);
        float targetY = (row * tileH) + (tileH * 0.5f);

        if (t < FALL_DURATION) {
            if (t < 0.0f) { trans.position.y = -2000.0f; continue; }
            float p = t / FALL_DURATION; float ease = 1.0f - pow(1.0f - p, 4.0f);
            float startY = targetY - 400.0f - (col * 50.0f);
            trans.position.y = startY + (targetY - startY) * ease;
            trans.rotation.x = XM_PIDIV2 * (1.0f - ease); ui.color.w = 1.0f;
        }
        else if (t < FALL_DURATION + STAY_DURATION) {
            trans.position.y = targetY; trans.rotation.x = 0.0f;
        }
        else if (t < FALL_DURATION + STAY_DURATION + END_DURATION) {
            float outT = (t - (FALL_DURATION + STAY_DURATION)) / END_DURATION;
            trans.position.y = targetY + (outT * 300.0f); trans.rotation.x = -XM_PIDIV2 * outT;
            ui.color.w = 1.0f - outT;
        }
        else {
            ui.color.w = 0.0f;
        }
    }

    if (state.sequenceTimer >= finishTime) {
        if (m_blackBackID != INVALID_ENTITY_ID) m_coordinator->DestroyEntity(m_blackBackID);
        for (auto id : m_mosaicTiles) m_coordinator->DestroyEntity(id);
        m_mosaicTiles.clear(); m_blackBackID = INVALID_ENTITY_ID;
        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        if (playerID != INVALID_ENTITY_ID && m_coordinator->HasComponent<RenderComponent>(playerID))
            m_coordinator->GetComponent<RenderComponent>(playerID).type = MESH_MODEL;
        StartEntranceSequence(controllerID);
    }
}

void GameControlSystem::ApplyModeVisuals(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool isScoutingVisual = (state.currentMode == GameMode::SCOUTING_MODE);

    if (state.topviewBgID != INVALID_ENTITY_ID && state.tpsBgID != INVALID_ENTITY_ID) {
        auto& nUI = m_coordinator->GetComponent<UIImageComponent>(state.topviewBgID);
        auto& tUI = m_coordinator->GetComponent<UIImageComponent>(state.tpsBgID);
        nUI.isVisible = isScoutingVisual; tUI.isVisible = !isScoutingVisual;
    }

    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<RenderComponent>(entity)) continue;
        auto& render = m_coordinator->GetComponent<RenderComponent>(entity);
        MeshType actionType = render.type;
        MeshType scoutType = MESH_NONE;

        if (m_coordinator->HasComponent<PlayerControlComponent>(entity) ||
            m_coordinator->HasComponent<CollectableComponent>(entity)) {
            actionType = MESH_MODEL; scoutType = MESH_NONE;
        }
        else if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "guard") { actionType = MESH_MODEL; scoutType = MESH_NONE; }
            else if (tag == "taser" || tag == "map_gimmick") {
                actionType = MESH_NONE; scoutType = MESH_NONE;
            }
            else if (tag == "ground" || tag == "wall") {
                actionType = MESH_MODEL; scoutType = MESH_BOX;
            }
            else if (tag == "door") {
                actionType = MESH_MODEL; scoutType = MESH_MODEL;
            }
        }
        render.type = isScoutingVisual ? scoutType : actionType;
    }
}