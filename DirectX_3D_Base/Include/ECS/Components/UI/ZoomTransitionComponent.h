#ifndef ___ZOOM_TRANSITION_COMPONENT_H___
#define ___ZOOM_TRANSITION_COMPONENT_H___

#include <string>
#include <DirectXMath.h>
#include "ECS/ComponentRegistry.h"

namespace ECS {

    enum class TransitionTarget {
        None,
        ToInfo,
        ToTitle,
        ToGame
    };

    struct ZoomTransitionComponent
    {
        // ★追加：起動スイッチ
        bool isActive = false;

        float zoomSpeed = 150.0f;
        float triggerScale = 20.0f;
        TransitionTarget nextScene = TransitionTarget::None;
        int targetStageNo = 0;
        bool isFinished = false;
    };

} // namespace ECS

namespace ECS {
    REGISTER_COMPONENT_TYPE(ZoomTransitionComponent)
}
#endif