/*****************************************************************//**
 * @file    ResultScene.cpp
 * @brief   リザルト画面シーン
 *********************************************************************/

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

void ResultScene::Init()
{
    // --- 1. ECS 初期化 & カメラ ---
    m_coordinator = std::make_shared<ECS::Coordinator>();
    ECS::ECSInitializer::InitECS(m_coordinator);

    // エフェクトシステムに対して「UI用カメラ設定」を適用::織田
    if (auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        // これを呼ばないと、前のシーンの2D設定が残ってしまい、
        // 奥行きやスケールが正しく反映されません。
        effectSystem->ClearOverrideCamera();
    }

    ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0.0f, 0.0f, -20.0f });

    const bool isClear = s_resultData.isCleared;
    //  メンバに保存（CreateButtons で使う)
    m_isClear = isClear;

    // Stage progress / best-time update (safety: if SetResultData wasn't called)
    if (!g_progressHandled)
    {
        ResultScene::SetResultData(s_resultData);
    }
   


    // NOTE:
    //  isCleared==true  : GAME CLEAR 表示
    //  isCleared==false : GAME OVER 表示
    //  ここが逆だと「クリアしてもゲームオーバー側」に見える。
    if (isClear == true) //織田
    {
        // 1) 背景
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 100 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAME_CLEAR", 0.0f, true, { 1,1,1,1 })
        );

        // 2) GAME CLEAR ロゴ
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.09f, 0 },
                { 0, 0, 0 },
                { 760.0f, 96.0f, 1.0f }
            ),
            UIImageComponent("UI_GAME_CLEAR", 0.0f, true, { 1,1,1,1 })
        );

        // 右ページ：★3つ
        {
            bool stars[3] = {
                !s_resultData.wasSpotted,
                s_resultData.collectedAllOrdered,
                s_resultData.clearedInTime
            };

            const char* conditionTex[3] = {
                "STAR_TEXT1",
                "STAR_TEXT2",
                "STAR_TEXT3"
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

                // 条件テキスト（そのまま）
                if (stars[i])
                {
                    m_coordinator->CreateEntity(
                        TransformComponent({ captionX, y, 0.0f }, { 0,0,0 }, { 320.0f, 60.0f, 1.0f }),
                        UIImageComponent(conditionTex[i], 1.0f, true, { 1,1,1,1 }),
                        TagComponent("AnimStarText")
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
                    UIImageComponent("ICO_STAR_OFF", 1.0f, true, { 1,1,1,1 })
                );

                // ★ On（ポップアニメ用）
                if (stars[i])
                {
                    // 最終サイズ(onSize)をTransformに入れておく（ResultControlSystemが0→最終へポップさせる）
                    m_coordinator->CreateEntity(
                        TransformComponent({ starX, y, 0.0f }, { 0,0,0 }, { onSize, onSize, 1.0f }),
                        UIImageComponent("ICO_STAR_ON", 2.0f, true, { 1,1,1,1 }),
                        TagComponent(std::string("AnimStar") + std::to_string(i)) // AnimStar0/1/2
                    );
                }
            }

            // クリアタイム
            float timeY = SCREEN_HEIGHT * 0.38f;
            float timeX = SCREEN_WIDTH * 0.365f;
            CreateTimeDisplay(s_resultData.clearTime, { timeX, timeY });

            // タイムの下：拾ったアイテム
            {
                const auto& icons = s_resultData.collectedItemIcons;
                int count = static_cast<int>(icons.size());
                if (count > 0)
                {
                    const float iconW = 80.0f;
                    float baseY2 = timeY + 80.0f;

                    struct IconPos { float x; float y; };
                    IconPos positions[] = {
                        { timeX - 160.0f, baseY2 + 40.5f},
                        { timeX - 80.0f, baseY2 + 40.5f},
                        { timeX - 0.0f,  baseY2 + 40.5f},
                        { timeX + 405.0f, baseY2 + 50.0f },
                        { timeX + 323.0f,  baseY2 + 50.0f },
                        { timeX + 240.0f,  baseY2 + 50.0f }
                    };

                    int maxIcons = std::min(count, (int)(sizeof(positions) / sizeof(positions[0])));
                    for (int i = 0; i < maxIcons; ++i)
                    {
                        m_coordinator->CreateEntity(
                            TransformComponent({ positions[i].x, positions[i].y, 0 }, { 0,0,0 }, { iconW, iconW, 1 }),
                            UIImageComponent(icons[i].c_str(), 1.0f, true, { 1,1,1,1 })
                        );
                    }
                }
            }

            // ダイヤテキスト
            m_coordinator->CreateEntity(
                TransformComponent(
                    { SCREEN_WIDTH * 0.46f, SCREEN_HEIGHT * 0.55f, 0 },
                    { 0, 0, 0 },
                    { 330.0f, 40.0f, 1.0f }
                ),
                UIImageComponent("GEMS_TEXT", 0.0f, true, { 1,1,1,1 })
            );

            // 絵画テキスト
            m_coordinator->CreateEntity(
                TransformComponent(
                    { SCREEN_WIDTH * 0.77f, SCREEN_HEIGHT * 0.55f, 0 },
                    { 0, 0, 0 },
                    { 330.0f, 40.0f, 1.0f }
                ),
                UIImageComponent("PAINTING_TEXT", 0.0f, true, { 1,1,1,1 })
            );

            // 化石テキスト
            m_coordinator->CreateEntity(
                TransformComponent(
                    { SCREEN_WIDTH * 0.27f, SCREEN_HEIGHT * 0.68f, 0 },
                    { 0, 0, 0 },
                    { 330.0f, 40.0f, 1.0f }
                ),
                UIImageComponent("FOSSIL_TEXT", 0.0f, true, { 1,1,1,1 })
            );

            // 陶器テキスト
            m_coordinator->CreateEntity(
                TransformComponent(
                    { SCREEN_WIDTH * 0.57f, SCREEN_HEIGHT * 0.68f, 0 },
                    { 0, 0, 0 },
                    { 330.0f, 40.0f, 1.0f }
                ),
                UIImageComponent("POTTERY_TEXT", 0.0f, true, { 1,1,1,1 })
            );


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

            //EffectComponent(
            //    /* AssetID  */ "EFK_TITLE_SHINE",
            //    /* Loop     */ true,
            //    /* AutoPlay */ true,
            //    /* Offset   */{ 0.0f, 0.0f, -3.0f },
            //    /* Scale    */ 0.3f


           /* {
                m_coordinator->CreateEntity(
                    TransformComponent(
                        { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f,1.0f },
                        { 0,0,0 },
                        { 100.0f, 100.0f, 1.0f }
                    ),
                    EffectComponent("EFK_STAMP1", true,true, { 0,0,-3.0 },0.3f)
                );
            }*/

            
            

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

        // 背景
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent("BG_GAME_OVER", 0.0f, true, { 1,1,1,1 })
        );
        // --------------------------------------------------------
        // パラパラ漫画アニメーション（Result演出）
        // --------------------------------------------------------
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.6f, SCREEN_HEIGHT * 0.5f, 0.0f }, // 表示位置
                { 0.0f, 0.0f, 0.0f },
                { 600.0f, 600.0f, 1.0f } // 1枚分のサイズ
            ),
            UIImageComponent(
                "RESULT_ANIM",   // ★ 最初のフレーム
                1.0f,
                true,
                { 1,1,1,1 }
            ),
            TagComponent("RESULT_ANIM") // ★ ResultControlSystem が拾う
        );

        // GAME OVER ロゴ
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.2f, SCREEN_HEIGHT * 0.09f, 0.0f },//0.1,0.15
                { 0.0f, 0.0f, 0.0f },
                { 460,72,1 }//680,96
            ),
            UIImageComponent("UI_GAME_OVER", 0.0f, true, { 1,1,1,1 })
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

        // D) ゲームオーバー時：お宝一覧（順番どおり＋未取得灰色）
        {
            const auto& icons = s_resultData.orderedItemIcons;
            const auto& flags = s_resultData.orderedItemCollected;
            size_t count = std::min(icons.size(), flags.size());

            if (count > 0)
            {
                const float iconSize = 80.0f;
                const float margin = 20.0f;

                // 画面下部ボタンの少し上、左から右へ並べるイメージ
                float baseY = SCREEN_HEIGHT * 0.25f;
                float baseX = SCREEN_WIDTH * 0.05f;

                for (size_t i = 0; i < count; ++i)
                {
                    float x = baseX + (iconSize + margin) * static_cast<float>(i);
                    float y = baseY;

                    bool collected = flags[i];
                    DirectX::XMFLOAT4 color = collected ? DirectX::XMFLOAT4{ 1,1,1,1 } : DirectX::XMFLOAT4{ 0.3f,0.3f,0.3f,0.7f };

                    m_coordinator->CreateEntity(
                        TransformComponent({ x, y, 0.0f }, { 0,0,0 }, { iconSize, iconSize, 1.0f }),
                        UIImageComponent(icons[i].c_str(), 1.0f, true, color)
                    );
                }
            }
        }

        // BGM再生
        ECS::EntityID m_gameoverBGM = ECS::EntityFactory::CreateLoopSoundEntity(
            m_coordinator.get(),
            "BGM_TEST",
            0.5f
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
    if (auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        effectSystem->ClearOverrideCamera();
        effectSystem->Uninit();
    }

    ECS::ECSInitializer::UninitECS();
    m_coordinator.reset();
}

void ResultScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);

    

    // 織田
    m_elapsedTime += deltaTime;

    for (const auto& pair : m_buttons)
    {
        auto& button = m_coordinator->GetComponent<UIButtonComponent>(pair.textEntity);

        float targetScale = BUTTON_NORMAL_SCALE;

        if (button.state == ButtonState::Hover)
        {
            targetScale = PULSE_CENTER_SCALE + PULSE_AMPLITUDE * std::sin(m_elapsedTime * PULSE_SPEED);


        }
        auto UpdateEntityScale = [&](EntityID entity, float baseW, float baseH)
            {
                auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

                float& currentRatio = transform.scale.z;

                currentRatio += (targetScale - currentRatio) * LERP_SPEED * deltaTime;

                transform.scale.x = baseW * currentRatio;
                transform.scale.y = baseH * currentRatio;

            };

        UpdateEntityScale(pair.textEntity, 210.0f, 60.0f);
        UpdateEntityScale(pair.frameEntity, 260.0f, 90.0f);
    }

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
            UIImageComponent("UI_FONT", 1.0f, true, { 1,1,1,1 })
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
    m_buttons.clear(); // 織田：ペアリストをクリア

    m_elapsedTime = 0.0f; // 織田：初期化

    const bool isClear = s_resultData.isCleared;

    // ボタンの中心Y（必要ならゲームオーバーだけ少し上げてもOK）
    const float y = SCREEN_HEIGHT * 0.94f;//0.93

    // 土台フレームのサイズ
    const float frameW = 260.0f;//260
    const float frameH = 90.0f;//90

    // 中の文字画像(RETRY/SELECT/TITLE) のサイズ
    const float textW = 210.0f;//210
    const float textH = 60.0f;//60

    // ボタン同士の間隔
    const float spacing = 5.0f;//15

    const float totalWidth = frameW * 3.0f + spacing * 2.0f;

    // 一番左のボタンの中心X
    const float firstX = (SCREEN_WIDTH * 0.740f) - totalWidth * 0.6f + frameW * 0.5f;
    //    const float firstX = (SCREEN_WIDTH * 0.6f) - totalWidth * 0.5f + frameW * 0.5f;

    // ★ ここでクリア／ゲームオーバーで使う土台テクスチャを切り替える
    const char* frameTexId = isClear
        ? "BTN_UNDER_CLEAR"  // ← btn_result_normal.png 用
        : "BTN_UNDER_GAMEOVER";  // ← btn_result_normal1.png 用

    auto createResultButton =
        [&](int index, const char* textTex, std::function<void()> onClick)
        {
            const float x = firstX + index * (frameW + spacing);

            // 織田：frameEntityに背景フレームの情報を入れる
            EntityID frameEntity = m_coordinator->CreateEntity(
                TransformComponent({ x, y, 0.0f }, { 0,0,0 }, { frameW, frameH, 1.0f }),
                UIImageComponent(
                    frameTexId,   // ← ここが切り替わる
                    1.5f,
                    true,
                    { 1,1,1,1 }
                ),
                UIButtonComponent(
                    ButtonState::Normal,
                    true,
                    nullptr,                     // ★ 背景はクリック処理しない
                    { frameW, frameH, 1.0f }
                ),
                TagComponent(textTex)
            );
            // 織田：textEntityにボタンテキストの情報を入れる
            EntityID textEntity = m_coordinator->CreateEntity(
                TransformComponent({ x, y, 0.0f }, { 0,0,0 }, { textW, textH, 1.0f }),
                UIImageComponent(
                    textTex,
                    2.0f,
                    true,
                    { 1,1,1,1 }
                ),
                UIButtonComponent(
                    ButtonState::Normal,
                    true,
                    onClick,
                    { textW, textH, 1.0f }//
                ),
                TagComponent(textTex) // 追加
            );

            m_buttons.push_back({ textEntity, frameEntity });// 織田：ペアにしてリストに保存
        };

    // LEFT: SELECT（ステージセレクト）
    createResultButton(
        0,
        "BTN_BACK_STAGE_SELECT",
        []()
        {
            SceneManager::ChangeScene<StageSelectScene>();
        }
    );
    // CENTER: RETRY
    createResultButton(
        1,
        "BTN_RETRY",
        []()
        {
            GameScene::SetStageNo(ResultScene::s_resultData.stageID);
            SceneManager::ChangeScene<GameScene>();
        }
    );


    // RIGHT: TITLE
    createResultButton(
        2,
        "BTN_BACK_TITLE",
        []()
        {
            SceneManager::ChangeScene<TitleScene>();
        }
    );
}