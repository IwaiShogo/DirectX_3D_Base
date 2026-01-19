#ifndef ___TitleControllerComponent_H___
#define ___TitleControllerComponent_H___

#include "ECS/Types.h"
#include <DirectXMath.h>
#include <vector>

enum class TitleState
{
    WaitInput,
    ZoomAnimation,
    ModeSelect
};

struct TitleControllerComponent
{
    TitleState state;
    float animTimer;
    float animDuration;

    float TitlelogoFadeTimer;
    float TitlelogoFadeDuration;

    bool isCardEnabled;
    DirectX::XMFLOAT3 cardOriginalScale;
    DirectX::XMFLOAT3 cardOriginalRotation;

    float uiAnimTimer;
    float uiAnimDuration;
    std::vector<float> menuTargetYs;

    // --- エフェクト関連 ---
    std::vector<ECS::EntityID> buttonEffectEntities;
    bool effectTriggered = false;

    // --- カメラ演出用 ---
    ECS::EntityID cameraEntityID;
    ECS::EntityID logoEntityID;
    ECS::EntityID cardEntityID;
    ECS::EntityID TitlekaigaEntityID;
    std::vector<ECS::EntityID> pressStartUIEntities;
    std::vector<ECS::EntityID> menuUIEntities;

    DirectX::XMFLOAT3 camStartPos;
    DirectX::XMFLOAT3 camEndPos;
    DirectX::XMFLOAT3 camControlPos;
    float startRotY;
    float endRotY;
    float startRotZ;
    float endRotZ;

    float lampSpawnTimer = 0.0f;
    struct ActiveLamp {
        ECS::EntityID id;
        float lifeTimer;
    };
    std::vector<ActiveLamp> activeLamps; // 寿命管理用
    TitleControllerComponent()
        : state(TitleState::WaitInput)
        , animTimer(0.0f)
        , animDuration(1.0f)
        , uiAnimTimer(0.0f)
        , uiAnimDuration(0.8f)
        , cameraEntityID(ECS::INVALID_ENTITY_ID)
        , logoEntityID(ECS::INVALID_ENTITY_ID)
        , cardEntityID(ECS::INVALID_ENTITY_ID)
        , camStartPos{ 0,0,0 }
        , camEndPos{ 0,0,0 }
        , camControlPos{ 0,0,0 }
        , startRotY(0.0f)
        , endRotY(0.0f)
        , TitlelogoFadeTimer(0.0f)
        , TitlelogoFadeDuration(1.0f)
        , cardOriginalScale{ 1.0f, 1.0f, 1.0f }
        , cardOriginalRotation{ 0.0f, 0.0f, 0.0f }
        , isCardEnabled(true)
        , startRotZ(0.0f)
        , endRotZ(0.0f)
    {
    }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TitleControllerComponent)

#endif