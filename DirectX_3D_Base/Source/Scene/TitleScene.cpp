/*****************************************************************//**
 * @file	TitleScene.cpp
 * @brief   タイトルシーン：3Dカード静止配置版
 *********************************************************************/

#include "Scene/TitleScene.h"
#include "Scene/StageSelectScene.h"
#include "ECS/ECSInitializer.h"
#include "DirectXMath.h"
#include <iostream>
#include "Scene/StageUnlockProgress.h"
#include <ECS/Components/Core/TransformComponent.h>
#include <ECS/Components/UI/UIImageComponent.h>
#include <ECS/Components/UI/UIButtonComponent.h>
#include <ECS/Systems/UI/UIInputSystem.h>
#include <ECS/Systems/Rendering/RenderSystem.h>
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Core/TitleControlSystem.h"
#include "ECS/Systems/Core/ScreenTransition.h"
#include "ECS/Components/Rendering/RenderComponent.h"
#include "ECS/Components/Core/SoundComponent.h"
using namespace DirectX;

namespace TitleLayout
{
    constexpr float ZOOM_DURATION = 1.2f;
    constexpr float MENU_SLIDE_DURATION = 0.3f;

    constexpr float LOGO_Y_RATIO = 0.3f;
    constexpr float BTN_Y_NEWGAME = 0.35f;
    constexpr float BTN_Y_CONTINUE = 0.65f;
    constexpr float PRESS_START_Y_RATIO = 0.8f;

    const XMFLOAT3 CARD_3D_SCALE = { 0.5f, 0.5f, 0.5f };
    const XMFLOAT3 BTN_BASE_SCALE = { 300.0f, 140.0f, 1.0f };
    const XMFLOAT3 LOGO_BASE_SCALE = { 550.0f, 410.0f, 1.0f };
    const XMFLOAT3 START_BTN_SCALE = { 450.0f, 150.0f, 1.0f };
    const XMFLOAT3 TITLE_KAIGA_SCALE = { 1280.0f, 720.0f, 1.0f };

    constexpr float CARD_STATIC_ROT_Z_DEG = 20.0f;
}

