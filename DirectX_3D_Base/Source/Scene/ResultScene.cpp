/*****************************************************************//**
 * @file    ResultScene.cpp
 * @brief   リザルト画面シーン
 *
 * @details
 *  - ゲームの結果は GameControlSystem から ResultData 経由で受け取る
 *  - このシーンは「見た目の Entity を並べるだけ」
 *  - 星やスタンプのアニメーションは ResultControlSystem が行う
 *
 *********************************************************************/

 // ===== インクルード =====
#include "Scene/ResultScene.h"

#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"

#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/Core/TagComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIButtonComponent.h>
#include <ECS/Components/UI/UICursorComponent.h>

#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/UI/UIRenderSystem.h>
#include <ECS/Systems/UI/CursorSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include <ECS/Systems/Rendering/EffectSystem.h>
#include <ECS/Systems/Core/ResultControlSystem.h>

#include <DirectXMath.h>
#include <iostream>

using namespace DirectX;
using namespace ECS;
using namespace std;

// 入力関連（今は未使用、将来パッド入力などに差し替え予定）
static bool IsInputTitle() { return false; }

// 静的メンバの実体
bool       ResultScene::isClear = false;
int        ResultScene::finalItenCount = 0;
ResultData ResultScene::s_resultData = {};

namespace
{
    // ステージID -> リザルト用ステージ名画像のアセット名を返す
    // 画像が用意できたらここを書き換えるだけでOK
    std::string GetResultStageNameTexture(const std::string& stageID)
    {
        //   今は全部同じ画像を使っておいて、
        //   実際のステージ名画像が出来たらここを差し替える想定
        //   例:
        //   if (stageID == "ST_001") return "RES_STAGE_NAME_001";
        //   if (stageID == "ST_002") return "RES_STAGE_NAME_002";
        //   ...
        //   みたいに増やしていく
        (void)stageID; // 今は未使用抑止

        return "BTN_BACK_STAGE_SELECT";   // 仮：いま表示されている SELECT 画像
    }
}


//===== ResultScene メンバー関数 =====

