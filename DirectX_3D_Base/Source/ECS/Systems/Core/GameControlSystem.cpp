/*****************************************************************//**
 * @file	GameControlSystem.cpp
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
#include "ECS/Systems/Core/GameControlSystem.h"
#include "Scene/SceneManager.h"
#include "ECS/EntityFactory.h"
#include "ECS/ECSInitializer.h"

#include "Scene/ResultScene.h"

#include <cmath>

using namespace DirectX;
using namespace ECS;

std::string GetItemIconPath(const std::string& itemID)
{
    if (itemID == "Takara_Daiya")   return "ICO_TREASURE1";
    if (itemID == "Takara_Crystal") return "ICO_TREASURE2";
    if (itemID == "Takara_Yubiwa")  return "ICO_TREASURE3";
    if (itemID == "Takara_Kaiga1")  return "ICO_TREASURE4";
    if (itemID == "Takara_Kaiga2")  return "ICO_TREASURE5";
    if (itemID == "Takara_Kaiga3")  return "ICO_TREASURE6";


    // デフォルト
    return "ICO_TREASURE";
}

void GameControlSystem::Update(float deltaTime)
{
    // 1. コントローラー（GameStateを持つEntity）を取得
    EntityID controllerID = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == INVALID_ENTITY_ID) return;
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // モード切替検知 (TopView -> Action)
    if (state.currentMode == GameMode::ACTION_MODE && state.sequenceState == GameSequenceState::None)
    {
        // アクションモードに入った瞬間、入場演出を開始
        StartEntranceSequence(controllerID);
    }

    // シーケンス処理
    if (state.sequenceState == GameSequenceState::Entering)
    {
        UpdateEntranceSequence(deltaTime, controllerID);
        return; // 演出中はゲームロジックを止める
    }
    else if (state.sequenceState == GameSequenceState::Exiting)
    {
        UpdateExitSequence(deltaTime, controllerID);
        return;
    }
    else if (state.sequenceState == GameSequenceState::Caught) {
        UpdateCaughtSequence(deltaTime, controllerID);
        return;
    }

    if (!m_uiInitialized)
    {
        InitGameUI();
        m_uiInitialized = true;
    }

    // 2. 各ロジックを順次実行
    // 処理順序が重要です: 入力 -> 状態更新 -> 判定 -> 遷移 -> UI表示

    CheckMapGimmickTrigger(controllerID); // touch gimmick: force TopView

    HandleInputAndStateSwitch(controllerID); // 入力によるモード変更
    UpdateTimerAndRules(deltaTime, controllerID); // 時間経過とクリア判定
    CheckSceneTransition(controllerID);      // ゲーム終了ならシーン遷移

    // シーン遷移が起きていなければUI更新
    if (!state.isGameOver && !state.isGameClear) {
        UpdateTopViewUI(controllerID);
        UpdateScanLine(deltaTime, controllerID);
        UpdateSonarEffect(deltaTime, controllerID);
        UpdateGameUI(deltaTime, controllerID);
        CheckDoorUnlock(controllerID);
    }

    // もしプレイ中ならゴール判定
    if (state.sequenceState == GameSequenceState::Playing && !state.isGameOver)
    {
        EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
        EntityID exitDoorID = FindExitDoor();

        if (playerID != INVALID_ENTITY_ID && exitDoorID != INVALID_ENTITY_ID)
        {
            // 出口ドアが開いていて、かつプレイヤーが十分近づいたら脱出演出へ
            auto& door = m_coordinator->GetComponent<DoorComponent>(exitDoorID);
            if (door.state == DoorState::Open)
            {
                auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
                auto& dTrans = m_coordinator->GetComponent<TransformComponent>(exitDoorID);

                float distSq = XMVectorGetX(XMVector3LengthSq(XMLoadFloat3(&pTrans.position) - XMLoadFloat3(&dTrans.position)));
                if (distSq < 2.0f * 2.0f) // 2m以内
                {
                    state.sequenceState = GameSequenceState::Exiting;
                    state.sequenceTimer = 0.0f;

                    // 脱出時もカメラを固定（入場時と同じ位置でOK、あるいは逆側）
                    if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
                    {
                        // 入場時と同じ計算で「部屋の中から去っていく背中」を映す
                        // ドアの位置
                        XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);
                        float rad = dTrans.rotation.y;

                        // ドアの向きベクトル (Z+ 方向)
                        // プレイヤーが入場する方向(rad)と同じ向き
                        float sinY = sin(rad);
                        float cosY = cos(rad);
                        XMVECTOR doorDir = XMVectorSet(sinY, 0.0f, cosY, 0.0f);

                        // カメラの位置計算:
                        // ドアから「前方へ4m」、かつ「高さ2m」の位置（部屋の中からドアを見下ろす）
                        XMVECTOR camPosVec = doorPos + (doorDir * 3.0f) + XMVectorSet(0.0f, 2.0f, 0.0f, 0.0f);

                        // 注視点:
                        // ドアの中心（より少し上）を見る
                        XMVECTOR lookAtVec = doorPos + XMVectorSet(0.0f, 1.5f, 0.0f, 0.0f);

                        XMFLOAT3 camPos, lookAt;
                        XMStoreFloat3(&camPos, camPosVec);
                        XMStoreFloat3(&lookAt, lookAtVec);

                        // カメラシステムにセット
                        camSys->SetFixedCamera(camPos, lookAt);
                    }

                    for (auto const& entity : m_coordinator->GetActiveEntities())
                    {
                        if (!m_coordinator->HasComponent<SoundComponent>(entity))
                            continue;

                        auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
                        const auto& id = sound.assetID;

                        // アイテム全回収後まで流れていた BGM_TEST2 を停止
                        if (id == "BGM_ACTION"
                            // もし BGM_TEST3 もここで止めたい場合は ↓ を有効に
                            || id == "BGM_ALLGET")
                        {
                            sound.RequestStop();
                        }
                    }

                    // ゴール演出開始SEを一回だけ鳴らす
                    ECS::EntityFactory::CreateOneShotSoundEntity(
                        m_coordinator,
                        "SE_CLEAR",  // ゴール用SE
                        0.8f         // 音量はお好みで
                    );
                    state.sequenceState = GameSequenceState::Exiting;
                    state.sequenceTimer = 0.0f;
                }
            }
        }
    }



}

void GameControlSystem::TriggerCaughtSequence(ECS::EntityID guardID)
{
    EntityID controllerID = FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    if (controllerID == INVALID_ENTITY_ID) return;

    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // 多重呼び出し防止
    if (state.sequenceState == GameSequenceState::Caught || state.isGameOver) return;

    // ステート変更
    state.sequenceState = GameSequenceState::Caught;
    state.sequenceTimer = 0.0f;
    m_catchingGuardID = guardID;
    m_caughtAnimPlayed = false; // フラグ・リセット

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);

    if (playerID != INVALID_ENTITY_ID && guardID != INVALID_ENTITY_ID)
    {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& gTrans = m_coordinator->GetComponent<TransformComponent>(guardID);

        // 1. アニメーション初期化
        // 警備員: プレイヤーに向かって走る
        if (m_coordinator->HasComponent<AnimationComponent>(guardID)) {
            m_coordinator->GetComponent<AnimationComponent>(guardID).Play("A_GUARD_RUN");
        }
        // プレイヤー: 驚く/立ち止まる
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
            m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_IDLE");
        }

        // 2. カメラ設定（二人の様子が見える位置へ）
        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
        {
            // 中間地点
            XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
            XMVECTOR gPos = XMLoadFloat3(&gTrans.position);
            XMVECTOR midPoint = (pPos + gPos) * 0.5f;

            // カメラ位置: 中間地点から少し離れた場所
            // (例: 高さ2.5m, 奥行き3.0m)
            XMVECTOR camOffset = XMVectorSet(2.0f, 2.5f, -3.0f, 0.0f);
            XMVECTOR camPosVec = midPoint + camOffset;
            XMVECTOR lookAtVec = midPoint;

            XMFLOAT3 camPos, lookAt;
            XMStoreFloat3(&camPos, camPosVec);
            XMStoreFloat3(&lookAt, lookAtVec);

            camSys->SetFixedCamera(camPos, lookAt);
        }

        // プレイヤー発見音
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_FOUND");
    }
}

// ---------------------------------------------------------
// A. 時間管理とルール判定 (旧 GameSceneSystem)
// ---------------------------------------------------------
void GameControlSystem::UpdateTimerAndRules(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // 既に終了していれば更新しない
    if (state.isGameOver || state.isGameClear) return;

    // 時間更新
    state.elapsedTime += deltaTime;
    /*if (state.elapsedTime >= state.timeLimit)
    {
        std::cout << "[GameControl] Time Up!" << std::endl;
        state.isGameOver = true;
    }*/

