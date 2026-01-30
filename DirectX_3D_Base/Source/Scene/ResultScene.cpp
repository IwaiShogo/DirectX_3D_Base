

#include "Scene/ResultScene.h"


#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include "Scene/StageUnlockProgress.h"

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/Core/TagComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIButtonComponent.h>
#include <ECS/Components/UI/UICursorComponent.h>
#include <ECS/Components/Rendering/ModelComponent.h>
#include <ECS/Components/Rendering/AnimationComponent.h>

#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/UI/UIRenderSystem.h>
#include <ECS/Systems/UI/CursorSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include <ECS/Systems/Rendering/EffectSystem.h>
#include <ECS/Systems/Core/ResultControlSystem.h>


#include <ECS/Components/Rendering/EffectComponent.h>

#include <DirectXMath.h>
#include <cctype>
#include <iostream>

using namespace DirectX;
using namespace ECS;
using namespace std;

static bool IsInputTitle() { return false; }

bool       ResultScene::isClear = false;
int        ResultScene::finalItenCount = 0;
ResultData ResultScene::s_resultData = {};
int        ResultScene::s_newlyUnlockedStageNo = -1;

// Stage progress / best-time update is handled when ResultData is set.
static bool g_progressHandled = false;

void ResultScene::SetResultData(const ResultData& data)
{
    s_resultData = data;
    // Reset per-result flag
    g_progressHandled = false;

    // Default: no newly unlocked stage
    s_newlyUnlockedStageNo = -1;

    if (s_resultData.isCleared)
    {
        // Unlock next stage (if any)
        s_newlyUnlockedStageNo = StageUnlockProgress::UnlockNextStageFromClearedStageID(s_resultData.stageID);

        // Update BEST TIME (fastest only)
        const int clearedStageNo = [](const std::string& sid) -> int
            {
                int v = 0;
                bool inDigits = false;
                for (char ch : sid)
                {
                    if (std::isdigit(static_cast<unsigned char>(ch)))
                    {
                        inDigits = true;
                        v = v * 10 + (ch - '0');
                    }
                    else if (inDigits)
                    {
                        break;
                    }
                }
                return (v >= 1 && v <= 6) ? v : -1;
            }(s_resultData.stageID);

        StageUnlockProgress::UpdateBestTimeIfFaster(clearedStageNo, s_resultData.clearTime);

        // Update STARS (起動中のみで初期化。リザルトで取った分だけ点灯)
        {
            const std::uint8_t addMask =
                (s_resultData.wasSpotted ? 0 : 0x1) |
                (s_resultData.collectedAllOrdered ? 0x2 : 0) |
                (s_resultData.clearedInTime ? 0x4 : 0);
            if (addMask != 0)
            {
                StageUnlockProgress::UpdateStageStarMaskOr(clearedStageNo, addMask);
            }
        }
    }

    g_progressHandled = true;
}


namespace
{
    std::string GetResultStageNameTexture(const std::string& stageID)
    {
        (void)stageID;
        return "UI_BEST_TIME";
    }
}


namespace
{
    // アイテムIDの受け取り、｛アイコンテクスチャ, 名前テクスチャ｝のペアを返す
    struct ItemTextureSet {
        const char* iconTex;
        const char* nameTex;
    };
}