void ResultScene::Init()
{
    // --- 1. ECS 初期化 & カメラ ---
    m_coordinator = std::make_shared<ECS::Coordinator>();
    ECS::ECSInitializer::InitECS(m_coordinator);

    // 2D カメラ（他シーンと同じく EntityFactory を使用）
    ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), { 0.0f, 0.0f, 0.0f });

    bool isClear = s_resultData.isCleared;

    // ============================================================
    // 2. GAME CLEAR / GAME OVER 共通のレイアウト
    // ============================================================

   // ============================================================
   // ゲームクリア時のUI
   // ============================================================
    if (isClear)
    {
        // 1) 背景（本＋カード）
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0 },
                { 0, 0, 0 },
                { SCREEN_WIDTH, SCREEN_HEIGHT, 1 }
            ),
            UIImageComponent(
                "BG_GAME_CLEAR",
                0.0f,
                true,
                { 1.0f, 1.0f, 1.0f, 1.0f }
            )
        );

        // 2) GAME CLEAR ロゴ
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.09f, 0 },
                { 0, 0, 0 },
                { 760.0f, 96.0f, 1.0f }
            ),
            UIImageComponent(
                "UI_GAME_CLEAR",
                0.0f,
                true,
                { 1, 1, 1, 1 }
            )
        );
        {
            float plateW = 320.0f;
            float plateH = 80.0f;
            float plateX = SCREEN_WIDTH * 0.30f;
            float plateY = SCREEN_HEIGHT * 0.23f;
        }



        // 4) 右ページ：★3つ（5）
  //    1: STAR_TEXT1 … 警備員から逃げきる
  //    2: STAR_TEXT2 … お宝を順番通りにすべて盗む
  //    3: STAR_TEXT3 … ○分以内にクリア
        {
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

                float baseY = SCREEN_HEIGHT * 0.30f; // 一番上の行のY
                float gapY = 55.0f;                 // 行間
                float starX = SCREEN_WIDTH * 0.535f; // ★
                float captionX = SCREEN_WIDTH * 0.67f;  // テキスト

                for (int i = 0; i < 3; ++i)
                {
                    float y = baseY + i * gapY;

                    // 条件テキスト（STAR_TEXT1/2/3）
                    if (stars[i])
                    {
                        // ★を取った行だけアニメ用タグを付ける
                        m_coordinator->CreateEntity(
                            TransformComponent(
                                { captionX, y, 0.0f },
                                { 0.0f, 0.0f, 0.0f },
                                { 320.0f, 60.0f, 1.0f }
                            ),
                            UIImageComponent(
                                conditionTex[i],
                                1.0f,
                                true,
                                { 1.0f, 1.0f, 1.0f, 1.0f }
                            ),
                            TagComponent("AnimStarText")
                        );
                    }
                    else
                    {
                        // 取れていない行は普通に固定表示
                        m_coordinator->CreateEntity(
                            TransformComponent(
                                { captionX, y, 0.0f },
                                { 0.0f, 0.0f, 0.0f },
                                { 320.0f, 60.0f, 1.0f }
                            ),
                            UIImageComponent(
                                conditionTex[i],
                                1.0f,
                                true,
                                { 1.0f, 1.0f, 1.0f, 1.0f }
                            )
                        );
                    }

                    // ★ Off（枠）
                    m_coordinator->CreateEntity(
                        TransformComponent(
                            { starX, y, 0.0f },
                            { 0.0f, 0.0f, 0.0f },
                            { 50.0f, 50.0f, 1.0f }
                        ),
                        UIImageComponent(
                            "ICO_STAR_OFF",
                            1.0f,
                            true,
                            { 1.0f, 1.0f, 1.0f, 1.0f }
                        )
                    );

                    // ★ On（ポップアニメ用）
                    if (stars[i])
                    {
                        m_coordinator->CreateEntity(
                            TransformComponent(
                                { starX, y, 0.0f },
                                { 0.0f, 0.0f, 0.0f },
                                { 0.0f, 0.0f, 1.0f }
                            ),
                            UIImageComponent(
                                "ICO_STAR_ON",
                                2.0f,
                                true,
                                { 1.0f, 1.0f, 1.0f, 1.0f }
                            ),
                            TagComponent("AnimStar")
                        );
                    }
                }
            }


            // 5) クリアタイム（★ブロックの左側：8）
            // タイム表示
            float timeY = SCREEN_HEIGHT * 0.38f;
            float timeX = SCREEN_WIDTH * 0.365f;
            CreateTimeDisplay(s_resultData.clearTime, { timeX, timeY });

            // タイムのすぐ下に、拾ったアイテムを並べる
            {
                const auto& icons = s_resultData.collectedItemIcons;
                int count = static_cast<int>(icons.size());
                if (count > 0)
                {
                    const float iconW = 64.0f;   // アイコンの大きさ

                    // ★ まず基準のY（1行目）を決める
                    float baseY = timeY + 80.0f; // タイムより少し下

                    // ★ ここに「アイテムごとの座標」を並べる（最大6個の例）
                    struct IconPos { float x; float y; };
                    IconPos positions[] = {
                        // 1行目（左 → 右）
                        { timeX - 100.0f, baseY        }, // 0番目
                        { timeX - 120.0f, baseY        }, // 1番目
                        { timeX + 80.0f, baseY        }, // 2番目
                        { timeX - 120.0f, baseY + 80.0f }, // 3番目
                        { timeX - 40.0f, baseY + 80.0f }, // 4番目
                        { timeX + 40.0f, baseY + 80.0f }  // 5番目
                    };

                    // 描画できる最大数（positions の数まで）
                    int maxIcons = std::min(count, (int)(sizeof(positions) / sizeof(positions[0])));

                    for (int i = 0; i < maxIcons; ++i)
                    {
                        float x = positions[i].x;
                        float y = positions[i].y;

                        m_coordinator->CreateEntity(
                            TransformComponent({ x, y, 0 }, { 0,0,0 }, { iconW, iconW, 1 }),
                            UIImageComponent(
                                icons[i].c_str(),
                                1.0f,
                                true,
                                { 1,1,1,1 }
                            )
                        );
                    }
                }


            }

            // 6) スタンプ（本の右上にドーン：6）
            {
                m_coordinator->CreateEntity(
                    TransformComponent(
                        { SCREEN_WIDTH * 0.80f, SCREEN_HEIGHT * 0.25f, 0 }, // 右上寄り
                        { 0,0,0 },
                        { 200,200, 1 }            // 大きめ
                    ),
                    UIImageComponent(
                        "ICO_STAMP1",             // 1種類だけ使用
                        3.0f,
                        true,
                        { 1,1,1,0 }               // 最初は透明（ResultControlSystemでフェードイン）
                    ),
                    TagComponent("AnimStamp")      // ★ これを ResultControlSystem が拾う
                );
            }


            // ステージ名プレート（後で画像差し替え予定）
            {
                // 左ページの上あたり
                float plateW = 320.0f;
                float plateH = 80.0f;
                float plateX = SCREEN_WIDTH * 0.33f;
                float plateY = SCREEN_HEIGHT * 0.25f;

                // ★ 選んだステージIDから、使う画像のアセット名を決める
                const std::string texID = GetResultStageNameTexture(s_resultData.stageID);

                m_coordinator->CreateEntity(
                    TransformComponent(
                        { plateX, plateY, 0.0f },
                        { 0.0f, 0.0f, 0.0f },
                        { plateW, plateH, 1.0f }
                    ),
                    UIImageComponent(
                        texID.c_str(),      // ← ここだけ差し替え
                        1.5f,
                        true,
                        { 1.0f, 1.0f, 1.0f, 1.0f }
                    )
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
                { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f },
                { 0.0f, 0.0f, 0.0f },
                { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1.0f }
            ),
            UIImageComponent("BG_GAME_OVER", 0.0f, true, { 1,1,1,1 })
        );

        // GAME OVER ロゴ
        m_coordinator->CreateEntity(
            TransformComponent(
                { SCREEN_WIDTH * 0.3f, SCREEN_HEIGHT * 0.15f, 0.0f },
                { 0.0f, 0.0f, 0.0f },
                { 680,96,1 }
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
                const float iconSize = 80.0f;   // アイコンの大きさ
                const float margin = 20.0f;   // アイコン同士の間隔

                // 画面下部ボタンの少し上、左から右へ並べるイメージ
                float baseY = SCREEN_HEIGHT * 0.45f;
                float baseX = SCREEN_WIDTH * 0.1f;

                for (size_t i = 0; i < count; ++i)
                {
                    float x = baseX + (iconSize + margin) * static_cast<float>(i);
                    float y = baseY;

                    bool collected = flags[i];

                    DirectX::XMFLOAT4 color;
                    if (collected)
                    {
                        // 取れたお宝：普通の色
                        color = { 1.0f, 1.0f, 1.0f, 1.0f };
                    }
                    else
                    {
                        // 取れていないお宝：灰色
                        color = { 0.3f, 0.3f, 0.3f, 0.7f };
                    }

                    m_coordinator->CreateEntity(
                        TransformComponent(
                            { x, y, 0.0f },
                            { 0.0f, 0.0f, 0.0f },
                            { iconSize, iconSize, 1.0f }
                        ),
                        UIImageComponent(
                            icons[i].c_str(),
                            1.0f,
                            true,
                            color
                        )
                    );
                }
            }
        }


    }

    // --------------------------------------------------------
    // 3. 下部ボタン（RETRY / SELECT / TITLE）
    // --------------------------------------------------------
    CreateButtons();

    // --------------------------------------------------------
    // 4. カーソル
    // --------------------------------------------------------
    m_coordinator->CreateEntity(
        TransformComponent({ 0,0,0 }, { 0,0,0 }, { 64,64,1 }),
        UIImageComponent("ICO_CURSOR", 5.0f),
        UICursorComponent()
    );

    std::cout << "ResultScene::Init() - completed." << std::endl;
}