#ifdef _DEBUG
    // Debug: C=Clear, O=GameOver
    // (avoid setting both flags at once)
    if (IsKeyTrigger('C'))
    {
        state.isGameClear = true;
        state.isGameOver = false;
    }
    if (IsKeyTrigger('O'))
    {
        state.isGameOver = true;
        state.isGameClear = false;
    }
#endif // _DEBUG

}

// ---------------------------------------------------------
// B. 入力と視点切替 (旧 StateSwitchSystem)
// ---------------------------------------------------------
void GameControlSystem::HandleInputAndStateSwitch(ECS::EntityID controllerID)
{
    // スペース or A
    bool pressedSpace = IsKeyTrigger(VK_SPACE);
    bool pressedA = IsButtonTriggered(BUTTON_A);

    if (!(pressedSpace || pressedA)) return;

    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // ここに入れる：スペース / A でSEを鳴らす
    // （トップビュー画面で鳴らしたいなら「反転前」の state.currentMode を見る）
    if (state.currentMode == GameMode::SCOUTING_MODE && (pressedSpace || pressedA))
    {
        ECS::EntityFactory::CreateOneShotSoundEntity(
            m_coordinator,
            "SE_TOPVIEWSTART", // ← Sound.csvのSE IDに置き換え
            0.8f
        );
    }
    // ★追加：切替前モードを保存
    GameMode prevMode = state.currentMode;

    // モード反転
    state.currentMode = (state.currentMode == GameMode::ACTION_MODE)
        ? GameMode::SCOUTING_MODE
        : GameMode::ACTION_MODE;

    // トップビュー → アクションになった瞬間にBGM切替
    if (prevMode == GameMode::SCOUTING_MODE && state.currentMode == GameMode::ACTION_MODE)
    {
        for (auto const& e : m_coordinator->GetActiveEntities())
        {
            if (!m_coordinator->HasComponent<SoundComponent>(e)) continue;
            auto& snd = m_coordinator->GetComponent<SoundComponent>(e);

            // ★トップビュー系は全部止める（IDが揺れてても止まるように）
            if (snd.assetID == "BGM_TOPVIEW"
                || snd.assetID == "BGM_TEST")     // ← もしトップビューBGMをこれで登録してるなら追加
            {
                snd.RequestStop();
            }

            // 念のため：すでに鳴ってるアクションBGMも止めて重複防止
            if (snd.assetID == "BGM_ACTION"
                || snd.assetID == "BGM_TEST2")    // ← もしアクションBGMをこれで登録してるなら追加
            {
                snd.RequestStop();
            }
        }

        // アクション用BGMを開始（IDはプロジェクトに合わせる）
        ECS::EntityFactory::CreateLoopSoundEntity(
            m_coordinator,
            "BGM_ACTION",
            0.5f
        );
    }

    // Apply visuals based on the decided mode (also used by MapGimmick force-switch)
    ApplyModeVisuals(controllerID);
    return;

    // 背景画像の切り替え
    if (state.topviewBgID != INVALID_ENTITY_ID && state.tpsBgID != INVALID_ENTITY_ID)
    {
        auto& normalUI = m_coordinator->GetComponent<UIImageComponent>(state.topviewBgID);
        auto& tpsUI = m_coordinator->GetComponent<UIImageComponent>(state.tpsBgID);

        if (state.currentMode == GameMode::SCOUTING_MODE)
        {
            // スカウティング（トップビュー）モード: 通常背景ON, TPS背景OFF
            // ※あなたの既存コードでは SCOUTING_MODE がトップビュー（BG_TOPVIEWが表示されるべき状態）だと推測されます
            normalUI.isVisible = true;
            tpsUI.isVisible = false;
        }
        else
        {
            // アクション（TPS）モード: 通常背景OFF, TPS背景ON
            normalUI.isVisible = false;
            tpsUI.isVisible = true;
        }
    }

    bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<RenderComponent>(entity)) continue;

        auto& render = m_coordinator->GetComponent<RenderComponent>(entity);
        bool isTarget = false;
        MeshType restoreType = MESH_BOX; // 復帰時のデフォルト

        // プレイヤー
        if (m_coordinator->HasComponent<PlayerControlComponent>(entity)) {
            isTarget = true;
            restoreType = MESH_MODEL;
        }
        // アイテム
        else if (m_coordinator->HasComponent<CollectableComponent>(entity)) {
            isTarget = true;
            restoreType = MESH_MODEL; // アイテムは箱表示
        }
        else if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "guard") {
                isTarget = true;
                restoreType = MESH_MODEL;
            }
            if (tag == "taser")
            {
                isTarget = true;
#ifdef _DEBUG
                restoreType = MESH_BOX;
#elif defined(NDEBUG)
                restoreType = MESH_NONE;
#endif
            }
#ifdef _DEBUG
            restoreType = MESH_BOX;
#elif defined(NDEBUG)
            restoreType = MESH_NONE;
