/*****************************************************************//**
 * @file    LoadingScene.cpp
 * @brief   ロード画面（LOADING文字表示 + 右下UVアニメ）
 *********************************************************************/

#include "Scene/LoadingScene.h"
#include "Scene/SceneManager.h"
#include "Scene/TitleScene.h"

#include "ECS/ECSInitializer.h"
#include "ECS/Systems/UI/UIRenderSystem.h"
#include "ECS/Systems/Core/ScreenTransition.h"

#include "Main.h" // SCREEN_WIDTH / SCREEN_HEIGHT

#include <cmath> // 追加: sin, abs
 // ★実体定義（ファイルの冒頭、namespaceの外などに配置）
float LoadingScene::s_minDisplaySec = 1.0f;
std::type_index LoadingScene::s_nextSceneType = typeid(TitleScene);
namespace
{
    int ClampInt(int v, int lo, int hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }
}

LoadingScene::EntityHandle LoadingScene::SpawnUI(
    const char* assetId,
    float x, float y,
    float w, float h,
    float depth,
    float r, float g, float b, float a)
{
    return m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */{ x, y, 0.0f },
            /* Rotation */{ 0.0f, 0.0f, 0.0f },
            /* Scale    */{ w, h, 1.0f }
        ),
        UIImageComponent(
            /* AssetID */ assetId,
            /* Depth   */ depth,
            /* Visible */ true,
            /* Color   */{ r, g, b, a }
        )
    );
}

void LoadingScene::SetLoadAnimFrame(int frameIndex)
{
    if (!m_hasLoadAnim) { return; }

    frameIndex = ClampInt(frameIndex, 0, kTotalFrames - 1);

    const int col = frameIndex;
    const int row = 0;

    auto& ui = m_coordinator->GetComponent<UIImageComponent>(m_loadAnimEntity);

    // 正規化UV（uvPos/uvScale）
    ui.uvScale.x = 1.0f / static_cast<float>(kCols);
    ui.uvScale.y = 1.0f / static_cast<float>(kRows);

    ui.uvPos.x = static_cast<float>(col) * ui.uvScale.x;
    ui.uvPos.y = static_cast<float>(row) * ui.uvScale.y;

    // もし上下が反転して見える場合は下記を有効化（今回row=0なので通常不要）
    // ui.uvPos.y = static_cast<float>(kRows - 1 - row) * ui.uvScale.y;
}