void ResultScene::Uninit()
{
    // エフェクトシステムの後始末
    if (auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        effectSystem->Uninit();
    }

    ECS::ECSInitializer::UninitECS();
    m_coordinator.reset();
}

void ResultScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);
}

void ResultScene::Draw()
{
    // 背景 UI
    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(true);
    }

    // 3D（あれば）
    if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
    {
        system->DrawSetup();
        system->DrawEntities();
    }

    // エフェクト
    if (auto system = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        system->Render();
    }

    // 前景 UI（カーソルなど）
    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(false);
    }
}

// ----------------------------------------------------------
// タイム表示用スプライト作成 ("MM:SS.d")
// ----------------------------------------------------------
void ResultScene::CreateTimeDisplay(float time, DirectX::XMFLOAT2 pos)
{
    // 0.1秒単位に丸める
    int tInt = static_cast<int>(time * 10.0f);
    int min = (tInt / 600) % 100;
    int sec = (tInt / 10) % 60;
    int dsec = tInt % 10;

    // フォントシートのインデックス
    // 0–9: 0〜9, ':'→11, '.'→12 として扱う
    int digits[] = {
        min / 10, min % 10,
        11,
        sec / 10, sec % 10,
        12,
        dsec
    };

    float w = 40.0f;
    float h = 60.0f;
    float startX = pos.x - (7 * w) / 2.0f;

    for (int i = 0; i < 7; ++i)
    {
        EntityID d = m_coordinator->CreateEntity(
            TransformComponent({ startX + i * w, pos.y, 0.0f }, { 0,0,0 }, { w,h,1 }),
            UIImageComponent("UI_FONT", 1.0f, true, { 1,1,1,1 })   // result_time.png
        );

        // UV 設定（5x3 のフォントシートを想定）
        int idx = digits[i];
        int r = (idx <= 9) ? idx / 5 : 2;
        int c = (idx <= 9) ? idx % 5 : (idx - 10);

        auto& ui = m_coordinator->GetComponent<UIImageComponent>(d);
        ui.uvPos = { c * 0.2f,  r * 0.333f };
        ui.uvScale = { 0.2f,      0.333f };
    }
}