#endif
        if (tag == "teleporter")
        {
            isTarget = true;
}
            if (tag == "ground" || tag == "wall" || tag == "door")
            {
                isTarget = true;
                restoreType = MESH_MODEL;
            }
        }

        // 対象であれば描画モード変更
        if (isTarget)
        {
            if (isScouting) {
                render.type = MESH_NONE; // トップビュー時は描画しない
            }
            else {
                render.type = restoreType; // アクション時は元の形状で描画
            }
        }
    }
}

// ---------------------------------------------------------
// C. シーン遷移管理 (旧 GameFlowSystem)
// ---------------------------------------------------------
void GameControlSystem::CheckSceneTransition(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    if (state.isGameOver || state.isGameClear)
    {
        // リザルト用データを作成
        ResultData data;
        data.isCleared = state.isGameClear;
        data.clearTime = state.elapsedTime;
        data.clearedInTime = (state.elapsedTime <= state.timeLimitStar);
        data.wasSpotted = state.wasSpotted;
        data.stageID = GameScene::GetStageNo();

        // ItemTracker から回収状況をまとめて ResultData に詰める
        if (m_coordinator->HasComponent<ItemTrackerComponent>(controllerID))
        {
            auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);

            data.collectedCount = tracker.collectedItems;
            data.totalItems = tracker.totalItems;
            data.collectedAllOrdered = tracker.useOrderedCollection;

            // いったん全部クリア
            data.collectedItemIcons.clear();
            data.orderedItemIcons.clear();
            data.orderedItemCollected.clear();

            // ステージに用意されているお宝を「順番どおり」に走査
            for (const auto& targetID : tracker.targetItemIDs)
            {
                bool isCollected = true;

                // シーン上の Collectable を探して回収状況を調べる
                for (auto const& entity : m_coordinator->GetActiveEntities())
                {
                    if (!m_coordinator->HasComponent<CollectableComponent>(entity))
                        continue;

                    auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
                    if (col.itemID != targetID)
                        continue;

                    // 残っていて isCollected == false なら「未回収」
                    if (!col.isCollected)
                        isCollected = false;

                    break;
                }

                // アイコン名に変換
                std::string iconName = GetItemIconPath(targetID);

                // クリア画面用：取れたお宝だけ
                if (isCollected)
                {
                    data.collectedItemIcons.push_back(iconName);
                }

                // ゲームオーバー用：全部 + 取れたかどうか
                data.orderedItemIcons.push_back(iconName);
                data.orderedItemCollected.push_back(isCollected);
            }
        }
        else
        {
            data.collectedItemIcons.clear();
            data.orderedItemIcons.clear();
            data.orderedItemCollected.clear();
        }




        // リザルトシーンへ渡して遷移
        ResultScene::SetResultData(data);
        SceneManager::ChangeScene<ResultScene>();
    }
}

// ---------------------------------------------------------
// D. トップビューUI更新 (旧 TopViewUISystem)
// ---------------------------------------------------------
void GameControlSystem::UpdateTopViewUI(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    bool showIcons = (state.currentMode == GameMode::SCOUTING_MODE);

    // 1. アイコンの作成・表示設定

    // プレイヤー
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID != INVALID_ENTITY_ID) {
        if (showIcons) UpdateIcon(playerID, "ICO_PLAYER", { 1, 1, 1, 1 });
        else if (m_iconMap.count(playerID)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[playerID]).isVisible = false;
    }

    // 全エンティティ走査 (アイテム・敵・ギミック)
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        // アイテム
        if (m_coordinator->HasComponent<CollectableComponent>(entity)) {
            auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
            // 未回収なら表示
            if (!col.isCollected && showIcons) {
                std::string asset = GetItemIconPath(col.itemID);
                UpdateIcon(entity, asset, { 1, 1, 1, 1 });
            }
            else if (m_iconMap.count(entity)) {
                m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
            }
        }
        // 敵 (Tag または Component で判定)
        bool isGuard = false;
        bool isTeleporter = false;
        if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "taser") isGuard = true;
            if (tag == "teleporter") isTeleporter = true;
        }

        if (isGuard) {
            if (showIcons) UpdateIcon(entity, "ICO_TASER", { 1, 1, 1, 1 });
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
        }
        if (isTeleporter) {
            if (showIcons) {
                // �A�Z�b�g���Ȃ��̂ŁA������`���V�A���F(���F)�ɂ��ĕ\��
                UpdateIcon(entity, "UI_TITLE_LOGO", { 0.0f, 1.0f, 1.0f, 1.0f });
            }
            else if (m_iconMap.count(entity)) {
                m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
            }
        }
    }

    // 2. アイコン位置の計算と適用
    if (!showIcons) return;

    EntityID cameraID = FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    if (cameraID == INVALID_ENTITY_ID) return;
    auto& camera = m_coordinator->GetComponent<CameraComponent>(cameraID);

    // ★修正ポイント: 行列を転置して元に戻す (DirectXMathの計算用に)
    XMMATRIX view = XMMatrixTranspose(XMLoadFloat4x4(&camera.viewMatrix));
    XMMATRIX proj = XMMatrixTranspose(XMLoadFloat4x4(&camera.projectionMatrix));
    XMMATRIX viewProj = view * proj;

    for (auto& pair : m_iconMap) {
        EntityID target = pair.first;
        EntityID icon = pair.second;
        auto& iconUI = m_coordinator->GetComponent<UIImageComponent>(icon);

        if (!iconUI.isVisible) continue;

        if (!m_coordinator->HasComponent<TransformComponent>(target)) {
            iconUI.isVisible = false;
            continue;
        }
        auto& targetTrans = m_coordinator->GetComponent<TransformComponent>(target);

        // ワールド座標 -> スクリーン座標変換
        XMVECTOR worldPos = XMLoadFloat3(&targetTrans.position);
        // XMVector3TransformCoord は w除算も行ってくれる
        XMVECTOR clipPos = XMVector3TransformCoord(worldPos, viewProj);
        XMFLOAT3 ndc;
        XMStoreFloat3(&ndc, clipPos);

        // NDC (-1.0 ~ 1.0) -> スクリーン座標 (Pixel)
        float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
        float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;

        auto& iconTrans = m_coordinator->GetComponent<TransformComponent>(icon);
        iconTrans.position = { screenX, screenY, 0.0f }; // Zは0 (最前面)

        // 画面外(前後)なら隠す
        // ※クリップ空間のZ範囲はDirectXでは 0.0～1.0
        if (ndc.z < 0.0f || ndc.z > 1.0f) {
            iconUI.isVisible = false;
        }
    }
}

