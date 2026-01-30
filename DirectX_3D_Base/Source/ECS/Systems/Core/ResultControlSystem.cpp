#include "ECS/Systems/Core/ResultControlSystem.h"
#include "ECS/ECS.h"
#include "ECS/EntityFactory.h"
#include "Scene/ResultScene.h"

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/Core/TagComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>

#include <cmath>
#include <cstdlib>
#include <string>

using namespace ECS;

static bool StartsWith(const std::string& s, const char* prefix)
{
    return s.rfind(prefix, 0) == 0;
}

void ResultControlSystem::Update(float deltaTime)
{
    if (!m_coordinator) return;

    m_timer += deltaTime;

    // ---------------------------------------------------------
// Resultボタンにカーソルが重なったとき
// ---------------------------------------------------------
    for (auto entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
        if (!m_coordinator->HasComponent<UIButtonComponent>(entity)) continue;
        if (!m_coordinator->HasComponent<TransformComponent>(entity)) continue;

        const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
        auto& button = m_coordinator->GetComponent<UIButtonComponent>(entity);
        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

        // --- 修正箇所：BTN_ で始まるタグ、または _Frame で終わるタグを対象にする ---
        bool isTargetButton = StartsWith(tag, "BTN_");

        if (isTargetButton)
        {
            // --- ★修正：土台（_Frame付き）はSE判定から除外する ---
            // tagの末尾が "_Frame" で終わっているかチェック
            bool isFrame = (tag.size() >= 6 && tag.compare(tag.size() - 6, 6, "_Frame") == 0);

            float targetScaleMultiplier = (button.state == ButtonState::Hover) ? 1.1f : 1.0f;

            // 「Frameではない」かつ「Hoverになった瞬間」だけ鳴らす
            if (!isFrame && button.state == ButtonState::Hover && trans.scale.x <= button.originalScale.x + 0.01f)
            {
                ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_CURSOR", 0.4f);
            }

            // スケール変更処理は土台・テキスト両方に適用（連動して動かすため）
            DirectX::XMFLOAT3 targetScale = {
                button.originalScale.x * targetScaleMultiplier,
                button.originalScale.y * targetScaleMultiplier,
                1.0f
            };

            trans.scale.x += (targetScale.x - trans.scale.x) * 0.2f;
            trans.scale.y += (targetScale.y - trans.scale.y) * 0.2f;
        }
    }




    {
        // アニメーション設定
        const float BASE_W = 40.0f; // 元の幅
        const float BASE_H = 60.0f; // 元の高さ



        const float START_TIME = 0.8f; // テキスト同じ「0.8秒後」に開始
        const float SPEED = 6.0f; // テキストと同じ速さ
        const float AMOUNT = 0.05f;

        // 計算用の変数
        float currentScale = 1.0f;

        // 時間が 0.8秒 を過ぎていたら波打つ
        if (m_timer >= START_TIME)
        {
            float t = m_timer - START_TIME;
            // sin(時間 * 6.0) で波を作る
            currentScale = 1.0f + AMOUNT * std::sin(t * SPEED);
        }

        // 全ての数字に適用
        for (auto const& entity : m_coordinator->GetActiveEntities())
        {
            if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

            // "AnimNumber" というタグが付いているものを探して適用
            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
            if (tag == "AnimNumber")
            {
                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
                trans.scale = { BASE_W * currentScale, BASE_H * currentScale, 1.0f };
            }
        }
    }


    if (ResultScene::isClear )
    {
        float targetTime = ResultScene::s_resultData.clearTime;

        // --- 【新ロジック】表示時間を目標時間まで進める ---
        if (m_displayTime < targetTime)
        {
            // 1.5秒で目標に到達するスピードで加算
            float addSpeed = targetTime / 1.5f;
            m_displayTime += addSpeed * deltaTime;

            // 超えすぎ防止
            if (m_displayTime >= targetTime) m_displayTime = targetTime;

            // --- SE再生：0.1秒（表示上の値）進むごとに鳴らす ---
            // 表示時間が進んでいる間、一定周期でタイマーを回す
            m_seTimer += deltaTime;
            if (m_seTimer >= 0.06f)
            {
                ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator, "SE_CURSOR", 0.3f);
                m_seTimer = 0.0f;
            }
        }
       

        // --- 数字のUV更新 (m_displayTime を使う) ---
        int tInt = static_cast<int>(m_displayTime * 10.0f);
        int digits[7] = {
            (tInt / 600) / 10, (tInt / 600) % 10, 11,
            ((tInt / 10) % 60) / 10, ((tInt / 10) % 60) % 10, 12,
            tInt % 10
        };

        // --- 数字エンティティのUV更新 (独立したループで実行) ---
        for (auto const& entity : m_coordinator->GetActiveEntities())
        {
            if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

            const std::string& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;

            // "TimeDigit_0" ～ "TimeDigit_6" を探す
            if (tag.find("TimeDigit_") == 0)
            {
                // 文字列の10文字目以降（数字部分）を数値に変換
                int index = std::stoi(tag.substr(10));
                if (index < 0 || index >= 7) continue;

                auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);
                int val = digits[index];

                // UI_FONT のレイアウト(0-9が2行に分かれている)に合わせたUV計算
                int r = (val <= 9) ? val / 5 : 2;
                int c = (val <= 9) ? val % 5 : (val - 10);

                ui.uvPos = { c * 0.2f, r * 0.333f };
                ui.uvScale = { 0.2f, 0.333f };
            }
        }
    }

    // ---------------------------------------------------------
    // 1. 星のアニメーション (0.5秒間隔で 1つずつポップ)
    //    Tag: "AnimStar0/1/2"
    // ---------------------------------------------------------
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

        const auto& tagComp = m_coordinator->GetComponent<TagComponent>(entity);
        const std::string& tag = tagComp.tag;

        if (!StartsWith(tag, "AnimStar")) continue;

        // "AnimStar0" の末尾数字を行番号として使う（無ければ0扱い）
        int row = 0;
        if (tag.size() > 7)
        {
            row = std::atoi(tag.c_str() + 7); // 7 = strlen("AnimStar")
            if (row < 0) row = 0;
            if (row > 2) row = 2;
        }

        float appearTime = 0.5f + row * 0.5f;
        if (m_timer < appearTime) continue;

        auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

        // 初回だけ「最終サイズ」を保存し、0開始に落とす
        auto it = m_starTargetScale.find(entity);
        if (it == m_starTargetScale.end())
        {
            m_starTargetScale[entity] = trans.scale; // ResultSceneで入れた最終サイズ
            it = m_starTargetScale.find(entity);

            trans.scale = { 0.0f, 0.0f, it->second.z };
        }

        // 出現からの経過時間でスケールを 0 -> 1.2 -> 1.0 に変化
        float t = (m_timer - appearTime) * 3.0f;
        float pop = 0.0f;

        if (t < 1.0f)            pop = t;
        else if (t < 1.5f)       pop = 1.0f + (1.5f - t) * 0.4f;
        else                     pop = 1.0f;

        const auto& target = it->second;
        trans.scale = { target.x * pop, target.y * pop, target.z };
    }

    // ---------------------------------------------------------
    // 2. �X�^���v�̃A�j���[�V���� (�S���o����� 2.0�b������Ńh���I)
    //    Tag: "AnimStamp"
    // ---------------------------------------------------------
    for (auto const& entity : m_coordinator->GetActiveEntities())
    {
        if (m_coordinator->HasComponent<TagComponent>(entity) &&
            m_coordinator->GetComponent<TagComponent>(entity).tag == "AnimStamp")
        {
            float appearTime = 2.0f;
            if (m_timer < appearTime) continue;

            auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);
            auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);

            float t = (m_timer - appearTime) * 2.0f;

            // 変数定義
            float scale = 1.0f;
            float alpha = 1.0f;

            // ★修正: ラジアンへの変換係数 (3.14... / 180)
            const float TO_RAD = 3.141592f / 180.0f;


            if (t < 1.0f)
            {
                scale = 5.0f - 4.0f * t; // 5 -> 1
                alpha = t;               // 0 -> 1

            }
            else
            {
                scale = 1.0f;
                alpha = 1.0f;

            }


            trans.scale = { 140.0f * scale, 140.0f * scale, 1.0f };
            trans.rotation.z = -30.0f * TO_RAD;
            ui.color.w = alpha;

            float triggerTiming = 0.8f;

            if (t >= triggerTiming && !m_playedStampEffect)
            {
                m_playedStampEffect = true;

                ECS::EntityFactory::CreateOneShotSoundEntity(
                    m_coordinator, // 第1引数はスマートポインタ(m_coordinator)でOK
                    "SE_STAMP",    // AssetManagerに登録してあるスタンプ音のID
                    0.8f           // ボリューム
                );


                std::cout << "Stamp Effect Played!" << std::endl;
                float centerX = 1920.0f * 0.52f;
                float centerY = 1080.0f * 0.18f;

                // ★テスト用：スケールを「巨大」にする
                // これで画面全体が光ったり何かが横切れば、エフェクトは再生できています
                float Scale = 600.0f;

                m_coordinator->CreateEntity(
                    TransformComponent(
                        { centerX, centerY, 0.0f }, // 画面ど真ん中、Z=0
                        { 0.0f, 0.0f, 0.0f },
                        { Scale, Scale, Scale }
                    ),
                    EffectComponent(
                        "EFK_STAMP",
                        false,
                        true,
                        { 0,0,0 },
                        1.0f
                    )
                );
            }
        }
    }

    if (m_starEffectStep < 3)
    {
        m_starEffectTimer += deltaTime;

        // ここでエフェクトの間隔を自由に調整できます 
        // 星の画像に合わせるなら 0.5f ですが、
        // 少し遅らせたいなら 0.6f、早めたいなら 0.4f などに変更してください。
        const float EFFECT_INTERVAL = 5.0f;

        if (m_starEffectTimer >= EFFECT_INTERVAL)
        {
            m_starEffectTimer = 0.0f; // タイマーリセット

            // リザルトデータを取得
            const auto& data = ResultScene::GetResultData();

            int i = m_starEffectStep;
            bool hasStar = false;

            // 星判定
            if (i == 0) hasStar = !data.wasSpotted;
            else if (i == 1) hasStar = data.collectedAllOrdered;
            else if (i == 2) hasStar = data.clearedInTime;

            if (hasStar)
            {
                // 座標計算
                float baseY = SCREEN_HEIGHT * 0.30f;
                float gapY = 55.0f;
                float starX = SCREEN_WIDTH * 0.535f;
                float y = baseY + i * gapY;

                // エフェクト生成
                m_coordinator->CreateEntity(
                    TransformComponent({ starX, y, -5.0f }, { 0,0,0 }, { 1.0f, 1.0f, 1.0f }),
                    EffectComponent("EFK_TITLE_SHINE", false, true, { 0.0f, 0.0f, 0.0f }, 1.0f)
                );
            }
            m_starEffectStep++;
        }
    }

    // ---------------------------------------------------------
    // 3. ★を取った行の STAR_TEXT を波打たせる
    //    Tag: "AnimStarText"
    // ---------------------------------------------------------
    {
        const float waveStartTime = 0.8f;

        if (m_timer >= waveStartTime)
        {
            float t = m_timer - waveStartTime;
            float waveScale = 1.0f + 0.05f * std::sin(t * 6.0f);

            // --- ★ここを追加：種類ごとの設定テーブル ---
            struct TextConfig { float w, h, x, y; };
            TextConfig configs[3] = {
                { 320.0f, 60.0f }, // 0番目: ノーダメージ
                { 350.0f, 65.0f }, // 1番目: アイテム全取得
                { 300.0f, 55.0f }  // 2番目: タイムクリア
            };

            for (auto const& entity : m_coordinator->GetActiveEntities())
            {
                if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

                const std::string& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;

                // "AnimStarText_" で始まるタグを探す
                if (tag.find("AnimStarText_") == 0)
                {
                    int idx = std::stoi(tag.substr(13));
                    if (idx < 0 || idx >= 3) continue;

                    auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                    // 個別のベースサイズをSystem側でも定義（Sceneと合わせる）
                    

                   
                    trans.scale.x = trans.scale.x * waveScale;
                    trans.scale.y = trans.scale.y * waveScale;

                    
                }
            }
        }
    }


    // ---------------------------------------------------------
    // 4. GameOver 演出（車＋ガスが走り、ロゴを置いていく）
    // ---------------------------------------------------------
    {
        const float SPEED = 500.0f;

        const float LOGO_STOP_X = SCREEN_WIDTH * 0.475f;
        const float LOGO_FINAL_Y = SCREEN_HEIGHT * 0.2f;
        const float LOGO_MOVE_UP_SPD = 240.0f;
        const float LOGO_SLOW_IN = 120.0f;

        // 画面外判定（ガス基準）
        const float GAS_MARGIN = 1200.0f;
        const float GAS_OUT_LEFT = -GAS_MARGIN;
        const float GAS_OUT_RIGHT = SCREEN_WIDTH + GAS_MARGIN;

        // 車とガスの距離
        const float CAR_OFFSET = 0.0f;
        const float GAS_OFFSET = 750.0f;

        // ==================================================
        // ロゴ・車・ガス 更新
        // ==================================================
        for (auto entity : m_coordinator->GetActiveEntities())
        {
            if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

            const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;

            bool isCar = (tag == "RESULT_ANIM_CAR");
            bool isGas = (tag == "RESULT_ANIM_GAS");
            bool isLogo = (tag == "RESULT_ANIM_LOGO");

            if (!isCar && !isGas && !isLogo) continue;

            auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

            // ---------------------------------------------------------
            // RESULT_ANIM_CLOUD（常に左へ流れる）2枚で途切れさせない
            // ---------------------------------------------------------
            {
                const float CLOUD_SPEED = 20.0f;

                // 左端判定（雲がほぼ消えたら）
                const float CLOUD_OUT_LEFT = -SCREEN_WIDTH * 0.5f;

                // 右端スポーン
                const float CLOUD_SPAWN_X = SCREEN_WIDTH + SCREEN_WIDTH * 0.5f;

                for (auto entity : m_coordinator->GetActiveEntities())
                {
                    if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
                    if (!m_coordinator->HasComponent<TransformComponent>(entity)) continue;

                    const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
                    if (tag != "RESULT_ANIM_CLOUD") continue;

                    auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                    // --------------------
                    // 左へ流す（常に）
                    // --------------------
                    trans.position.x -= CLOUD_SPEED * deltaTime;

                    // --------------------
                    // 左端に来た雲だけ右へ
                    // --------------------
                    if (trans.position.x < CLOUD_OUT_LEFT)
                    {
                        trans.position.x = CLOUD_SPAWN_X;
                    }
                }
            }

            // -------- ロゴ --------
            if (isLogo)
            {
                if (!m_logoStoppingX)
                {
                    trans.position.x -= SPEED * deltaTime;
                    if (trans.position.x <= LOGO_STOP_X + LOGO_SLOW_IN)
                        m_logoStoppingX = true;
                }
                else if (!m_logoMovingUp)
                {
                    float dx = LOGO_STOP_X - trans.position.x;
                    trans.position.x += dx * 6.0f * deltaTime;

                    if (std::fabs(dx) < 1.0f)
                    {
                        trans.position.x = LOGO_STOP_X;
                        m_logoMovingUp = true;
                    }
                }
                else
                {
                    trans.position.y -= LOGO_MOVE_UP_SPD * deltaTime;
                    if (trans.position.y <= LOGO_FINAL_Y)
                    {
                        trans.position.y = LOGO_FINAL_Y;
                        m_logoFinished = true;
                    }
                }
            }
            // -------- 車・ガス --------
            else
            {
                float dir = m_moveRightToLeft ? -1.0f : 1.0f;
                trans.position.x += dir * SPEED * deltaTime;
            }
        }

        // ==================================================
        // ロゴ完成後：反転制御（ガス完全基準）
        // ==================================================
        if (!m_logoFinished) return;

        // --------------------------------------
        // ガスが画面内に入るまで反転禁止
        // --------------------------------------
        if (m_waitEnterScreen)
        {
            for (auto entity : m_coordinator->GetActiveEntities())
            {
                if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
                if (m_coordinator->GetComponent<TagComponent>(entity).tag != "RESULT_ANIM_GAS") continue;

                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                if (trans.position.x > 0.0f &&
                    trans.position.x < SCREEN_WIDTH)
                {
                    m_waitEnterScreen = false;
                }
            }
        }

        // --------------------------------------
        // 反転判定（ガスが完全に抜けたか）
        // --------------------------------------
        if (!m_waitEnterScreen)
        {
            bool needFlip = false;

            for (auto entity : m_coordinator->GetActiveEntities())
            {
                if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;
                if (m_coordinator->GetComponent<TagComponent>(entity).tag != "RESULT_ANIM_GAS") continue;

                auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                if ((m_moveRightToLeft && trans.position.x < GAS_OUT_LEFT) ||
                    (!m_moveRightToLeft && trans.position.x > GAS_OUT_RIGHT))
                {
                    needFlip = true;
                    break;
                }
            }

            // ----------------------------------
            // 実際の反転処理
            // ----------------------------------
            if (needFlip)
            {
                m_moveRightToLeft = !m_moveRightToLeft;
                m_waitEnterScreen = true;

                for (auto entity : m_coordinator->GetActiveEntities())
                {
                    if (!m_coordinator->HasComponent<TagComponent>(entity)) continue;

                    const auto& tag = m_coordinator->GetComponent<TagComponent>(entity).tag;
                    if (tag != "RESULT_ANIM_CAR" && tag != "RESULT_ANIM_GAS") continue;

                    auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

                    float offset = (tag == "RESULT_ANIM_GAS") ? GAS_OFFSET : CAR_OFFSET;

                    // スポーン位置を画面端からのオフセットで計算
                    float spawnX = m_moveRightToLeft
                        ? GAS_OUT_RIGHT + offset
                        : GAS_OUT_LEFT - offset;

                    trans.position.x = spawnX;

                    // scale.x は常に正
                    trans.scale.x = std::fabs(trans.scale.x);

                    // ================================
                    // UV座標で左右反転
                    // ================================
                    if (m_coordinator->HasComponent<UIImageComponent>(entity))
                    {
                        auto& ui = m_coordinator->GetComponent<UIImageComponent>(entity);

                        if (m_moveRightToLeft)
                        {
                            // 左向き（右→左移動中）
                            ui.uvPos.x = 0.0f;   // 左端
                            ui.uvScale.x = 1.0f; // 通常描画
                        }
                        else
                        {
                            // 右向き（左→右移動中）
                            ui.uvPos.x = 1.0f;    // 右端
                            ui.uvScale.x = -1.0f; // 反転
                        }
                    }
                }
            }
        }
    }

}