ItemTextureSet GetItemTextures(const std::string& itemID)
{
    // アイテムIDに応じて画像を切り替える
    // ※アセットIDは実際のプロジェクトに合わせて調整する
    if (itemID == "ICO_TREASURE1") {
        return { "ICO_TREASURE1", "UI_RESULT_TREASURE1" }; // ダイア
    }
    else if (itemID == "ICO_TREASURE2") {
        return { "ICO_TREASURE2", "UI_RESULT_TREASURE1" }; // クリスタル
    }
    else if (itemID == "ICO_TREASURE3") {
        return { "ICO_TREASURE3", "UI_RESULT_TREASURE1" }; // 指輪
    }
    else if (itemID == "ICO_TREASURE4") {
        return { "ICO_TREASURE4", "UI_RESULT_TREASURE2" }; // 絵画1（ひまわり）
    }
    else if (itemID == "ICO_TREASURE5") {
        return { "ICO_TREASURE5", "UI_RESULT_TREASURE2" }; // 絵画2（女の人）
    }
    else if (itemID == "ICO_TREASURE6") {
        return { "ICO_TREASURE6", "UI_RESULT_TREASURE2" }; // 絵画3（睡蓮花）
    }
    else if (itemID == "ICO_TREASURE7") {
        return { "ICO_TREASURE7", "UI_RESULT_TREASURE4" }; // 陶器1（茶色）
    }
    else if (itemID == "ICO_TREASURE8") {
        return { "ICO_TREASURE8", "UI_RESULT_TREASURE4" }; // 陶器2（水色）
    }
    else if (itemID == "ICO_TREASURE9") {
        return { "ICO_TREASURE9", "UI_RESULT_TREASURE4" }; // 陶器3（紫色）
    }
    else if (itemID == "ICO_TREASURE10") {
        return { "ICO_TREASURE10", "UI_RESULT_TREASURE3" }; // 化石1（頭部）
    }
    else if (itemID == "ICO_TREASURE11") {
        return { "ICO_TREASURE11", "UI_RESULT_TREASURE3" }; // 化石2（アンモナイト）
    }
    else if (itemID == "ICO_TREASURE12") {
        return { "ICO_TREASURE12", "UI_RESULT_TREASURE3" }; // 化石3（足）
    }
    // デフォルト（未知のID用）
    return { "ICO_TREASURE1", "ICO_TREASURE1" };
}



