#ifndef ___UI_ANIMATION_COMPONENT___
#define ___UI_ANIMATION_COMPONENT___

#include <string>
#include <DirectXMath.h>
#include "ECS/ComponentRegistry.h"

struct UIAnimationComponent
{
    // アニメーションの種類（位置移動か、スケール拡大か）
    enum class AnimType { Move, Scale };
    AnimType type;

    // 開始値と終了値（Vector3）
    DirectX::XMFLOAT3 startValue;
    DirectX::XMFLOAT3 endValue;

    // タイミング設定
    float delayTime;      // 何秒待ってから開始するか (例: 0.4)
    float duration;       // 何秒かけてアニメーションするか (例: 0.5)

    // 内部経過時間カウンタ
    float currentTime = 0.0f;

    // 完了したかどうか
    bool isFinished = false;
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(UIAnimationComponent)

#endif // !___UI_ANIMATION_COMPONENT___