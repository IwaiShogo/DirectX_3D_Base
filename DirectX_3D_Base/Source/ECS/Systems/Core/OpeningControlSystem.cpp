#include "ECS/Systems/Core/OpeningControlSystem.h"
#include "ECS/ECS.h"
#include "ECS/EntityFactory.h"


#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/Core/TagComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <DirectXMath.h>

using namespace ECS;

// -------------------------------------------------
//                内部状態
// -------------------------------------------------
namespace
{
    struct OpeningCharData
    {
        const char* texture;
        DirectX::XMFLOAT2 offset;
    };

    struct OpeningTextBlock
    {
        const OpeningCharData* chars;
        int charCount;
    };

    constexpr float CHAR_INTERVAL = 0.1f;
    constexpr float START_X = 700.0f;
    constexpr float START_Y = 850.0f;
    constexpr float SHADOW_OFFSET_Y = 20.0f;
    constexpr float CHAR_OFFSET_X = 10.0f;

    constexpr float FADE_TIME = 0.5f;

    constexpr float NEXT_PROMPT_DELAY = 0.3f;

    float s_timer = 0.0f;
    float s_promptAnimTime = 0.0f;
    float s_nextPromptDelayTimer = 0.0f;

    int   s_charIndex = 0;
    int   s_blockIndex = 0;
    bool  s_finished = false;
    bool s_moveBackward = false;
    bool s_nextPromptVisible = false;
    bool  s_waitNextPrompt = false;

    const OpeningCharData OPENING_TEXT_1[] =
    {
        {"OP_TXT_O_KATA",{-130.0f,-30.0f}},
        {"OP_TXT_RE",{-140.0f,-30.0f}},
        {"OP_TXT_HA",{-40.0f,-120.0f}},
        {"OP_TXT_HAN",{-170.0f,-35.0f}},
        {"OP_TXT_HITO",{-50,-120}},
        {"OP_TXT_MAE",{-50,-120}},
        {"OP_TXT_NO",{40,-240}},
        {"OP_TXT_KAI",{-70,-120}},
        {"OP_TXT_NUSU",{-10,-235}},
    };

    const OpeningCharData OPENING_TEXT_2[] =
    {
        {"OP_TXT_TA",{-200,-240}},
        {"OP_TXT_KU",{-200,-240}},
        {"OP_TXT_SA",{-200,-240}},
        {"OP_TXT_N",{-200,-240}},
        {"OP_TXT_NO",{-200,-240}},
        {"OP_TXT_O_GANA",{-200,-240}}, 
        {"OP_TXT_TAKARA",{-200,-240}}, 
        {"OP_TXT_WO",{-200,-240}},
        {"OP_TXT_NUSU",{-200,-240}}, 
        {"OP_TXT_MI",{-200,-240}}, 
        {"OP_TXT_NI",{-200,-240}}, 
        {"OP_TXT_IMA",{-200,-240}},
        {"OP_TXT_HI",{-200,-240}}, 
        {"OP_TXT_MO",{-200,-240}}, 
        {"OP_TXT_KAKE",{-200,-240}}, 
        {"OP_TXT_KE",{-200,-240}},
        {"OP_TXT_MA",{-200,-240}}, 
        {"OP_TXT_WA",{-200,-240}},
        {"OP_TXT_RU",{-200,-240}},
    };

    const OpeningCharData OPENING_TEXT_3[] =
    {
        {"OP_TXT_KA_GANA",{-230,-345}},//x減らすと左 y減らすと上
        {"OP_TXT_XTU",{ -230,-345}},
        {"OP_TXT_KO",{-230,-345}}, 
        {"OP_TXT_I",{-230,-345}}, 
        {"OP_TXT_I",{-210,-345}}, 
        {"OP_TXT_ICHI",{-215,-130}}, 
        {"OP_TXT_HITO",{-210,-130}},
        {"OP_TXT_MAE",{-210,-130}},
        {"OP_TXT_NO",{-120,-240}}, 
        {"OP_TXT_KAI",{-220,-125}},
        {"OP_TXT_NUSU",{-160,-240}}, 
        {"OP_TXT_WO",{-110,-240}}, 
        {"OP_TXT_ME",{-240,-125}}, 
        {"OP_TXT_YUBI",{-240,-125}},
        {"OP_TXT_SHI",{-240,-125}},
        {"OP_TXT_TE",{-252,-120}}, 
        {"OP_TXT_GA",{-160,-335}},
        {"OP_TXT_N",{140,-240}},
        {"OP_TXT_BA",{-160,-335}},
        {"OP_TXT_RU",{-170,-240}}, 
        {"OP_TXT_ZO",{-160,-330}},
        {"OP_TXT_BANG",{-150,-240}},
    };

    const char* OPENING_BG_TEXTURES[] =
    {
        "BG_OP01",
        "BG_OP02",
        "BG_OP03",
    };