void GameControlSystem::UpdateCaughtSequence(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);

    // 警備員かプレイヤーがいなければ即終了
    if (playerID == INVALID_ENTITY_ID || m_catchingGuardID == INVALID_ENTITY_ID) {
        state.isGameOver = true;
        CheckSceneTransition(controllerID);
        return;
    }

    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
    auto& gTrans = m_coordinator->GetComponent<TransformComponent>(m_catchingGuardID);

    // --- フェーズ1: 接近 ---
    if (!m_caughtAnimPlayed)
    {
        XMVECTOR pPos = XMLoadFloat3(&pTrans.position);
        XMVECTOR gPos = XMLoadFloat3(&gTrans.position);

        // 距離と方向
        XMVECTOR dirVec = XMVectorSubtract(pPos, gPos);

        // Y軸（高さ）の差を無視して、水平方向のみのベクトルにする
        dirVec = XMVectorSetY(dirVec, 0.0f);

        // 水平距離で長さを再計算
        float distance = XMVectorGetX(XMVector3Length(dirVec));
        dirVec = XMVector3Normalize(dirVec);

        // 停止距離
        float stopDist = 1.5f;

        if (distance > stopDist)
        {
            // 近づく
            float moveSpeed = 3.5f * deltaTime;
            XMVECTOR newPos = gPos + (dirVec * moveSpeed);

            // 移動後のY座標は元の高さを維持する (めり込み防止)
            float originalY = gTrans.position.y;
            XMStoreFloat3(&gTrans.position, newPos);
            gTrans.position.y = originalY; // 高さは固定

            // 向き調整
            float dx = XMVectorGetX(dirVec);
            float dz = XMVectorGetZ(dirVec);
            gTrans.rotation.y = atan2(dx, dz);

            pTrans.rotation.y = atan2(-dx, -dz);
        }
        else
        {
            // --- フェーズ2: 到着＆捕獲アクション ---
            m_caughtAnimPlayed = true;
            state.sequenceTimer = 0.0f; // タイマーリセット(アニメ再生待ち用)

            // 警備員: 攻撃/捕獲モーション
            if (m_coordinator->HasComponent<AnimationComponent>(m_catchingGuardID)) {
                // "A_GUARD_ATTACK" や "A_GUARD_CATCH" など
                m_coordinator->GetComponent<AnimationComponent>(m_catchingGuardID).Play("A_GUARD_ATTACK", false);
            }

            // プレイヤー: やられたモーション
            if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
                // "A_PLAYER_DAMAGE" や "A_PLAYER_CAUGHT"
                m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_DAMAGE", false);
            }

            // 効果音 (バシッ！とか)
            EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_HIT");
        }
    }
    // --- フェーズ3: 余韻 ---
    else
    {
        // アニメーションが終わるくらいまで待つ (例: 2秒)
        if (state.sequenceTimer > 2.0f)
        {
            state.isGameOver = true;
            CheckSceneTransition(controllerID);
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

    // カメラ行列の準備（交差判定用）
    EntityID cameraID = FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    XMMATRIX viewProj = XMMatrixIdentity();
    if (cameraID != INVALID_ENTITY_ID) {
        auto& camera = m_coordinator->GetComponent<CameraComponent>(cameraID);
        XMMATRIX view = XMMatrixTranspose(XMLoadFloat4x4(&camera.viewMatrix));
        XMMATRIX proj = XMMatrixTranspose(XMLoadFloat4x4(&camera.projectionMatrix));
        viewProj = view * proj;
    }

    // ScanLineComponentを持つ全エンティティを更新
    // (本来はSignatureでフィルタリングされていますが、ここでは全探索またはComponentManager経由で取得)
    // ※GameControlSystemのSignatureにScanLineComponentを追加するのを忘れずに！
    // もしSignatureに追加していない場合は、以下のようにGetComponentで確認します

    // 最適化のため、EntityFactoryでTagをつけて検索するか、
    // 単純に全エンティティからComponent持ちを探すループを回します
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<ScanLineComponent>(entity)) continue;

        auto& scan = m_coordinator->GetComponent<ScanLineComponent>(entity);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

        // モードによる表示切替
        ui.isVisible = isScouting;
        if (!isScouting) continue;

        // --- 1. 移動 ---
        float prevY = trans.position.y;
        float move = scan.speed * deltaTime;

        if (scan.movingDown) {
            trans.position.y += move;
            if (trans.position.y >= scan.endY) {
                trans.position.y = scan.endY; scan.movingDown = false;
            }
        }
        else {
            trans.position.y -= move;
            if (trans.position.y <= scan.startY) {
                trans.position.y = scan.startY; scan.movingDown = true;
            }
        }
        float currY = trans.position.y;
        trans.position.x = SCREEN_WIDTH * 0.5f;

        // --- 2. 交差判定とエフェクト発生 ---
        // アイコンが表示されているエンティティ（ターゲット）に対して判定
        for (auto& pair : m_iconMap) {
            EntityID target = pair.first;
            EntityID icon = pair.second;

            // 表示されているアイコンのみ対象
            if (!m_coordinator->GetComponent<UIImageComponent>(icon).isVisible) continue;
            if (!m_coordinator->HasComponent<TransformComponent>(target)) continue;

            // ターゲットのスクリーンY座標を取得
            auto& targetTrans = m_coordinator->GetComponent<TransformComponent>(target);
            XMFLOAT3 sPos = GetScreenPosition(targetTrans.position, viewProj);

            // Y座標が移動範囲に含まれているか判定
            float minY = std::min(prevY, currY);
            float maxY = std::max(prevY, currY);

            if (sPos.z >= 0.0f && sPos.z <= 1.0f) { // 画面内かつ
                if (sPos.y >= minY && sPos.y <= maxY) {
                    // ターゲットの種類に応じて色を決定
                    XMFLOAT4 color = { 1, 1, 1, 1 };
                    if (m_coordinator->HasComponent<CollectableComponent>(target)) color = { 1, 1, 0, 1 }; // 黄
                    else if (m_coordinator->GetComponent<TagComponent>(target).tag == "taser") color = { 1, 0, 0, 1 }; // 赤（敵）

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

    // 1. グリッド等のエフェクト表示切替
    // (InitやGameSceneで m_topViewEffects にEntityIDを入れておく必要がありますが、
    //  ここでは簡易的に「Tagが"TopViewEffect"のもの」を探すか、生成時にリストに入れる処理が必要です)
    // 今回はGameSceneで生成したグリッドを制御するため、
    // ScanLineと同様に「コンポーネントを持たないけど表示制御したいUI」の扱いになります。
    // 手っ取り早く実装するため、GameSceneでグリッドに `ScanLineComponent` (speed=0) を持たせるのが一番簡単です。
    // そうすれば UpdateScanLine の中で勝手に表示切替されます。

    // ここでは「ソナー」の処理に集中します。

    if (isScouting)
    {
        m_sonarSpawnTimer += deltaTime;
        if (m_sonarSpawnTimer >= 1.0f) {
            m_sonarSpawnTimer = 0.0f;
            EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
            if (playerID != INVALID_ENTITY_ID && m_iconMap.count(playerID)) {
                auto& iconTrans = m_coordinator->GetComponent<TransformComponent>(m_iconMap[playerID]);
                // ソナーEntity生成
                EntityID sonar = m_coordinator->CreateEntity(
                    TransformComponent(
                        iconTrans.position,
                        { 0, 0, 0 },
                        { 0, 0, 1 }
                    ),
                    UIImageComponent(
                        "UI_SONAR",
                        4.0f,
                        true,
                        { 0.0f, 1.0f, 0.5f, 1.0f }
                    ),
                    SonarComponent(
                        1.5f,
                        0.0f,
                        500.0f
                    )
                );
            }
        }
    }

    // 2. 既存ソナーの更新と削除
    std::vector<EntityID> toDestroy;
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<SonarComponent>(entity)) continue;

        // ★追加: モードがトップビューでなければ即削除対象にする
        if (!isScouting)
        {
            toDestroy.push_back(entity);
            continue;
        }

        auto& sonar = m_coordinator->GetComponent<SonarComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);

        sonar.timer += deltaTime;
        float progress = sonar.timer / sonar.maxTime;

        if (progress >= 1.0f) {
            toDestroy.push_back(entity);
            continue;
        }

        float scale = sonar.startScale + (sonar.maxScale - sonar.startScale) * progress;
        trans.scale = { scale, scale, 1.0f };
        ui.color.w = 0.5f - (progress * progress);
    }

    // 削除実行
    for (auto id : toDestroy) m_coordinator->DestroyEntity(id);
}