void LoadingScene::Init()
{
    m_coordinator = std::make_shared<ECS::Coordinator>();
    ECS::ECSInitializer::InitECS(m_coordinator);

    // ------------------------------------------------------------
    // 背景（黒）
    // ------------------------------------------------------------
    (void)SpawnUI(
        "UI_STAGE_FADE",
        SCREEN_WIDTH * 0.5f,
        SCREEN_HEIGHT * 0.5f,
        static_cast<float>(SCREEN_WIDTH),
        static_cast<float>(SCREEN_HEIGHT),
        -10.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // ------------------------------------------------------------
    // 左下配置パラメータ
    // ------------------------------------------------------------
    constexpr float kLetterSize = 90.0f;
    constexpr float kLetterSpacing = 14.0f;
    constexpr float kMarginLeft = 6.0f;
    constexpr float kMarginBottom = 6.0f;

    constexpr float kBaseDepth = 0.50f;
    constexpr float kDepthStep = 0.01f;

    // ------------------------------------------------------------
    // "LOADING"
    // ------------------------------------------------------------
    const char* kLetters[] =
    {
        "UI_LOAD_L",
        "UI_LOAD_O",
        "UI_LOAD_A",
        "UI_LOAD_D",
        "UI_LOAD_I",
        "UI_LOAD_N",
        "UI_LOAD_G",
    };
    constexpr int kLetterCount = static_cast<int>(sizeof(kLetters) / sizeof(kLetters[0]));

    const float baseCenterY = static_cast<float>(SCREEN_HEIGHT) - kMarginBottom - (kLetterSize * 0.5f);
    const float startX = kMarginLeft;

    // リスト初期化
    m_textEntities.clear();
    m_textBaseY.clear();

    for (int i = 0; i < kLetterCount; ++i)
    {
        const float x = startX + (kLetterSize * i) + (kLetterSpacing * i) + (kLetterSize * 0.5f);

        // Entityを生成し、ハンドルと基準Y座標を保存
        EntityHandle handle = SpawnUI(kLetters[i], x, baseCenterY, kLetterSize, kLetterSize, kBaseDepth + (kDepthStep * i));

        m_textEntities.push_back(handle);
        m_textBaseY.push_back(baseCenterY);
    }

    // ------------------------------------------------------------
    // "..."（UI_LOAD_PERIOD）
    // ------------------------------------------------------------
    const char* kDotAsset = "UI_LOAD_PERIOD";
    constexpr int   kDotCount = 3;
    constexpr float kDotSize = 26.0f;
    constexpr float kDotSpacing = 10.0f;
    constexpr float kGapLetterToDots = 12.0f;

    const float lettersWidth = (kLetterSize * kLetterCount) + (kLetterSpacing * (kLetterCount - 1));
    const float dotsStartX = startX + lettersWidth + kGapLetterToDots;

    for (int d = 0; d < kDotCount; ++d)
    {
        const float x = dotsStartX + (kDotSize * d) + (kDotSpacing * d) + (kDotSize * 0.5f);
        (void)SpawnUI(
            kDotAsset,
            x, baseCenterY,
            kDotSize, kDotSize,
            kBaseDepth + (kDepthStep * (kLetterCount + d))
        );
    }

    // ------------------------------------------------------------
    // 右下：UI_LOAD_ANIM（1枚で横30分割のUVアニメ）
    // ------------------------------------------------------------
    {
        constexpr float kAnimSize = 200.0f;
        const float x = static_cast<float>(SCREEN_WIDTH) - (kAnimSize * 0.6f);
        const float y = static_cast<float>(SCREEN_HEIGHT) - (kAnimSize * 0.6f);

        m_loadAnimEntity = SpawnUI("UI_LOAD_ANIM", x, y, kAnimSize, kAnimSize, kBaseDepth + 0.2f);
        m_hasLoadAnim = true;

        m_loadAnimFrame = 0;
        m_loadAnimTimer = 0.0f;

        // 初期フレームを適用（最初の1コマ）
        SetLoadAnimFrame(m_loadAnimFrame);
    }
    // ============================================================
       // ★ここに追加：起動時のフェードイン処理
       // 前のシーンから真っ暗な状態で遷移してくるため、ここから明るくします。
       // ============================================================
    ECS::EntityID fadeEffect = ScreenTransition::CreateOverlay(
        m_coordinator.get(),
        "UI_STAGE_FADE", // 黒いテクスチャ
        SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f,
        static_cast<float>(SCREEN_WIDTH * 2), static_cast<float>(SCREEN_HEIGHT * 2)
    );

    if (m_coordinator->HasComponent<UIImageComponent>(fadeEffect))
    {
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(fadeEffect);
        ui.color = { 0.0f, 0.0f, 0.0f, 1.0f }; // 最初は真っ黒
        ui.depth = 500000.0f;                 // 何よりも手前に置く
    }
    ScreenTransition::RequestFadeInEx(m_coordinator.get(), fadeEffect, 0.5f);
}

void LoadingScene::Uninit()
{
    ECS::ECSInitializer::UninitECS();
    m_coordinator.reset();

    // 追加: リストクリア
    m_textEntities.clear();
    m_textBaseY.clear();

    m_hasLoadAnim = false;
    m_loadAnimTimer = 0.0f;
    m_loadAnimFrame = 0;
    m_elapsed = 0.0f;
}

void LoadingScene::Update(float deltaTime)
{
    m_elapsed += deltaTime;
    m_coordinator->UpdateSystems(deltaTime);

    // ------------------------------------------------------------
    // 追加: 文字のウェーブ/ジャンプアニメーション
    // ------------------------------------------------------------
    if (!m_textEntities.empty())
    {
        constexpr float kJumpSpeed = 10.0f; // アニメーション速度
        constexpr float kJumpHeight = 15.0f; // 跳ねる高さ
        constexpr float kWaveLag = 0.5f;  // 文字ごとの遅延（ウェーブ具合）

        for (size_t i = 0; i < m_textEntities.size(); ++i)
        {
            // Transformコンポーネントを取得
            auto& transform = m_coordinator->GetComponent<TransformComponent>(m_textEntities[i]);

            // abs(sin) で「跳ねる（下にいかない）」動きにする
            float bounce = std::abs(std::sin(m_elapsed * kJumpSpeed - (static_cast<float>(i) * kWaveLag)));

            // 基準位置から上にオフセット (Y座標系が下向き正ならマイナス)
            transform.position.y = m_textBaseY[i] - (bounce * kJumpHeight);
        }
    }

    // UVアニメ更新（0..29 ループ）
    if (m_hasLoadAnim && kTotalFrames > 1)
    {
        const float frameSec = 1.0f / kAnimFPS;
        m_loadAnimTimer += deltaTime;

        while (m_loadAnimTimer >= frameSec)
        {
            m_loadAnimTimer -= frameSec;
            m_loadAnimFrame = (m_loadAnimFrame + 1) % kTotalFrames;
            SetLoadAnimFrame(m_loadAnimFrame);
        }
    }

    if (m_elapsed >= s_minDisplaySec)
    {
        // 登録されている型に基づいて SceneManager で遷移
        if (s_nextSceneType == typeid(OpeningScene)) {
            SceneManager::ChangeScene<OpeningScene>();
        }
        else if (s_nextSceneType == typeid(StageSelectScene)) {
            SceneManager::ChangeScene<StageSelectScene>();
        }
        else if (s_nextSceneType == typeid(GameScene)) {
            SceneManager::ChangeScene<GameScene>();
        }
        else {
            SceneManager::ChangeScene<TitleScene>(); // フォールバック
        }
    }
}

void LoadingScene::Draw()
{
    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(true);
        system->Render(false);
    }
}