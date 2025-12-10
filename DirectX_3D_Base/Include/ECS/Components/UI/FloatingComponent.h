/*****************************************************************//**
 * @file    FloatingComponent.h
 * @brief   オブジェクトをその場で上下に浮遊させるためのコンポーネント
 *********************************************************************/
#ifndef ___FLOATING_COMPONENT_H__
#define ___FLOATING_COMPONENT_H__

#include <DirectXMath.h>

struct FloatingComponent
{
    float amplitude;    // 振幅（上下にどれくらい動くか）
    float speed;        // 速さ（点滅や揺れのサイクル速度）
    float time;         // 経過時間タイマー
    float initialY;     // 基準となるY座標（この高さを中心に揺れる）

    /**
     * @brief コンストラクタ
     * @param _amplitude 揺れ幅（例: 0.5f なら上下0.5ずつ動く）
     * @param _speed     揺れる速さ
     * @param _initialY  基準の高さ（生成時のY座標を入れる）
     */
    FloatingComponent(float _amplitude = 0.5f, float _speed = 2.0f, float _initialY = 0.0f)
        : amplitude(_amplitude)
        , speed(_speed)
        , time(0.0f)
        , initialY(_initialY)
    {
    }
};

// コンポーネント登録用のマクロ（ECSの仕様に合わせて記述）
// ※ComponentRegistry.hの場所はプロジェクト構成に合わせて調整してください
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(FloatingComponent)
#endif // !___FLOATING_COMPONENT_H__