void GameControlSystem::InitGameUI()
{
    // 1. タイム表示用エンティティの生成
    // 形式: MM:SS.d (例 01:23.4) -> 7文字 (分2桁, コロン, 秒2桁, ドット, 小数1桁)
    for (auto id : m_timerDigits) m_coordinator->DestroyEntity(id);
    for (auto id : m_itemHUDs) m_coordinator->DestroyEntity(id);
    m_timerDigits.clear();
    m_itemHUDs.clear();

    float startX = 50.0f;
    float startY = 50.0f;
    float w = 30.0f;
    float h = 50.0f;

    // 7桁分作成
    for (int i = 0; i < 7; ++i) {
        EntityID digit = m_coordinator->CreateEntity(
            TransformComponent(
                { startX + i * w, startY, 0.0f }, { 0,0,0 }, { w, h, 1.0f }
            ),
            UIImageComponent(
                "UI_FONT", 0.0f, true, { 1, 1, 1, 1 }
            )
        );
        m_timerDigits.push_back(digit);
    }
}

void GameControlSystem::UpdateGameUI(float deltaTime, ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // --- 1. タイム表示更新 (MM:SS.d) ---
    float time = state.elapsedTime;
    int minutes = (int)(time / 60.0f);
    int seconds = (int)(time) % 60;
    int deciseconds = (int)((time - (int)time) * 10.0f); // 小数第1位

    // 各桁の数値 (10:コロン, 11:ドット とする)
    int indices[7];
    indices[0] = (minutes / 10) % 10;
    indices[1] = minutes % 10;
    indices[2] = 10; // ':'
    indices[3] = (seconds / 10) % 10;
    indices[4] = seconds % 10;
    indices[5] = 11; // '.'
    indices[6] = deciseconds % 10;

    // UV計算 (5列3行)
    // 0 1 2 3 4
    // 5 6 7 8 9
    // - : .
    // ':': index=10 -> row=2, col=1
    // '.': index=11 -> row=2, col=2
    // '-': index=12 -> row=2, col=0 (必要なら)

    const float UV_UNIT_X = 1.0f / 5.0f;
    const float UV_UNIT_Y = 1.0f / 3.0f;

    for (int i = 0; i < 7; ++i) {
        if (i >= m_timerDigits.size()) break;

        auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_timerDigits[i]);
        int idx = indices[i];
        int row, col;

        if (idx <= 9) { // 数字
            row = idx / 5;
            col = idx % 5;
        }
        else if (idx == 10) { // ':'
            row = 2; col = 1;
        }
        else if (idx == 11) { // '.'
            row = 2; col = 2;
        }
        else { // '-' or Error
            row = 2; col = 0;
        }

        ui.uvPos = { col * UV_UNIT_X, row * UV_UNIT_Y };
        ui.uvScale = { UV_UNIT_X, UV_UNIT_Y };
    }

    // --- 2. アイテムHUD更新 ---
    if (m_coordinator->HasComponent<ItemTrackerComponent>(controllerID))
    {
        auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);
        size_t total = tracker.targetItemIDs.size();

        // 初回生成
        if (m_itemHUDs.size() < total)
        {
            float hudX = SCREEN_WIDTH - 60.0f;
            float hudY = 50.0f;
            float gapY = 60.0f;

            for (size_t i = m_itemHUDs.size(); i < total; ++i)
            {
                std::string iconPath = GetItemIconPath(tracker.targetItemIDs[i]);
                EntityID hud = m_coordinator->CreateEntity(
                    TransformComponent(
                        { hudX, hudY + i * gapY, 0.0f }, { 0,0,0 }, { 50, 50, 1.0f }
                    ),
                    UIImageComponent(
                        iconPath, 0.0f, true, { 1, 1, 1, 1 }
                    )
                );
                m_itemHUDs.push_back(hud);
            }
        }

        // 状態更新
        for (size_t i = 0; i < m_itemHUDs.size(); ++i)
        {
            if (i >= tracker.targetItemIDs.size()) break;

            auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_itemHUDs[i]);
            auto& trans = m_coordinator->GetComponent<TransformComponent>(m_itemHUDs[i]);
            std::string targetID = tracker.targetItemIDs[i];

            // 回収済み判定
            bool isCollected = true;
            bool foundInScene = false;

            for (auto const& entity : m_coordinator->GetActiveEntities()) {
                if (m_coordinator->HasComponent<CollectableComponent>(entity)) {
                    auto& col = m_coordinator->GetComponent<CollectableComponent>(entity);
                    if (col.itemID == targetID) {
                        foundInScene = true;
                        if (!col.isCollected) isCollected = false;
                        break;
                    }
                }
            }

            // 表示更新
            if (isCollected) {
                // 獲得済み: 明るく、サイズ固定
                ui.color = { 1.0f, 1.0f, 1.0f, 1.0f };
                trans.scale = { 50, 50, 1 };
            }
            else {
                // 未獲得: 暗く
                ui.color = { 0.3f, 0.3f, 0.3f, 0.5f };
                trans.scale = { 45, 45, 1 };

                // ★追加: 順序モードで「次」のターゲットならアニメーション
                if (tracker.useOrderedCollection)
                {
                    // currentTargetOrderは1始まりなので、インデックス(0始まり)と比較する際は -1 する
                    if ((int)i == (tracker.currentTargetOrder - 1))
                    {
                        // 黄色く強調
                        ui.color = { 1.0f, 1.0f, 0.5f, 1.0f };

                        // サイン波でふわふわ拡大縮小 (45.0f を基準に +変動)
                        float s = 50.0f + sinf(state.elapsedTime * 10.0f) * 5.0f;
                        trans.scale = { s, s, 1.0f };
                    }
                }
            }
        }
    }
}