    // -------------------------------------------------
    // ブロック定義
    // -------------------------------------------------
    const OpeningTextBlock OPENING_TEXT_BLOCKS[] =
    {
        { OPENING_TEXT_1, _countof(OPENING_TEXT_1) },
        { OPENING_TEXT_2, _countof(OPENING_TEXT_2) },
        { OPENING_TEXT_3, _countof(OPENING_TEXT_3) },
    };

    constexpr int BLOCK_COUNT =
        _countof(OPENING_TEXT_BLOCKS);

    enum class FadeState
    {
        None,
        FadeOut,
        FadeIn
    };

    FadeState s_fadeState = FadeState::None;
    float s_fadeTimer = 0.0f;

    ECS::EntityID s_backgroundEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID s_UIbackgroundEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID s_fadeEntity = ECS::INVALID_ENTITY_ID;

    std::vector<ECS::EntityID> s_openingTextEntities;
    std::vector<ECS::EntityID> s_nextPromptEntities;
}

// -------------------------------------------------
//                Opening 開始
// -------------------------------------------------
void OpeningControlSystem::StartOpening()
{
    // ===== 初期化 =====
    s_timer = 0.0f;
    s_charIndex = 0;
    s_blockIndex = 0;
    s_finished = false;
    s_fadeState = FadeState::None;
    s_fadeTimer = 0.0f;

    s_openingTextEntities.clear();

    s_backgroundEntity = ECS::INVALID_ENTITY_ID;
    s_UIbackgroundEntity = ECS::INVALID_ENTITY_ID;
    s_fadeEntity = ECS::INVALID_ENTITY_ID;

    m_active = true;

    ChangeBackground(0);
}

// -------------------------------------------------
//                背景切り替え
// -------------------------------------------------
void OpeningControlSystem::ChangeBackground(int blockIndex)
{
    if (blockIndex >= BLOCK_COUNT)
        return;

    // 既存背景削除
    if (s_backgroundEntity != ECS::INVALID_ENTITY_ID)
        m_coordinator->DestroyEntity(s_backgroundEntity);

    if (s_UIbackgroundEntity != ECS::INVALID_ENTITY_ID)
        m_coordinator->DestroyEntity(s_UIbackgroundEntity);

    s_backgroundEntity = m_coordinator->CreateEntity(
        TransformComponent(
            { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f },
            { 0, 0, 0 },
            { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1.0f }
        ),
        UIImageComponent(
            OPENING_BG_TEXTURES[blockIndex],
            0.0f,
            true,
            { 1,1,1,1 }
        ),
        TagComponent("OPENING_BG")
    );

    s_UIbackgroundEntity = m_coordinator->CreateEntity(
        TransformComponent(
            {
                SCREEN_WIDTH * 0.5f,
                SCREEN_HEIGHT * 0.5f + SHADOW_OFFSET_Y,
                0.0f
            },
            { 0, 0, 0 },
            { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1.0f }
        ),
        UIImageComponent(
            "UI_FADE_OP",
            1.0f,
            true,
            { 1,1,1,1 }
        )
    );
}

// -------------------------------------------------
//                フェード生成
// -------------------------------------------------
void OpeningControlSystem::CreateFadeEntity(float alpha)
{
    if (s_fadeEntity != ECS::INVALID_ENTITY_ID)
        return;

    s_fadeEntity = m_coordinator->CreateEntity(
        TransformComponent(
            { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f },
            { 0, 0, 0 },
            { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1.0f }
        ),
        UIImageComponent(
            "FADE_WHITE",
            100.0f,
            true,
            { 0, 0, 0, alpha }
        ),
        TagComponent("OPENING_FADE")
    );
}

void OpeningControlSystem::CreateNextPromptImages()
{
    if (s_nextPromptVisible)
        return;

    constexpr float BASE_X = 1210.0f;
    constexpr float BASE_Y = 690.0f;
    constexpr float OFFSET_X = 30.0f;
    constexpr float SCALE_X = 40.0f;
    constexpr float SCALE_Y = 40.0f;
    const char* NEXT_TEXTURES[3] =
    {
        "OP_TXT_TSUGI",
        "OP_TXT_HE",
        "OP_BUTTON"
    };

    for (int i = 0; i < 3; i++)
    {
        ECS::EntityID opnext = m_coordinator->CreateEntity(
            TransformComponent(
                {
                    BASE_X + (i - 1) * OFFSET_X,
                    BASE_Y,
                    0.0f
                },
                { 0,0,0 },
                { SCALE_X, SCALE_Y, 1.0f }
            ),
            UIImageComponent(
                NEXT_TEXTURES[i],
                10.0f,
                true,
                { 1,1,1,1 }
            ),
            TagComponent("OPENING_NEXT")
        );

        s_nextPromptEntities.push_back(opnext);
    }

    s_nextPromptVisible = true;
}