void ResultScene::Init()
{

    // --- 1. ECS 初期化 & カメラ ---
    m_coordinator = std::make_shared<ECS::Coordinator>();
    ECS::ECSInitializer::InitECS(m_coordinator);

    PlayerControlSystem::ResetFadeState();//テーザーでゲームオーバーになったときフェード初期化

    // エフェクトシステムに対して「UI用カメラ設定」を適用::織田
    if (auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        // これを呼ばないと、前のシーンの2D設定が残ってしまい、
        // 奥行きやスケールが正しく反映されません。
        effectSystem->ClearOverrideCamera();
    }



    EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0,0,0 });

    if (auto effectSystem = ECSInitializer::GetSystem<EffectSystem>())
    {
        effectSystem->SetScreenSpaceCamera((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
    }

    ResultScene::isClear = s_resultData.isCleared;
    m_isClear = ResultScene::isClear;

    // Stage progress / best-time update (safety: if SetResultData wasn't called)
    if (!g_progressHandled)
    {
        ResultScene::SetResultData(s_resultData);
    }

    // すでに演出の中で再生されている可能性があるため、一度リセット
    if (m_bgmEntity == ECS::INVALID_ENTITY_ID) {
        m_bgmEntity = ECS::EntityFactory::CreateLoopSoundEntity(
            m_coordinator.get(),
            isClear ? "BGM_GAMECLEAR" : "BGM_GAMEOVER",
            0.5f
        );
    }

    // NOTE:
    //  isCleared==true  : GAME CLEAR 表示
    //  isCleared==false : GAME OVER 表示
    //  ここが逆だと「クリアしてもゲームオーバー側」に見える。
    if (isClear == true)
    {
        // 1) 背景
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 900 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAME_CLEAR", -5.0f, true, { 1,1,1,1 })
        );

        // 2) GAME CLEAR ロゴ
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.09f, 0 },
                { 0, 0, 0 },
                { 760.0f, 96.0f, 1.0f }
            ),
            UIImageComponent("UI_GAME_CLEAR", -2.0f, true, { 1,1,1,1 })
        );

       



        /*m_starEffectTimer = 0.0f;
        m_starEffectStep = 0;*/
        // 右ページ：★3つ
        {
            bool stars[3] = {
                !s_resultData.wasSpotted,
                s_resultData.collectedAllOrdered,
                s_resultData.clearedInTime
            };

            float limitSec = s_resultData.timeLimitStar;

            // 2. 秒数に応じて出す画像の名前を決める
            std::string timeConditionTex;
            if (limitSec >= 180.0f) {
                timeConditionTex = "STAR_TEXT3_3MINUTE"; // 180秒以上のステージ
            }
            else if (limitSec >= 120.0f) {
                timeConditionTex = "STAR_TEXT3_2MINUTE"; // 120秒以上のステージ
            }
            else {
                timeConditionTex = "STAR_TEXT3_1MINUTE"; // それ以外の短いステージ
            }

            const char* conditionTex[3] = {
                "STAR_TEXT1",
                "STAR_TEXT2",
                timeConditionTex.c_str()
            };

            float baseY = SCREEN_HEIGHT * 0.30f;
            float gapY = 55.0f;
            float starX = SCREEN_WIDTH * 0.535f;
            float captionX = SCREEN_WIDTH * 0.67f;

            // ★サイズ（上だけ通常、下2つだけ少し小さく）
            const float STAR_OFF_SIZE_TOP = 50.0f;
            const float STAR_OFF_SIZE_LOW = 34.0f; // 好きに調整
            const float STAR_ON_SIZE_TOP = 50.0f;
            const float STAR_ON_SIZE_LOW = 34.0f; // 好きに調整

            for (int i = 0; i < 3; ++i)
            {
                float y = baseY + i * gapY;
                float currentX = captionX;

                float baseW = 350.0f;
                float baseH = 60.0f;

                // ---------------------------------------------------------
                // ここで 0, 1, 2 番目ごとの「ポジション」と「サイズ」を個別設定！
                // ---------------------------------------------------------
                if (i == 0) { // 1行目：ノーダメージ
                    currentX = SCREEN_WIDTH * 0.67f;
                    y = SCREEN_HEIGHT * 0.30f;
                    baseW = 320.0f; baseH = 60.0f;
                }
                else if (i == 1) { // 2行目：アイテム全取得
                    currentX = SCREEN_WIDTH * 0.67f;
                    y = SCREEN_HEIGHT * 0.38f;
                    baseW = 330.0f; baseH = 65.0f; // 少し大きく
                }
                else if (i == 2) { // 3行目：タイムクリア
                    currentX = SCREEN_WIDTH * 0.66f; // 右へずらす
                    y = SCREEN_HEIGHT * 0.46f;
                    baseW = 340.0f; baseH = 55.0f; // 少し小さく
                }
                // 条件テキスト（そのまま）
                if (stars[i])
                {
                    m_coordinator->CreateEntity(
                        // ここで「個別設定した座標とサイズ」を流し込む
                        TransformComponent({ currentX , y, 0.0f }, { 0,0,0 }, { baseW, baseH, 1.0f }),
                        UIImageComponent(conditionTex[i], 1.0f, true, { 1,1,1,1 }),
                        TagComponent("AnimStarText_" + std::to_string(i)) // タグを分ける
                    );

                    float centerX = 1920.0f * 0.36f;
                    m_coordinator->CreateEntity(
                        TransformComponent(
                            { centerX, y, 0.0f }, // 画面ど真ん中、Z=0
                            { 0.0f, 0.0f, 0.0f },
                            { 5, 5, 0 }
                        ),
                        EffectComponent(
                            "EFK_EYESLIGHT",
                            true,
                            true,
                            { 0,0,0 },
                            1.0f
                        )
                    );
                }
                else
                {
                    m_coordinator->CreateEntity(
                        TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 320.0f, 60.0f, 1.0f }),
                        UIImageComponent(conditionTex[i], 1.0f, true, { 1,1,1,1 })
                    );
                }

                const float offSize = (i == 0) ? STAR_OFF_SIZE_TOP : STAR_OFF_SIZE_LOW;
                const float onSize = (i == 0) ? STAR_ON_SIZE_TOP : STAR_ON_SIZE_LOW;

                // ★ Off（枠）
                m_coordinator->CreateEntity(
                    TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { offSize, offSize, 1.0f }),
                    UIImageComponent("ICO_STAR_OFF", -2.0f, true, { 1,1,1,1 })
                );

                // ★ On（ポップアニメ用）
                if (stars[i])
                {
                    // 最終サイズ(onSize)をTransformに入れておく（ResultControlSystemが0→最終へポップさせる）
                    m_coordinator->CreateEntity(
                        TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
                        UIImageComponent("ICO_STAR_ON", -1.0f, true, { 1,1,1,1 }),
                        TagComponent(std::string("AnimStar") + std::to_string(i)) // AnimStar0/1/2
                    );
                }
            }

            // クリアタイム
            float timeY = SCREEN_HEIGHT * 0.38f;
            float timeX = SCREEN_WIDTH * 0.365f;
            CreateTimeDisplay(s_resultData.clearTime, { timeX, timeY });

            // 目標タイム
            {
                // 1. 秒を「分」に変換 (例: 180.0f -> 3)
                int limitMinutes = static_cast<int>(s_resultData.timeLimitStar / 60.0f);

                // 2. 座標の設定 (デザインに合わせて微調整してください)
                float limitTimeX = SCREEN_WIDTH * 0.56f;
                float limitTimeY = SCREEN_HEIGHT * 0.45f;

                // 3. 数字作成関数を呼び出す
                // 第3引数に「時間内クリアしたか(clearedInTime)」を渡すことで、
                // 条件達成時にアニメーション(AnimStarTextタグが付与)されます
                CreateNumberDisplay(limitMinutes, { limitTimeX, limitTimeY });
            }



            // タイムの下：拾ったアイテム
            {
                const auto& itemIDs = s_resultData.collectedItemIcons; // ここにはIDが入っているはず
                int count = static_cast<int>(itemIDs.size());
                // ★デバッグ用ログ出力：受け取っているID一覧を表示
                std::cout << "=== ResultScene Item IDs ===" << std::endl;
                for (int i = 0; i < count; ++i) {
                    std::cout << "Index " << i << ": " << itemIDs[i] << std::endl;
                }
                std::cout << "============================" << std::endl;

                if (count > 0)
                {
                    const float iconW = 80.0f;
                    const float nameW = 330.0f;
                    const float nameH = 80.0f;
                    const  float baseY2 = timeY + 80.0f;

                    struct IconPos { float x; float y; };
                    IconPos positions[] = {
                        {timeX - 100.0f, baseY2 + 0.5f},
                        {timeX - 100.0f, baseY2 + 120.5f},
                        {timeX + 300.0f, baseY2 + 40.5f}
                        /*{timeX + 405.0f, baseY2 + 50.0f},
                        {timeX + 323.0f, baseY2 + 50.0f},
                        {timeX + 240.0f, baseY2 + 50.0f}*/


                    };

                    int maxIcons = std::min(count, (int)(sizeof(positions) / sizeof(positions[0])));

                    for (int i = 0; i < maxIcons; ++i)
                    {

                        // ★ここでも確認できます
                        std::cout << "Processing Item: " << itemIDs[i] << std::endl;

                        // 1. テクスチャ名の決定
                        auto texSet = GetItemTextures(itemIDs[i]);

                        // ★画像が見つからない場合の確認
                        if (std::string(texSet.iconTex) == "ICO_UNKNOWN") {
                            std::cout << "  -> WARNING: Unknown ID! Check GetItemTextures." << std::endl;
                        }
                       

                        float x = positions[i].x;
                        float y = positions[i].y;

                        m_coordinator->CreateEntity(
                            TransformComponent({ x,y,0 }, { 0,0,0 }, { iconW, iconW, 1 }),
                            UIImageComponent(texSet.iconTex, 1.0f, true, { 1,1,1,1 })
                        );

                        float nameX = x + 80.0f;
                        float nameY = y;

                        m_coordinator->CreateEntity(
                            TransformComponent(
                                { nameX, nameY, 0 },
                                { 0,0,0 },
                                { nameW * 0.4f, nameH * 0.4f, 1.0f }
                            ),
                            UIImageComponent(texSet.nameTex, 1.0f, true, { 1,1,1,1 })
                        );
                    }
                }
            }

           


            // スタンプ
            {
                const float STAMP_LEFT_OFFSET = 20.0f;  // 左へ(+)
                const float STAMP_Y_OFFSET = 28.0f;  // 上へ(+) / 下へはマイナス
                const float STAMP_ROT_DEG = 90.0f;

                m_coordinator->CreateEntity(
                    TransformComponent(
                        {
                            SCREEN_WIDTH * 0.80f - STAMP_LEFT_OFFSET,
                            SCREEN_HEIGHT * 0.25f + STAMP_Y_OFFSET,
                            0.0f
                        },
                        // 回転がラジアン実装の前提（違ったら下の注記）
                        { 0.0f, 0.0f, DirectX::XMConvertToRadians(STAMP_ROT_DEG) },
                        { 140.0f, 140.0f, 90.0f }
                    ),
                    UIImageComponent("ICO_STAMP1", 3.0f, true, { 1,1,1,0 }),
                    TagComponent("AnimStamp")
                );
            }


            // ベストタイムプレート
            {
                float plateW = 320.0f;
                float plateH = 80.0f;
                float plateX = SCREEN_WIDTH * 0.35f;
                float plateY = SCREEN_HEIGHT * 0.28f;

                const std::string texID = GetResultStageNameTexture(s_resultData.stageID);

                m_coordinator->CreateEntity(
                    TransformComponent({ plateX, plateY, 0.0f }, { 0,0,0 }, { plateW, plateH, 1.0f }),
                    UIImageComponent(texID.c_str(), 1.5f, true, { 1,1,1,1 })
                );
            }
        }
        
    }
    else
    {
        // ========================================================
        // GAME OVER 画面のレイアウト
        // ========================================================

        // --------------------------------------------------------
        // GAME OVER 演出：初期位置定義
        // --------------------------------------------------------
        const float startX = SCREEN_WIDTH + 300.0f;   // 画面右外
        const float startY = SCREEN_HEIGHT * 0.8f;    // 車・ガスのY位置
        const float CloudX = SCREEN_WIDTH * 0.7f;  // 雲の初期位置

        // 背景.空
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAMEOVER_SKY", 0.0f, true, { 1,1,1,1 })
        );

        // 背景.太陽
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAMEOVER_SUN", 0.0f, true, { 1,1,1,1 })
        );

        // 背景.海
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAMEOVER_OCEAN", 0.0f, true, { 1,1,1,1 })
        );

        // 背景.雲
        m_coordinator->CreateEntity(
            TransformComponent(
                { CloudX, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAMEOVER_CLOUD", 0.0f, true, { 1,1,1,1 }),
            TagComponent("RESULT_ANIM_CLOUD")
        );

        // 背景.雲2
        m_coordinator->CreateEntity(
            TransformComponent(
                { CloudX+1280, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAMEOVER_CLOUD2", 0.0f, true, { 1,1,1,1 }),
            TagComponent("RESULT_ANIM_CLOUD")
        );

        // 車
        m_coordinator->CreateEntity(
            TransformComponent({ startX, startY, 0.0f }, { 0,0,0 }, { 900,532,1 }),
            UIImageComponent("GAMEOVER_CHARACTER", 1.0f, true, { 1,1,1,1 }),
            TagComponent("RESULT_ANIM_CAR")
        );

        // ガス
        m_coordinator->CreateEntity(
            TransformComponent({ startX + 750.0f, startY, 0.0f }, { 0,0,0 }, { 1080,555,1 }),
            UIImageComponent("GAMEOVER_GAS", 1.0f, true, { 1,1,1,1 }),
            TagComponent("RESULT_ANIM_GAS")
        );

        // GAME OVER ロゴ（真ん中で置いていく）
        m_coordinator->CreateEntity(
            TransformComponent(
                { startX + 800.0f,startY+25 , 0.0f },
                { 0,0,0 },
                { 460,72,1 }
            ),
            UIImageComponent("UI_GAME_OVER", 1.2f, true, { 1,1,1,1 }),
            TagComponent("RESULT_ANIM_LOGO")
        );
        


        // ステージ名プレート（ゲームオーバー時も表示）
        //{
        //    float plateW = 320.0f;
        //    float plateH = 80.0f;
        //    float plateX = SCREEN_WIDTH * 0.30f;
        //    float plateY = SCREEN_HEIGHT * 0.23f;

        //    m_coordinator->CreateEntity(
        //        TransformComponent(
        //            { plateX, plateY, 0.0f },
        //            { 0.0f, 0.0f, 0.0f },
        //            { plateW, plateH, 1.0f }
        //        ),
        //        UIImageComponent(
        //            "BTN_BACK_STAGE_SELECT", // ← 後でステージ名画像に差し替え
        //            1.5f,
        //            true,
        //            { 1.0f, 1.0f, 1.0f, 1.0f }
        //        )
        //    );
        //}

        //  1/20 つかわない
        //// D) ゲームオーバー時：お宝一覧（順番どおり＋未取得灰色）
        //{
        //    const auto& icons = s_resultData.orderedItemIcons;
        //    const auto& flags = s_resultData.orderedItemCollected;
        //    size_t count = std::min(icons.size(), flags.size());

        //    if (count > 0)
        //    {
        //        const float iconSize = 80.0f;
        //        const float margin = 20.0f;

        //        // 画面下部ボタンの少し上、左から右へ並べるイメージ
        //        float baseY = SCREEN_HEIGHT * 0.25f;
        //        float baseX = SCREEN_WIDTH * 0.05f;

        //        for (size_t i = 0; i < count; ++i)
        //        {
        //            float x = baseX + (iconSize + margin) * static_cast<float>(i);
        //            float y = baseY;

        //            bool collected = flags[i];
        //            DirectX::XMFLOAT4 color = collected ? DirectX::XMFLOAT4{ 1,1,1,1 } : DirectX::XMFLOAT4{ 0.3f,0.3f,0.3f,0.7f };

        //            m_coordinator->CreateEntity(
        //                TransformComponent({ x, y, 0.0f }, { 0,0,0 }, { iconSize, iconSize, 1.0f }),
        //                UIImageComponent(icons[i].c_str(), 1.0f, true, color)
        //            );
        //        }
        //    }
        //}

        // BGM再生
        ECS::EntityID m_gameoverBGM = ECS::EntityFactory::CreateLoopSoundEntity(
            m_coordinator.get(),
            "BGM_GAMEOVER",
            0.8f
        );
    }

    // 下部ボタン
    CreateButtons();

    // カーソル
    m_coordinator->CreateEntity(
        TransformComponent({ 0,0,0 }, { 0,0,0 }, { 64,64,1 }),
        UIImageComponent("ICO_CURSOR", 5.0f),
        UICursorComponent()
    );

    std::cout << "ResultScene::Init() - completed." << std::endl;

    
}

void ResultScene::Uninit()
{
    // BGMエンティティの停止リクエスト
    if (m_bgmEntity != ECS::INVALID_ENTITY_ID)
    {
        if (m_coordinator->HasComponent<SoundComponent>(m_bgmEntity)) {
            auto& sound = m_coordinator->GetComponent<SoundComponent>(m_bgmEntity);
            sound.RequestStop(); //

            // ★重要：ここで物理的に音を止める命令を即座に実行させる
            auto audioSystem = ECS::ECSInitializer::GetSystem<AudioSystem>();
            if (audioSystem) {
                audioSystem->Update(0.0f); // 強制的に1フレーム更新してStopを反映させる
            }
        }
        m_coordinator->DestroyEntity(m_bgmEntity);
        m_bgmEntity = ECS::INVALID_ENTITY_ID;
    }

    // エフェクト等の停止
    if (auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>()) {
        effectSystem->Uninit();
    }

    // 最後にECS全体を落とす
    ECS::ECSInitializer::UninitECS();
}

void ResultScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);



    // 織田
    



}