DirectX::XMFLOAT3 GameControlSystem::GetScreenPosition(const DirectX::XMFLOAT3& worldPos, const DirectX::XMMATRIX& viewProj)
{
    XMVECTOR wPos = XMLoadFloat3(&worldPos);
    XMVECTOR clipPos = XMVector3TransformCoord(wPos, viewProj);
    XMFLOAT3 ndc;
    XMStoreFloat3(&ndc, clipPos);

    // NDC (-1.0 ~ 1.0) -> Screen (Pixel)
    // Y軸は反転(上端が0)させる
    float screenX = (ndc.x + 1.0f) * 0.5f * SCREEN_WIDTH;
    float screenY = (1.0f - ndc.y) * 0.5f * SCREEN_HEIGHT;

    return XMFLOAT3(screenX, screenY, ndc.z);
}

void GameControlSystem::SpawnSmallSonar(const XMFLOAT3& screenPos, XMFLOAT4 color)
{
    EntityID sonar = m_coordinator->CreateEntity(
        TransformComponent(
            { screenPos.x, screenPos.y, 0.0f },
            { 0,0,0 },
            { 0,0,1 }
        ),
        UIImageComponent(
            "UI_SONAR",
            4.0f,
            true,
            color
        ),
        SonarComponent(
            1.0f,
            0.0f,
            200.0f
        )
    );
}

// ---------------------------------------------------------
// 入場演出 (StartEntranceSequence / UpdateEntranceSequence)
// ---------------------------------------------------------
void GameControlSystem::StartEntranceSequence(EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceState = GameSequenceState::Entering;
    state.sequenceTimer = 0.0f;

    // 1. プレイヤーをドアの外（またはドア位置）に配置
    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    EntityID doorID = FindEntranceDoor(); // タグなどで入口ドアを探す関数

    if (playerID != INVALID_ENTITY_ID && doorID != INVALID_ENTITY_ID)
    {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& dTrans = m_coordinator->GetComponent<TransformComponent>(doorID);

        // --- 1. プレイヤー配置 ---
        float rad = dTrans.rotation.y;
        float startDist = 5.0f;
        // ドアの外側(-Z方向と仮定)に配置
        pTrans.position.x = dTrans.position.x - sin(rad) * startDist;
        pTrans.position.z = dTrans.position.z - cos(rad) * startDist;
        pTrans.rotation.y = dTrans.rotation.y;

        // --- 2. カメラ位置の計算 ---
        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
        {
            XMVECTOR doorPos = XMLoadFloat3(&dTrans.position);

            float sinY = sin(rad);
            float cosY = cos(rad);
            XMVECTOR doorDir = XMVectorSet(sinY, 0.0f, cosY, 0.0f);

            // ★修正A: 部屋の内側に配置するために「マイナス」にする
            // 距離も 2.5f 程度に調整
            XMVECTOR camPosVec = doorPos + (doorDir * 7.5f) + XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f);

            XMVECTOR lookAtVec = doorPos + XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

            XMFLOAT3 camPos, lookAt;
            XMStoreFloat3(&camPos, camPosVec);
            XMStoreFloat3(&lookAt, lookAtVec);

            printf("[DEBUG] DoorPos: %.2f, %.2f, %.2f\n", dTrans.position.x, dTrans.position.y, dTrans.position.z);
            printf("[DEBUG] CamPos : %.2f, %.2f, %.2f\n", camPos.x, camPos.y, camPos.z);

            // システムに目標をセット
            camSys->SetFixedCamera(camPos, lookAt);
        }

        // --- 2. ドアを開ける ---
        if (m_coordinator->HasComponent<AnimationComponent>(doorID)) {
            m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_OPEN", false);
        }

        // 通れるようにコリジョンをトリガー化
        if (m_coordinator->HasComponent<CollisionComponent>(doorID)) {
            m_coordinator->GetComponent<CollisionComponent>(doorID).type = COLLIDER_TRIGGER;
        }

        // ドアが開く音
        EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DOOR_OPEN");
    }
    auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);

    // 全回収したら
    if (tracker.collectedItems >= tracker.totalItems)
    {
        // 出口ドアを探して開ける
        EntityID exitDoor = FindExitDoor();
        if (exitDoor != INVALID_ENTITY_ID)
        {
            auto& door = m_coordinator->GetComponent<DoorComponent>(exitDoor);
            if (door.isLocked) // まだ開いてなければ
            {
                door.isLocked = false;
                door.state = DoorState::Open;

                m_coordinator->GetComponent<AnimationComponent>(exitDoor).Play("A_DOOR_OPEN", false);
                m_coordinator->GetComponent<CollisionComponent>(exitDoor).type = COLLIDER_TRIGGER;

                // ★ 全アイテム回収後のBGM切り替え -------------------
                // 1. 既存のBGMを止める
                for (auto const& entity : m_coordinator->GetActiveEntities())
                {
                    if (!m_coordinator->HasComponent<SoundComponent>(entity))
                        continue;

                    auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);

                    // アクション用BGM (assetID = "BGM_TEST2") を停止
                    if (sound.assetID == "BGM_ACTION")
                    {
                        sound.RequestStop();
                    }
                }
                // 2. クリア待機用BGM（BGM_TEST3）を再生開始
                ECS::EntityID clearBgm = ECS::EntityFactory::CreateLoopSoundEntity(
                    m_coordinator,
                    "BGM_TEST3",  // ★ Sound.csv に登録されているID
                    0.5f          // 音量は好みで
                );

                // 必要ならタグを付けておく（あとで止めたい時用）
                if (m_coordinator->HasComponent<TagComponent>(clearBgm))
                {
                    m_coordinator->GetComponent<TagComponent>(clearBgm).tag = "BGM_CLEAR";
                }
                // 音やメッセージ「脱出せよ！」などを出す
            }
        }
    }

}

void GameControlSystem::UpdateEntranceSequence(float deltaTime, EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;

    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    // --- 1. 入場移動 (0.0s ~ 2.5s) ---
    // ★時間を 2.0f -> 2.5f に延ばして、より奥へ進ませる
    if (state.sequenceTimer < 2.5f)
    {
        // アニメーション再生 (歩き)
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
            m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN");
        }

        // プレイヤーが向いている方向(回転)に進む
        float speed = 4.0f * deltaTime;
        float rad = pTrans.rotation.y;

        pTrans.position.x += sin(rad) * speed;
        pTrans.position.z += cos(rad) * speed;
    }
    // --- 2. ドア閉鎖 & 待機 (2.5s ~ 3.5s) ---
    // ★移動が終わったら、ドアを閉めて少し待つ
    else if (state.sequenceTimer < 4.5f)
    {
        // 待機モーション
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
            m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_IDLE");
        }

        // ★このフェーズに入った瞬間(1回だけ)ドアを閉める
        // (前フレームまでは 2.5f 未満だった場合)
        if (state.sequenceTimer - deltaTime < 2.5f)
        {
            EntityID doorID = FindEntranceDoor();
            if (doorID != INVALID_ENTITY_ID) {
                if (m_coordinator->HasComponent<AnimationComponent>(doorID)) {
                    m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_CLOSE", false);
                }
                // コリジョンを壁に戻す (閉じ込める)
                if (m_coordinator->HasComponent<CollisionComponent>(doorID)) {
                    m_coordinator->GetComponent<CollisionComponent>(doorID).type = COLLIDER_STATIC;
                }
                // 閉まる音
                EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DOOR_CLOSE");
            }
        }
    }
    // --- 3. 演出終了 & 操作開始 (3.5s以降) ---
    else
    {
        // カメラを背後に戻す (前回の回答で追加したリセット処理)
        if (auto camSys = ECS::ECSInitializer::GetSystem<CameraControlSystem>())
        {
            camSys->ReleaseFixedCamera();
            camSys->ResetCameraAngle(pTrans.rotation.y, 0.6f);
        }

        // ★ここで初めて操作可能になる
        state.sequenceState = GameSequenceState::Playing;
    }
}

