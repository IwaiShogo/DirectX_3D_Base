/*****************************************************************//**
 * @file	PointLightComponent.h
 * @brief	点光源
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

#ifndef ___POINT_LIGHT_COMPONENT_H___
#define ___POINT_LIGHT_COMPONENT_H___

#include <DirectXMath.h>

struct PointLightComponent
{
    DirectX::XMFLOAT4 color; // 色 (R, G, B, 1.0)
    float range;             // 光の届く距離
    bool isActive;           // 点灯/消灯フラグ
    DirectX::XMFLOAT3 offset;   // 中心からのズレ

    // コンストラクタ
    PointLightComponent(float r = 1.0f, float g = 1.0f, float b = 1.0f, float rng = 10.0f, DirectX::XMFLOAT3 off = {0.0f, 0.0f, 0.0f})
        : color(r, g, b, 1.0f), range(rng), isActive(true), offset(off) {
    }
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(PointLightComponent)

#endif // !___POINT_LIGHT_COMPONENT_H___