void ResultScene::Draw()
{
    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(true);
    }

    if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
    {
        system->DrawSetup();
        system->DrawEntities();
    }

    if (auto system = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        system->Render();
    }

    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(false);
    }
}

void ResultScene::CreateTimeDisplay(float time, DirectX::XMFLOAT2 pos)
{
    int digits[7] = { 10,10,11,10,10,12,10 }; // --:--.-
    if (time > 0.05f)
    {
        int tInt = static_cast<int>(time * 10.0f);
        int min = (tInt / 600) % 100;
        int sec = (tInt / 10) % 60;
        int dsec = tInt % 10;

        digits[0] = min / 10;
        digits[1] = min % 10;
        digits[2] = 11; // ':'
        digits[3] = sec / 10;
        digits[4] = sec % 10;
        digits[5] = 12; // '.'
        digits[6] = dsec;
    }


    float w = 40.0f;
    float h = 60.0f;
    float startX = pos.x - (7 * w) / 2.0f;

    for (int i = 0; i < 7; ++i)
    {
        EntityID d = m_coordinator->CreateEntity(
            TransformComponent({ startX + i * w, pos.y, 0.0f }, { 0,0,0 }, { w,h,1 }),
            UIImageComponent("UI_FONT", 1.0f, true, { 1,1,1,1 }),
            TagComponent("TimeDigit_" + std::to_string(i))
        );

        int idx = digits[i];
        int r = (idx <= 9) ? idx / 5 : 2;
        int c = (idx <= 9) ? idx % 5 : (idx - 10);

        auto& ui = m_coordinator->GetComponent<UIImageComponent>(d);
        ui.uvPos = { c * 0.2f,  r * 0.333f };
        ui.uvScale = { 0.2f,      0.333f };
    }
}