// ---------------------------------------------------------
// アイテムコンプ時のドア開放
// ---------------------------------------------------------
void GameControlSystem::CheckDoorUnlock(EntityID controllerID)
{
    // ItemTrackerが無い場合は安全に抜ける
    if (!m_coordinator->HasComponent<ItemTrackerComponent>(controllerID)) return;

    auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(controllerID);

    // 全回収したら
    if (tracker.collectedItems >= tracker.totalItems)
    {
        // 出口ドアを探して開ける
        EntityID exitDoor = FindExitDoor();
        if (exitDoor != INVALID_ENTITY_ID)
        {
            auto& door = m_coordinator->GetComponent<DoorComponent>(exitDoor);

            // まだ開いてなければ（= 1回だけ実行される）
            if (door.isLocked)
            {
                // ドア解錠・開く
                door.isLocked = false;
                door.state = DoorState::Open;

                if (m_coordinator->HasComponent<AnimationComponent>(exitDoor))
                {
                    m_coordinator->GetComponent<AnimationComponent>(exitDoor).Play("A_DOOR_OPEN", false);
                }
                if (m_coordinator->HasComponent<CollisionComponent>(exitDoor))
                {
                    m_coordinator->GetComponent<CollisionComponent>(exitDoor).type = COLLIDER_TRIGGER;
                }

                // いま鳴ってるBGMを止める（必要に応じてID追加してOK）
                for (auto const& entity : m_coordinator->GetActiveEntities())
                {
                    if (!m_coordinator->HasComponent<SoundComponent>(entity)) continue;

                    auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
                    const auto& id = sound.assetID;

                    if (id == "BGM_TEST" || id == "BGM_TEST2")
                    {
                        sound.RequestStop();
                    }
                }

                // 全回収BGMへ切り替え
                ECS::EntityFactory::CreateLoopSoundEntity(
                    m_coordinator,
                    "BGM_TEST3", // ← 全回収後に流したいBGMのID
                    0.5f
                );

                // 任意：全回収SEを鳴らしたいなら
                // ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_TEST2", 0.8f);
            }
        }
    }
}

// ---------------------------------------------------------
// 脱出演出 (ゴール接触時に呼ばれる)
// ---------------------------------------------------------
void GameControlSystem::UpdateExitSequence(float deltaTime, EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    EntityID exitDoorID = FindExitDoor();

    if (playerID != INVALID_ENTITY_ID && exitDoorID != INVALID_ENTITY_ID)
    {
        auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);
        auto& dTrans = m_coordinator->GetComponent<TransformComponent>(exitDoorID);

        // --- 1. 移動フェーズ (0.0s ~ 4.0s) ---
        if (state.sequenceTimer < 4.0f)
        {
            // 歩きモーション
            if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
                m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN");
            }

            // ★ムーンウォーク対策:
            // 「ドアへのベクトル」ではなく、「ドアの逆向き(退出方向)」へ回転し、その前方へ進む

            // 目標の向き: ドアの向きの反対 (部屋の外へ)
            float targetRot = dTrans.rotation.y + XM_PI;

            // 向きの補間
            float currentRot = pTrans.rotation.y;
            float diff = targetRot - currentRot;
            while (diff > XM_PI) diff -= XM_2PI;
            while (diff < -XM_PI) diff += XM_2PI;
            pTrans.rotation.y += diff * 5.0f * deltaTime;

            // 移動 (向いている方向へ)
            float walkSpeed = 2.0f * deltaTime;
            float rad = pTrans.rotation.y;
            float moveX = sin(rad) * walkSpeed;
            float moveZ = cos(rad) * walkSpeed;

            pTrans.position.x += moveX;
            pTrans.position.z += moveZ;

            // 位置補正 (ドアの正面ラインに寄せる)
            // ドアとのX軸(横)ズレを簡易的に修正
            XMVECTOR doorPosV = XMLoadFloat3(&dTrans.position);
            XMVECTOR playerPosV = XMLoadFloat3(&pTrans.position);
            XMVECTOR toDoor = XMVectorSubtract(doorPosV, playerPosV);
            // ※厳密な計算は省略し、ここでは「進む」ことを優先しています
        }

        // --- 2. ドアを閉める (2.5秒経過後など、プレイヤーが出た後) ---
        // ★追加: ゴール時もドアを閉める処理
        if (state.sequenceTimer > 2.5f)
        {
            // ドアが開いていれば閉める
            bool isOpen = false;
            if (m_coordinator->HasComponent<CollisionComponent>(exitDoorID)) {
                if (m_coordinator->GetComponent<CollisionComponent>(exitDoorID).type == COLLIDER_TRIGGER) {
                    isOpen = true;
                }
            }
            if (isOpen)
            {
                if (m_coordinator->HasComponent<AnimationComponent>(exitDoorID)) {
                    m_coordinator->GetComponent<AnimationComponent>(exitDoorID).Play("A_DOOR_CLOSE", false);
                }
                if (m_coordinator->HasComponent<CollisionComponent>(exitDoorID)) {
                    m_coordinator->GetComponent<CollisionComponent>(exitDoorID).type = COLLIDER_STATIC;
                }
                EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_DOOR_CLOSE");
            }
        }
    }

    // --- 3. 終了判定 (時間を長く) ---
    if (state.sequenceTimer > 5.0f)
    {
        state.isGameClear = true;
        CheckSceneTransition(controllerID);
    }
}

EntityID GameControlSystem::FindEntranceDoor()
{
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (m_coordinator->HasComponent<DoorComponent>(entity)) {
            if (m_coordinator->GetComponent<DoorComponent>(entity).isEntrance) return entity;
        }
    }
    return INVALID_ENTITY_ID;
}

