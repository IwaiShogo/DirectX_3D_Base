/*****************************************************************//**
 * @file	FlickerComponent.h
 * @brief	点滅のパターン
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/13	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___FLICKER_COMPONENT_H___
#define ___FLICKER_COMPONENT_H___

#include <DirectXMath.h>

struct FlickerComponent
{
    float timer;          // 時間計測用
    float flickerSpeed;   // 点滅の速さ
    float minIntensity;   // 一番暗い時の明るさ (0.0~1.0)
    float maxIntensity;   // 一番明るい時の明るさ (0.0~1.0)

    // 乱数で不規則にするためのオフセット
    float randomOffset;

    DirectX::XMFLOAT4 baseColor; // 電球の基本色（黄色など）

    FlickerComponent(DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 0.5f, 1.0f })
        : timer(0.0f)
        , flickerSpeed(10.0f) // チカチカする速さ
        , minIntensity(0.1f)  // 消灯時の明るさ
        , maxIntensity(2.0f)  // 点灯時の明るさ(1.0超えで発光表現)
        , randomOffset(0.0f)
        , baseColor(color)
    {
        randomOffset = (float)(rand() % 100) / 10.0f;
    }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(FlickerComponent)

#endif