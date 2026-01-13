/*****************************************************************//**
 * @file	FlickerSystem.cpp
 * @brief	
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

#include "ECS/Systems/Rendering/FlickerSystem.h" // パスは環境に合わせて
#include <cmath>
#include <algorithm>

using namespace DirectX;

void FlickerSystem::Update(float deltaTime)
{
    for (auto const& entity : m_entities)
    {
        auto& flicker = m_coordinator->GetComponent<FlickerComponent>(entity);
        auto& render = m_coordinator->GetComponent<RenderComponent>(entity);

        flicker.timer += deltaTime;

        // サイン波 + ノイズ で「故障しかけの蛍光灯」のようなチカチカを作る
        // sin(時間 * 速さ) で周期的に明滅
        float wave = sinf((flicker.timer + flicker.randomOffset) * flicker.flickerSpeed);

        // さらにランダムなノイズを混ぜる
        float noise = ((float)(rand() % 100) / 100.0f) * 0.5f;

        // 明るさを決定
        float intensity = (wave + 1.0f) * 0.5f;

        // ノイズでたまに消えたりついたりさせる
        if (noise > 0.4f) intensity = 0.0f;

        // 最小～最大の間で調整
        float finalIntensity = flicker.minIntensity + (flicker.maxIntensity - flicker.minIntensity) * intensity;

        // RenderComponentの色を書き換える
        // (RenderSystemで描画されるときに、この色が反映される)
        render.color.x = flicker.baseColor.x * finalIntensity;
        render.color.y = flicker.baseColor.y * finalIntensity;
        render.color.z = flicker.baseColor.z * finalIntensity;
        render.color.w = 1.0f; // アルファは不透明
    }
}