EntityID GameControlSystem::FindExitDoor()
{
    // 入力ドアを出口としても使うため、Entranceと同じものを探す
    return FindEntranceDoor();
}
// ---------------------------------------------------------
// MapGimmick / forced mode switch helpers
// ---------------------------------------------------------
bool GameControlSystem::IsAABBOverlap(ECS::EntityID a, ECS::EntityID b)
{
    if (a == ECS::INVALID_ENTITY_ID || b == ECS::INVALID_ENTITY_ID) return false;
    if (!m_coordinator) return false;

    if (!m_coordinator->HasComponent<TransformComponent>(a) || !m_coordinator->HasComponent<TransformComponent>(b)) return false;
    if (!m_coordinator->HasComponent<CollisionComponent>(a) || !m_coordinator->HasComponent<CollisionComponent>(b)) return false;

    const auto& ta = m_coordinator->GetComponent<TransformComponent>(a);
    const auto& tb = m_coordinator->GetComponent<TransformComponent>(b);
    const auto& ca = m_coordinator->GetComponent<CollisionComponent>(a);
    const auto& cb = m_coordinator->GetComponent<CollisionComponent>(b);

    auto makeMinMax = [](const TransformComponent& t, const CollisionComponent& c, DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax)
        {
            const float cx = t.position.x + c.offset.x;
            const float cy = t.position.y + c.offset.y;
            const float cz = t.position.z + c.offset.z;

            outMin = { cx - c.size.x, cy - c.size.y, cz - c.size.z };
            outMax = { cx + c.size.x, cy + c.size.y, cz + c.size.z };
        };

    DirectX::XMFLOAT3 amin, amax, bmin, bmax;
    makeMinMax(ta, ca, amin, amax);
    makeMinMax(tb, cb, bmin, bmax);

    const bool overlapX = (amin.x <= bmax.x) && (amax.x >= bmin.x);
    const bool overlapY = (amin.y <= bmax.y) && (amax.y >= bmin.y);
    const bool overlapZ = (amin.z <= bmax.z) && (amax.z >= bmin.z);

    return overlapX && overlapY && overlapZ;
}

void GameControlSystem::ApplyModeVisuals(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // Background switch
    if (state.topviewBgID != INVALID_ENTITY_ID && state.tpsBgID != INVALID_ENTITY_ID)
    {
        auto& normalUI = m_coordinator->GetComponent<UIImageComponent>(state.topviewBgID);
        auto& tpsUI = m_coordinator->GetComponent<UIImageComponent>(state.tpsBgID);

        if (state.currentMode == GameMode::SCOUTING_MODE)
        {
            normalUI.isVisible = true;
            tpsUI.isVisible = false;
        }
        else
        {
            normalUI.isVisible = false;
            tpsUI.isVisible = true;
        }
    }

    const bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);

    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<RenderComponent>(entity)) continue;

        auto& render = m_coordinator->GetComponent<RenderComponent>(entity);

        bool isTarget = false;
        bool keepVisibleInScouting = false; // TopViewでも表示を維持する対象（壁/床/扉など）
        MeshType restoreTypeAction = render.type;   // ACTION_MODE へ戻す時の型
        MeshType restoreTypeScouting = MESH_NONE;   // SCOUTING_MODE での表示型（既定は非表示）

        // Player
        if (m_coordinator->HasComponent<PlayerControlComponent>(entity))
        {
            isTarget = true;
            keepVisibleInScouting = false;
            restoreTypeAction = MESH_MODEL;
            restoreTypeScouting = MESH_NONE;
        }
        // Items
        else if (m_coordinator->HasComponent<CollectableComponent>(entity))
        {
            isTarget = true;
            keepVisibleInScouting = false;
            restoreTypeAction = MESH_MODEL;
            restoreTypeScouting = MESH_NONE;
        }
        else if (m_coordinator->HasComponent<TagComponent>(entity))
        {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;

            if (tag == "guard")
            {
                isTarget = true;
                keepVisibleInScouting = false;
                restoreTypeAction = MESH_MODEL;
                restoreTypeScouting = MESH_NONE;
            }
            else if (tag == "taser")
            {
                isTarget = true;
                keepVisibleInScouting = false;
#ifdef _DEBUG
                restoreTypeAction = MESH_BOX;
#else
                restoreTypeAction = MESH_NONE;
#endif
                restoreTypeScouting = MESH_NONE;
            }
            else if (tag == "map_gimmick")
            {
                isTarget = true;
                keepVisibleInScouting = false;
#ifdef _DEBUG
                restoreTypeAction = MESH_BOX;
#else
                restoreTypeAction = MESH_NONE;
#endif
                restoreTypeScouting = MESH_NONE;
            }
            else if (tag == "ground" || tag == "wall")
            {
                // ★重要:
                // EntityFactory は ground/wall を MESH_BOX で生成しているが、ModelComponent(M_CORRIDOR等)も付いている。
                // 夜っぽい見た目（テクスチャ/陰影）を維持したいので ACTION_MODE では MESH_MODEL で描画する。
                // TopView は見やすさ優先で BOX 表示にする（必要なら MODEL でもOK）。
                isTarget = true;
                keepVisibleInScouting = true;

                restoreTypeAction = MESH_MODEL;   // ゲーム画面（暗い/夜）を維持
                restoreTypeScouting = MESH_BOX;   // TopViewは分かりやすく
            }
            else if (tag == "door")
            {
                isTarget = true;
                keepVisibleInScouting = true;

                restoreTypeAction = MESH_MODEL;
                restoreTypeScouting = MESH_MODEL;
            }
        }

        if (!isTarget) continue;

        if (isScouting)
        {
            render.type = keepVisibleInScouting ? restoreTypeScouting : MESH_NONE;
        }
        else
        {
            render.type = restoreTypeAction;
        }
    }
}



void GameControlSystem::CheckMapGimmickTrigger(ECS::EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // Action中に踏んだらTopViewへ（TopView中は無視）
    if (state.currentMode != GameMode::ACTION_MODE) return;

    // 入場/退場/捕獲演出中は無視
    if (state.sequenceState != GameSequenceState::Playing) return;

    ECS::EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == ECS::INVALID_ENTITY_ID) return;

    for (auto const& e : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<TagComponent>(e)) continue;
        auto& tag = m_coordinator->GetComponent<TagComponent>(e).tag;
        if (tag != "map_gimmick") continue;

        if (!IsAABBOverlap(playerID, e)) continue;

        // Force switch to TopView
        state.currentMode = GameMode::SCOUTING_MODE;

        // One-shot SE (use an existing ID you already have)
        ECS::EntityFactory::CreateOneShotSoundEntity(
            m_coordinator,
            "SE_TOPVIEWSTART",
            0.8f
        );

        ApplyModeVisuals(controllerID);

        // Prevent re-trigger spam: mark used + shrink collider
        tag = "map_gimmick_used";
        if (m_coordinator->HasComponent<CollisionComponent>(e))
        {
            m_coordinator->GetComponent<CollisionComponent>(e).size = { 0.0f, 0.0f, 0.0f };
        }
        break;
    }
}