void TitleScene::Init()
{
    m_coordinator = std::make_shared<ECS::Coordinator>();
    ECS::ECSInitializer::InitECS(m_coordinator);

    StageUnlockProgress::Load();

    TitleControllerComponent titleCtrl;
    titleCtrl.camStartPos = XMFLOAT3(0.0f, 2.5f, -9.8f);
    titleCtrl.camEndPos = XMFLOAT3(0.0f, 1.4f, -4.2f);
    titleCtrl.camControlPos = XMFLOAT3{ 3.5f,1.8f, -11.0f };
    titleCtrl.animDuration = TitleLayout::ZOOM_DURATION;
    titleCtrl.uiAnimDuration = TitleLayout::MENU_SLIDE_DURATION;
    titleCtrl.startRotY = XMConvertToRadians(-90.0f);
    titleCtrl.endRotY = XMConvertToRadians(0.0f);
    std::srand(static_cast<unsigned int>(time(NULL)));
    // --- カメラ生成 ---
    ECS::EntityID cam = ECS::EntityFactory::CreateBasicCamera(m_coordinator.get(), titleCtrl.camStartPos);
    titleCtrl.cameraEntityID = cam;
    if (m_coordinator->HasComponent<TransformComponent>(cam)) {
        m_coordinator->GetComponent<TransformComponent>(cam).rotation.y = titleCtrl.startRotY;
    }

    // --- 1. 背景・カードの生成 ---
    // 美術館背景
    m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */{ 0.0f, 0.0f, 0.0f },
            /* Rotation */{ 0.0f, 0.0f, 0.0f },
            /* Scale    */{ 1.0f, 1.0f, 1.0f }
        ),
        RenderComponent(
            /* Type  */ MESH_MODEL,
            /* Color */{ 1.0f, 1.0f, 1.0f, 1.0f }
        ),
        ModelComponent(
            /* AssetID */ "M_TITLE_MUSEUM",
            /* Scale   */ 0.1f,
            /* Flags   */ Model::ZFlip
        ),
        EffectComponent(
            /* AssetID  */ "EFK_TITLE_SHINE",
            /* Loop     */ false,
            /* AutoPlay */ false,
            /* Offset   */{ 0.0f, 0.0f, -3.0f },
            /* Scale    */ 0.3f
        )
    );

    XMFLOAT3 finalCardPos = { 0.0f,1.4f,-3.5f };
    titleCtrl.cardEndPos = finalCardPos;
    titleCtrl.cardStartPos = { -1.0f, 2.5f, -12.8f };

    titleCtrl.cardEndScale = TitleLayout::CARD_3D_SCALE;
    titleCtrl.cardStartScale = {
        titleCtrl.cardEndScale.x * 0.25f,
        titleCtrl.cardEndScale.y * 0.25f,
        titleCtrl.cardEndScale.z * 0.25f
    };

    // タイトルカード
    titleCtrl.cardEntityID = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */titleCtrl.cardStartPos,
            /* Rotation */{ 0.0f,0.0f, XMConvertToRadians(45.0f) },
            /* Scale    */ titleCtrl.cardStartScale
        ),
        RenderComponent(
            /* Type  */ MESH_MODEL,
            /* Color */{ 1.0f, 1.0f, 1.0f, 1.0f }
        ),
        ModelComponent(
            /* AssetID */ "M_TITLE_CARD",
            /* Scale   */ 0.1f,
            /* Flags   */ Model::ZFlip
        )
    );


    // ガラスケース
    m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */{ 0.0f, 0.0f, 0.0f },
            /* Rotation */{ 0.0f, 0.0f, 0.0f },
            /* Scale    */{ 1.0f, 1.0f, 1.0f }
        ),
        RenderComponent(
            /* Type  */ MESH_MODEL,
            /* Color */{ 1.0f, 1.0f, 1.0f, 1.0f }
        ),
        ModelComponent(
            /* AssetID */ "M_TITLE_GLASSCASE",
            /* Scale   */ 0.1f,
            /* Flags   */ Model::ZFlip
        )
    );

    // --- 2. UIの生成 ---
    // ロゴ
    titleCtrl.logoEntityID = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * TitleLayout::LOGO_Y_RATIO, 0.0f },
            /* Rotation */{ 0.0f, 0.0f, 0.0f },
            /* Scale    */ TitleLayout::LOGO_BASE_SCALE
        ),
        UIImageComponent(
            /* AssetID */ "UI_TITLE_LOGO",
            /* Depth   */ 0.5f,
            /* Visible */ true,
            /* Color   */{ 1.0f, 1.0f, 1.0f, 0.0f }
        )
    );

    // Press Start
    ECS::EntityID pressStart = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * TitleLayout::PRESS_START_Y_RATIO, 0.0f },
            /* Rotation */{ 1.0f, 0.0f, 0.0f },
            /* Scale    */ TitleLayout::START_BTN_SCALE
        ),
        UIImageComponent(
            /* AssetID */ "UI_PRESS_START",
            /* Depth   */ 0.0f,
            /* Visible */ true,
            /* Color   */{ 1.0f, 1.0f, 1.0f, 0.0f }
        )
    );
    titleCtrl.pressStartUIEntities.push_back(pressStart);

    // --- 3. メニューUI ---
    {
        float targetY_NewGame = SCREEN_HEIGHT * TitleLayout::BTN_Y_NEWGAME;
        float targetY_Continue = SCREEN_HEIGHT * TitleLayout::BTN_Y_CONTINUE;
        const XMFLOAT3 hitScale = { TitleLayout::BTN_BASE_SCALE.x * 0.66f, TitleLayout::BTN_BASE_SCALE.y * 0.66f, 1.0f };
        const XMFLOAT3 menuRotation = { 0.0f, 0.0f, XMConvertToRadians(-20.0f) };

        // New Game ボタン
        ECS::EntityID newGame = m_coordinator->CreateEntity(
            TransformComponent(
                /* Position */{ SCREEN_WIDTH * 0.5f - 60.0f, targetY_NewGame, 0.0f },
                /* Rotation */ menuRotation,
                /* Scale    */ TitleLayout::BTN_BASE_SCALE
            ),
            UIImageComponent(
                /* AssetID */ "BTN_NEW_GAME",
                /* Depth   */ 0.5f,
                /* Visible */ true,
                /* Color   */{ 1.0f, 1.0f, 1.0f, 0.0f }
            ),
            UIButtonComponent(
                /* State    */ ButtonState::Normal,
                /* Selected */ false,
                /* Callback */ [this]() { 
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_DECISION", 0.5f);
                    for (auto const& entity : m_coordinator->GetActiveEntities()) {
                        if (m_coordinator->HasComponent<SoundComponent>(entity)) {
                            auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
                            sound.RequestStop();
                        }
                    }
                    
                    SceneManager::ChangeScene<OpeningScene>(); },//StageSelectScene
                /* HitScale */ hitScale
            )
        );

        // Continue ボタン
        ECS::EntityID cont = m_coordinator->CreateEntity(
            TransformComponent(
                /* Position */{ SCREEN_WIDTH * 0.5f + 60.0f , targetY_Continue - 10.0f, 0.0f },
                /* Rotation */ menuRotation,
                /* Scale    */ TitleLayout::BTN_BASE_SCALE
            ),
            UIImageComponent(
                /* AssetID */ "BTN_CONTINUE",
                /* Depth   */ 0.5f,
                /* Visible */ true,
                /* Color   */{ 1.0f, 1.0f, 1.0f, 0.0f }
            ),
            UIButtonComponent(
                /* State    */ ButtonState::Normal,
                /* Selected */ false,
                /* Callback */ [this]() {
                    ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_DECISION", 0.5f);
                    for (auto const& entity : m_coordinator->GetActiveEntities()) {
                        if (m_coordinator->HasComponent<SoundComponent>(entity)) {
                            auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
                            sound.RequestStop();
                        }
                    }
                    SceneManager::ChangeScene<StageSelectScene>(); },
                /* HitScale */ hitScale
            )
        );

        titleCtrl.menuUIEntities.push_back(newGame);
        titleCtrl.menuUIEntities.push_back(cont);
        titleCtrl.menuTargetYs.push_back(targetY_NewGame);
        titleCtrl.menuTargetYs.push_back(targetY_Continue);
    }

    // --- システムコントローラー ---
    ECS::EntityID controller = m_coordinator->CreateEntity(TitleControllerComponent());
    m_coordinator->GetComponent<TitleControllerComponent>(controller) = titleCtrl;

    // --- カーソル ---
    m_coordinator->CreateEntity(
        TransformComponent(
            /* Position */{ 0.0f, 0.0f, 0.0f },
            /* Rotation */{ 0.0f, 0.0f, 0.0f },
            /* Scale    */{ 64.0f, 64.0f, 1.0f }
        ),
        UIImageComponent(
            /* AssetID  */ "ICO_CURSOR",
            /* Depth    */ 1.0f
        ),
        UICursorComponent()
    );

   
	// --- BGM再生 ---
    ECS::EntityFactory::CreateLoopSoundEntity(m_coordinator.get(), "BGM_TITLE", 0.5f);
    std::cout << "TitleScene::Init() - Layout Completed with Commented Parameters." << std::endl;
}
void TitleScene::Uninit()
{
    for (auto const& entity : m_coordinator->GetActiveEntities()) {
        if (!m_coordinator->HasComponent<SoundComponent>(entity)) continue;
        auto& sound = m_coordinator->GetComponent<SoundComponent>(entity);
        sound.RequestStop();
    }
    if (auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>()) effectSystem->Uninit();
    ECS::ECSInitializer::UninitECS();
    m_coordinator.reset();
}

void TitleScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);
}

void TitleScene::Draw()
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