void ResultScene::CreateButtons()
{
    m_buttons.clear();
    m_elapsedTime = 0.0f;

    const bool isClear = s_resultData.isCleared;
    const char* frameTexId = isClear ? "BTN_UNDER_GAMECLEAR" : "BTN_UNDER_KUMO";

    // --- ラムダ関数の定義変更 ---
    // indexによる計算を廃止し、pos(座標)とframeSize(土台サイズ)を引数に追加
    auto createResultButton =
        [&](const char* textTex,
            DirectX::XMFLOAT2 basePos,     // 1. 土台を置く中心座標
            DirectX::XMFLOAT2 frameSize,    // 2. 土台の大きさ
            DirectX::XMFLOAT2 textSize,     // 3. 文字の大きさ
            DirectX::XMFLOAT2 textOffset,   // 4. 土台の中心から文字をどれだけズラすか
            std::function<void()> onClick)
        {
            // 1) 土台フレームの生成
            EntityID frameEntity = m_coordinator->CreateEntity(
                TransformComponent({ basePos.x, basePos.y, 0.0f }, { 0,0,0 }, { frameSize.x, frameSize.y, 1.0f }),
                UIImageComponent(frameTexId, 1.5f, true, { 1,1,1,1 }),
                UIButtonComponent(ButtonState::Normal, true, nullptr, { frameSize.x, frameSize.y, 1.0f }),
                TagComponent(std::string(textTex) + "_Frame")
            );
            // 元のサイズを記憶（アニメーション用）
            m_coordinator->GetComponent<UIButtonComponent>(frameEntity).originalScale = { frameSize.x, frameSize.y, 1.0f };

            // 2) ボタンテキストの生成（座標に offset を加算）
            EntityID textEntity = m_coordinator->CreateEntity(
                TransformComponent({ basePos.x + textOffset.x, basePos.y + textOffset.y, 0.0f }, { 0,0,0 }, { textSize.x, textSize.y, 1.0f }),
                UIImageComponent(textTex, 2.0f, true, { 1,1,1,1 }),
                UIButtonComponent(ButtonState::Normal, true, onClick, { textSize.x, textSize.y, 1.0f }),
                TagComponent(textTex)
            );
            // 元のサイズを記憶（アニメーション用）
            m_coordinator->GetComponent<UIButtonComponent>(textEntity).originalScale = { textSize.x, textSize.y, 1.0f };

            m_buttons.push_back({ textEntity, frameEntity });
    };

    // 1. SELECTボタン
    createResultButton(
        "BTN_BACK_STAGE_SELECT",
        { SCREEN_WIDTH * 0.49f, SCREEN_HEIGHT * 0.93f }, // ポジション
        { 260.0f, 120.0f },                             // 【土台サイズ】
        { 220.0f, 60.0f },                              // 【文字サイズ】
        { 0.0f, 5.0f },                                 // 【文字のズレ】
        []() {
            ResultScene::isClear = false;
            SceneManager::ChangeScene<StageSelectScene>();
        }
    );

    // 2. RETRYボタン
    createResultButton(
        "BTN_RETRY",
        { SCREEN_WIDTH * 0.69f, SCREEN_HEIGHT * 0.93f },
        { 260.0f, 120.0f },                             // 【土台サイズ】
        { 220.0f, 60.0f },                              // 【文字サイズ】
        { 20.0f, 5.0f },                                 // 【文字のズレ】
        []() {
            ResultScene::isClear = false;
            GameScene::SetStageNo(ResultScene::s_resultData.stageID);
            SceneManager::ChangeScene<GameScene>();
        }
    );

    // 3. TITLEボタン
    createResultButton(
        "BTN_BACK_TITLE",
        { SCREEN_WIDTH * 0.89f, SCREEN_HEIGHT * 0.93f },
        { 260.0f, 120.0f },                             // 【土台サイズ】
        { 220.0f, 60.0f },                              // 【文字サイズ】
        { 20.0f, 5.0f },                                 // 【文字のズレ】
        []() {
            ResultScene::isClear = false;
            SceneManager::ChangeScene<TitleScene>();
        }
    );
    
}

