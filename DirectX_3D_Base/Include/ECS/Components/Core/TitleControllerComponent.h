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

    //タイトルロゴ用
    float TitlelogoFadeTimer;    // ロゴのフェード用タイマー
    float TitlelogoFadeDuration; // ロゴのフェードにかける時間

    //カード
    bool isCardEnabled; // trueでカード表示、falseで非表示
    DirectX::XMFLOAT3 cardOriginalScale; //カード元のスケール保存
    DirectX::XMFLOAT3 cardOriginalRotation; //カード元の回転保存
    //UIアニメーション用
    float uiAnimTimer;                //UIアニメーション経過時間
    float uiAnimDuration;             //アニメーションにかかる時間
    std::vector<float> menuTargetYs;  //各ボタン目的Y座標 

    //回転角度0
    float startRotY;
    float endRotY;

    // 制御対象のエンティティID
    ECS::EntityID cameraEntityID;
    ECS::EntityID logoEntityID;
    ECS::EntityID cardEntityID;
    std::vector<ECS::EntityID> pressStartUIEntities;
    std::vector<ECS::EntityID> menuUIEntities;

    // カメラ演出用データ
    DirectX::XMFLOAT3 camStartPos;   //始点
    DirectX::XMFLOAT3 camEndPos;     //終点
    DirectX::XMFLOAT3 camControlPos; //基準点1
    float startRotZ; // 角度
    float endRotZ;   // 角度

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
//既存のマクロを使用
REGISTER_COMPONENT_TYPE(TitleControllerComponent)

#endif //!___TitleControllerComponent_H___