// ----------------------------------------------------------
// 下部ボタン生成（RETRY / SELECT / TITLE）
// ----------------------------------------------------------
void ResultScene::CreateButtons()
{
    // クリアしているかどうかでゲームオーバーを判定
    const bool isClear = s_resultData.isCleared;

    // ボタンの中心Y（必要ならゲームオーバーだけ少し上げてもOK）
    const float y = SCREEN_HEIGHT * 0.93f;

    // 土台フレームのサイズ
    const float frameW = 260.0f;
    const float frameH = 90.0f;

    // 中の文字画像(RETRY/SELECT/TITLE) のサイズ
    const float textW = 210.0f;
    const float textH = 60.0f;

    // ボタン同士の間隔
    const float spacing = 15.0f;

    // ３つ分の全幅（= 並べて真ん中に置くために使う）
    const float totalWidth = frameW * 3.0f + spacing * 2.0f;

    // 一番左のボタンの中心X
    const float firstX = (SCREEN_WIDTH * 0.6f) - totalWidth * 0.5f + frameW * 0.5f;

    // ★ ここでクリア／ゲームオーバーで使う土台テクスチャを切り替える
    const char* frameTexId = isClear
        ? "BTN_UNDER_RISULT"  // ← btn_result_normal1.png 用
        : "BTN_UNDER_GAMEOVER";  // ← btn_result_normal.png 用

    auto createResultButton =
        [&](int index, const char* textTex, std::function<void()> onClick)
        {
            const float x = firstX + index * (frameW + spacing);

            // ① 土台（クリア or ゲームオーバーで絵を切り替え）
            m_coordinator->CreateEntity(
                TransformComponent({ x, y, 0.0f }, { 0,0,0 }, { frameW, frameH, 1.0f }),
                UIImageComponent(
                    frameTexId,   // ← ここが切り替わる
                    1.5f,
                    true,
                    { 1,1,1,1 }
                )
            );

            // ② 手前の文字（RETRY / SELECT / TITLE）
            m_coordinator->CreateEntity(
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
                    onClick
                )
            );
        };

    // LEFT: RETRY
    createResultButton(
        0,
        "BTN_RETRY",
        []()
        {
            GameScene::SetStageNo(ResultScene::s_resultData.stageID);
            SceneManager::ChangeScene<GameScene>();
        }
    );

    // CENTER: SELECT（ステージセレクト）
    createResultButton(
        1,
        "BTN_BACK_STAGE_SELECT",
        []()
        {
            SceneManager::ChangeScene<StageSelectScene>();
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
