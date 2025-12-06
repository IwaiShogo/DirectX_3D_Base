#ifndef ___TitleControllerComponent_H___
#define ___TitleControllerComponent_H___

#include "ECS/Types.h"
#include <DirectXMath.h>
#include <vector>

// タイトル画面の状態定義
enum class TitleState
{
    WaitInput,      // 入力待ち
    ZoomAnimation,  // ズーム演出中
    ModeSelect      // モード選択中
};

struct TitleControllerComponent
{
    TitleState state;
    float animTimer;
    float animDuration;

    // 制御対象のエンティティID
    ECS::EntityID cameraEntityID;
    std::vector<ECS::EntityID> pressStartUIEntities;
    std::vector<ECS::EntityID> menuUIEntities;

    // カメラ演出用データ
    DirectX::XMFLOAT3 camStartPos;
    DirectX::XMFLOAT3 camEndPos;

    TitleControllerComponent()
        : state(TitleState::WaitInput)
        , animTimer(0.0f)
        , animDuration(1.0f)
        , cameraEntityID(ECS::INVALID_ENTITY_ID)
        , camStartPos{ 0,0,0 }, camEndPos{ 0,0,0 }
    {
    }
};

#include "ECS/ComponentRegistry.h"
//既存のマクロを使用
REGISTER_COMPONENT_TYPE(TitleControllerComponent)

#endif //!___TitleControllerComponent_H___