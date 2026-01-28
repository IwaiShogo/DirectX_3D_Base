/*****************************************************************//**
 * @file    FloatingSystem.h
 * @brief   FloatingComponentを持つエンティティをリッチに浮遊・回転させるシステム
 *********************************************************************/
#pragma once

#include "ECS/Coordinator.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/FloatingComponent.h"
#include <cmath>

 // 定数定義（必要に応じてヘッダー等に移動してください）
#ifndef PI
#define PI 3.1415926535f
#endif
#ifndef TWO_PI
#define TWO_PI (PI * 2.0f)
#endif

class FloatingSystem : public ECS::System
{
public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    void Update(float deltaTime)
    {
        // 管理下の全エンティティ（FloatingComponent + TransformComponent持ち）をループ
        for (auto const& entity : m_entities)
        {
            auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
            auto& floating = m_coordinator->GetComponent<FloatingComponent>(entity);

            // 時間を進める
            floating.time += deltaTime;

            // ---------------------------------------------------------
            // 1. 位置の計算 (初期位置より下がらない設定)
            // ---------------------------------------------------------
            // sin波 (-1.0 ~ 1.0)
            float sineWave = std::sin(floating.time * floating.speed);

            // 0.0 ~ 1.0 に正規化 (初期位置 initialY を底辺として浮き上がる)
            float normalizedWave = (sineWave + 1.0f) * 0.5f;

            // 上下位置更新
            transform.position.y = floating.initialY + (normalizedWave * floating.amplitude);

            // ---------------------------------------------------------
            // 2. 回転演出 (弧度法: Radian)
            // ---------------------------------------------------------
            // Y軸回転: 1秒間に約1ラジアン（約57度）回転
            // 度数法での45度/秒は ラジアンで PI/4 (約0.785f)
            float rotationSpeed = 1.0f;

            transform.rotation.y += rotationSpeed * deltaTime;

            // 2π (1周) を超えたらリセット
            if (transform.rotation.y > TWO_PI)
            {
                transform.rotation.y -= TWO_PI;
            }

            // Z軸の揺れ（浮遊感）
            // 度数法での10度は ラジアンで約 0.174f
            float tiltMax = 0.174f;
            transform.rotation.z = sineWave * tiltMax;

            // ---------------------------------------------------------
            // 3. スケール演出 (モデル側のスケール操作)
            // ---------------------------------------------------------
            // 呼吸するように伸縮させる (Squash & Stretch)
            // Y軸（縦）が伸びるときは、X/Z軸（横）が縮むと体積が保たれているように見えます

            float scaleBase = 1.0f;     // 基本スケール
            float breathAmount = 0.05f; // 変形量（5%程度）

            // sineWaveが正(浮く)のとき縦に伸び、負(沈む)のとき潰れるなど
            transform.scale.y = scaleBase + (sineWave * breathAmount);

            // 逆相で横幅を変える
            float inverseScale = scaleBase - (sineWave * breathAmount * 0.5f);
            transform.scale.x = inverseScale;
            transform.scale.z = inverseScale;
        }
    }

private:
    ECS::Coordinator* m_coordinator = nullptr;
};