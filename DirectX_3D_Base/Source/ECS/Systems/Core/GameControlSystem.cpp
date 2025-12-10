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

#include "ECS/Components/Core/GameStateComponent.h"
#include "ECS/Components/Gameplay/CollectableComponent.h"   
#include "ECS/Components/Gameplay/ItemTrackerComponent.h"  
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

    if (!m_uiInitialized)
    {
        InitGameUI();
        m_uiInitialized = true;
    }

    // 2. 各ロジックを順次実行
    // 処理順序が重要です: 入力 -> 状態更新 -> 判定 -> 遷移 -> UI表示

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
                }
            }
        }
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
    if (IsKeyTrigger('C'))
    {
        state.isGameClear = true;
    }
    if (IsKeyTrigger('O'))
    {
        state.isGameOver = true;
    }
#endif // _DEBUG

}

// ---------------------------------------------------------
// B. 入力と視点切替 (旧 StateSwitchSystem)
// ---------------------------------------------------------
void GameControlSystem::HandleInputAndStateSwitch(ECS::EntityID controllerID)
{
    // Tabキー または Yボタン
    bool toggle = IsKeyTrigger(VK_SPACE) || IsButtonTriggered(BUTTON_A);
    if (!toggle) return;

    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);

    // モード反転
    state.currentMode = (state.currentMode == GameMode::ACTION_MODE)
        ? GameMode::SCOUTING_MODE
        : GameMode::ACTION_MODE;

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

    // 全エンティティ走査 (アイテム・敵)
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
        if (m_coordinator->HasComponent<TagComponent>(entity)) {
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "taser") isGuard = true;
        }

        if (isGuard) {
            if (showIcons) UpdateIcon(entity, "ICO_TASER", { 1, 1, 1, 1 });
            else if (m_iconMap.count(entity)) m_coordinator->GetComponent<UIImageComponent>(m_iconMap[entity]).isVisible = false;
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
                    else if (m_coordinator->GetComponent<TagComponent>(target).tag == "taser") color = {1, 0, 0, 1}; // 赤（敵）

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

        // ドアの回転(Y軸)を取得
        float rad = dTrans.rotation.y;

        // ★重要: ドアの「後ろ（外側）」へのベクトル
        // モデルの向きによりますが、通常ドアの正面(Z+)が部屋の内側なら、
        // 「外」は -Z 方向（つまり前方ベクトルの逆）になります。
        // もし逆なら + を - に調整してください。

        // ドアの外 5.0m の位置
        float startDist = 5.0f;
        float startX = dTrans.position.x - sin(rad) * startDist;
        float startZ = dTrans.position.z - cos(rad) * startDist;

        // プレイヤー配置
        pTrans.position.x = startX;
        pTrans.position.z = startZ;

        // プレイヤーの向きをドアと同じにする（部屋の方を向く）
        pTrans.rotation.y = dTrans.rotation.y;

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
}

void GameControlSystem::UpdateEntranceSequence(float deltaTime, EntityID controllerID)
{
    auto& state = m_coordinator->GetComponent<GameStateComponent>(controllerID);
    state.sequenceTimer += deltaTime;

    EntityID playerID = FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == INVALID_ENTITY_ID) return;

    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    // アニメーション再生 (歩き)
    if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
        m_coordinator->GetComponent<AnimationComponent>(playerID).Play("A_PLAYER_RUN");
    }

    // --- 0.0s ~ 2.0s: 直進して部屋に入る ---
    if (state.sequenceTimer < 2.0f)
    {
        // プレイヤーが向いている方向(回転)に進む
        float speed = 4.0f * deltaTime; // 少し速めに
        float rad = pTrans.rotation.y;

        pTrans.position.x += sin(rad) * speed;
        pTrans.position.z += cos(rad) * speed;
    }
    // --- 2.0s: 演出終了 ---
    else
    {
        // 待機モーションに戻す
        if (m_coordinator->HasComponent<AnimationComponent>(playerID)) {
            m_coordinator->GetComponent<AnimationComponent>(playerID).PlayBlend("A_PLAYER_IDLE", 0.5f);
        }

        // ドアを閉める
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

        // ゲーム状態をプレイ中に変更
        state.sequenceState = GameSequenceState::Playing;
    }
}

// ---------------------------------------------------------
// アイテムコンプ時のドア開放
// ---------------------------------------------------------
void GameControlSystem::CheckDoorUnlock(EntityID controllerID)
{
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

                // 音やメッセージ「脱出せよ！」などを出す
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
    auto& pTrans = m_coordinator->GetComponent<TransformComponent>(playerID);

    // 光の方へ歩いていく
    float speed = 3.0f * deltaTime;
    float rad = pTrans.rotation.y;
    pTrans.position.x += sin(rad) * speed;
    pTrans.position.z += cos(rad) * speed;

    EntityID doorID = FindEntranceDoor();
    if (doorID != INVALID_ENTITY_ID) {
        if (m_coordinator->HasComponent<AnimationComponent>(doorID)) {
            m_coordinator->GetComponent<AnimationComponent>(doorID).Play("A_DOOR_CLOSE", false);
        }
    }

    // フェードアウトなどをかけて、一定時間後にリザルトへ
    if (state.sequenceTimer > 2.0f)
    {
        state.isGameClear = true;
        CheckSceneTransition(controllerID); // リザルトへ遷移
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