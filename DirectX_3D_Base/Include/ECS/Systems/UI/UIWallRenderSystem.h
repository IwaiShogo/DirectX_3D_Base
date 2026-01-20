/* UIWallRenderSystem.h */

#pragma once

// ===== インクルード =====
#include "ECS/ECS.h"
#include "ECS/Coordinator.h" 
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/UIWallComponent.h"
// CameraComponentの定義も必要です（ビュー行列取得のため）
#include "ECS/Components/Core/CameraComponent.h"

namespace ECS
{
    /**
     * @class UIWallRenderSystem
     * @brief 壁にアタッチされたUIを描画するシステム
     */
    class UIWallRenderSystem : public System
    {
    public:
    private:
        Coordinator* m_coordinator = nullptr;

    public:
        UIWallRenderSystem() = default;

        void Init(Coordinator* coordinator) override { m_coordinator = coordinator; }

        /**
         * @brief 描画処理のメインループ。深度に基づいてソートしてから描画します。
         */
       // void Draw();
        void Draw(float deltaTime);
        // RunからDrawに変更
    };
}