void ResultScene::CreateNumberDisplay(int number, DirectX::XMFLOAT2 pos)
{
    // 数字を文字列に変換 (例: 3 -> "3", 12 -> "12")
    std::string str = std::to_string(number);

    float w = 40.0f; // 文字の幅
    float h = 60.0f; // 文字の高さ

    // 中央揃えのために開始位置を調整
    float totalWidth = str.length() * w;
    float startX = pos.x - (totalWidth / 2.0f) + (w / 2.0f);

    for (size_t i = 0; i < str.length(); ++i)
    {
        int idx = str[i] - '0';



        int r = 0; // 行 (0:上段, 1:下段)
        int c = 0; // 列 (0～5)

        if (idx >= 1 && idx <= 6)
        {
            // 1～6 は上段 (Row 0)
            r = 0;
            c = idx - 1; // 1なら0番目, 6なら5番目
        }
        else if (idx >= 7 && idx <= 9)
        {
            // 7～9 は下段 (Row 1)
            r = 1;
            c = idx - 7; // 7なら0番目
        }
        else if (idx == 0)
        {
            // 0 は下段の4番目 (Row 1, Col 3)
            r = 1;
            c = 3;
        }

        // UV計算
        // 横は6等分 (1.0 / 6)
        // 縦は2等分 (1.0 / 2)
        float uvW = 1.0f / 6.0f;
        float uvH = 1.0f / 2.0f;

        float uvX = c * uvW;
        float uvY = r * uvH;



        EntityID d = m_coordinator->CreateEntity(
            TransformComponent({ startX + i * w, pos.y, 0.0f }, { 0,0,0 }, { w, h, 1 }),
            UIImageComponent("UI_CLEARNUMBERS", 1.0f, true, { 1,1,1,1 }), // 色を変えたい場合はここを変更
            TagComponent("AnimNumber")
        );

        auto& ui = m_coordinator->GetComponent<UIImageComponent>(d);
        ui.uvPos = { uvX, uvY };
        ui.uvScale = { uvW, uvH };
    }
}

void OnStageClear(int clearedStageNo)
{
    // 最大解放ステージを更新
    int nextStage = clearedStageNo + 1;

    if (nextStage <= 18) // 18ステージまで
    {
        StageUnlockProgress::UnlockStage(nextStage);
        StageUnlockProgress::SetPendingRevealStage(nextStage);
        StageUnlockProgress::Save();
    }

    // StageSelectSceneに戻ると、自動的に次のステージのページに切り替わり、
    // カードが出現演出される
}
