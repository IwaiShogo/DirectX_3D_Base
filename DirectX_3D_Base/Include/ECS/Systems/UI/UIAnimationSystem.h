#ifndef __UI_ANIMATION_SYSTEM_H__
#define __UI_ANIMATION_SYSTEM_H__

#include "ECS/SystemManager.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/UI/UIAnimationComponent.h" // さっき作ったデータをインクルード
#include "ECS/Components/Core/TransformComponent.h" // 位置操作

namespace ECS {

    class UIAnimationSystem : public System
    {
    public:
        // 初期化関数
        void Init(Coordinator* coordinator);

        // 毎フレーム呼ばれる更新関数
        void Update(float deltaTime);

    private:
        // ECS操作用のポインタ
        Coordinator* m_coordinator;

        // 計算用ヘルパー関数（クラスの中に隠蔽する）
        float Lerp(float start, float end, float t);
        DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float t);
        float Clamp01(float value);
    };
}
#endif // __UI_ANIMATION_SYSTEM_H__