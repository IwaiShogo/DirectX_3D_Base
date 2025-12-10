/*****************************************************************//**
 * @file    RotatorComponent.h
 * @brief   エンティティを継続的に回転させるコンポーネント
 *********************************************************************/
#pragma once

#include <DirectXMath.h>
#include "ECS/ComponentRegistry.h"

struct RotatorComponent
{
    DirectX::XMFLOAT3 rotationSpeed; // 各軸の回転速度 (ラジアン/秒)

    /**
     * @brief コンストラクタ
     * @param speedX X軸回転速度
     * @param speedY Y軸回転速度
     * @param speedZ Z軸回転速度
     */
    RotatorComponent(float speedX = 0.0f, float speedY = 0.0f, float speedZ = 0.0f)
        : rotationSpeed(speedX, speedY, speedZ)
    {
    }

    // Y軸だけ回したい場合の便利コンストラクタ
    RotatorComponent(float speedY)
        : rotationSpeed(0.0f, speedY, 0.0f)
    {
    }
};

REGISTER_COMPONENT_TYPE(RotatorComponent)