// -------------------------------------------------
//                     Update
// -------------------------------------------------
void OpeningControlSystem::Update(float deltaTime)
{
    if (!m_active)
        return;

    // ==============================
    // フェード処理
    // ==============================
    if (s_fadeState != FadeState::None)
    {
        s_fadeTimer += deltaTime;
        float t = s_fadeTimer / FADE_TIME;
        if (t > 1.0f) t = 1.0f;

        auto& fadeImg =
            m_coordinator->GetComponent<UIImageComponent>(s_fadeEntity);

        if (s_fadeState == FadeState::FadeOut)
        {
            fadeImg.color.w = t;

            if (t >= 1.0f)
            {
                // 文字削除
                for (auto e : s_openingTextEntities)
                    m_coordinator->DestroyEntity(e);
                s_openingTextEntities.clear();

                // 次へ画像削除
                for (auto e : s_nextPromptEntities)
                    m_coordinator->DestroyEntity(e);

                s_nextPromptEntities.clear();
                s_nextPromptVisible = false;

                //"次へ"の遅延状態リセット
                s_waitNextPrompt = false;
                s_nextPromptDelayTimer = 0.0f;

                //  進む or 戻る
                if (s_moveBackward)
                    s_blockIndex--;
                else
                    s_blockIndex++;

                if (s_blockIndex >= 0 && s_blockIndex < BLOCK_COUNT)
                {
                    ChangeBackground(s_blockIndex);

                    s_timer = 0.0f;
                    s_charIndex = 0;
                    s_finished = false;

                    s_fadeState = FadeState::FadeIn;
                    s_fadeTimer = 0.0f;
                }
                else
                {
                    // 最後まで行ったときだけシーン遷移
                    if (!s_moveBackward)
                        SceneManager::ChangeScene<StageSelectScene>();
                }

                s_moveBackward = false;
            }
        }
        else // FadeIn
        {
            fadeImg.color.w = 1.0f - t;

            if (t >= 1.0f)
            {
                m_coordinator->DestroyEntity(s_fadeEntity);
                s_fadeEntity = ECS::INVALID_ENTITY_ID;
                s_fadeState = FadeState::None;
            }
        }
        return;
    }

    // ==============================
    // 「次へ」表示の遅延処理
    // ==============================
    if (s_waitNextPrompt && !s_nextPromptVisible)
    {
        s_nextPromptDelayTimer += deltaTime;

        if (s_nextPromptDelayTimer >= NEXT_PROMPT_DELAY)
        {
            CreateNextPromptImages();
            s_waitNextPrompt = false;
        }
    }


   //文字、背景 1,2,3枚目切り替え
    if (s_finished)
    {
        if (IsKeyTrigger(VK_RETURN) || IsButtonTriggered(BUTTON_A))
        {
            ECS::EntityFactory::CreateOneShotSoundEntity(
                m_coordinator,
                "SE_DECISION",  // SE_DECIDE
                1.0f         //   
            );
            CreateFadeEntity(0.0f);
            s_fadeState = FadeState::FadeOut;
            s_fadeTimer = 0.0f;
        }
        else if ((IsKeyTrigger(VK_BACK)) || IsButtonTriggered(BUTTON_B))
        {
            if (s_blockIndex > 0)
            {
                ECS::EntityFactory::CreateOneShotSoundEntity(
                    m_coordinator,
                    "SE_BACK",  // SE_BACK
                    0.8f         //     
                );
                s_moveBackward = true;
                CreateFadeEntity(0.0f);
                s_fadeState = FadeState::FadeOut;
                s_fadeTimer = 0.0f;
            }
        }

        // 「次へ」ボタン アニメーション
        if (s_nextPromptVisible && s_nextPromptEntities.size() >= 3)
        {
            s_promptAnimTime += deltaTime;

            // sin波で拡大縮小（0.9 ～ 1.1）
            float scale = 1.0f + sinf(s_promptAnimTime * 4.0f) * 0.15f;

            ECS::EntityID buttonEntity = s_nextPromptEntities[2];

            auto& transform =
                m_coordinator->GetComponent<TransformComponent>(buttonEntity);

            transform.scale.x = 40.0f * scale;
            transform.scale.y = 40.0f * scale;
        }

        return;
    }

    // ==============================
    // 文字表示
    // ==============================
    s_timer += deltaTime;

    const OpeningTextBlock& block =
        OPENING_TEXT_BLOCKS[s_blockIndex];

    if (s_charIndex < block.charCount &&
        s_timer >= CHAR_INTERVAL * s_charIndex)
    {
        const auto& data = block.chars[s_charIndex];

        float x = START_X + CHAR_OFFSET_X * s_charIndex + data.offset.x;
        float y = START_Y + data.offset.y;

        ECS::EntityID entity = m_coordinator->CreateEntity(
            TransformComponent(
                { x, y, 0.0f },
                { 0, 0, 0 },
                { 500.0f, 500.0f, 1.0f }
            ),
            UIImageComponent(
                data.texture,
                1.0f,
                true,
                { 1,1,1,1 }
            ),
            TagComponent("OPENING_TEXT")
        );

        s_openingTextEntities.push_back(entity);

        s_charIndex++;

        if (s_charIndex >= block.charCount)
        {
            s_finished = true;

            // 次へ表示待ち状態に入る
            s_waitNextPrompt = true;
            s_nextPromptDelayTimer = 0.0f;
        }

    }
}