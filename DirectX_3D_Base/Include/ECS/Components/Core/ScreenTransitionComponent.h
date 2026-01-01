#pragma once

#include <DirectXMath.h>
#include "ECS/Types.h"
#include "Main.h"

namespace ECS
{
    struct ScreenTransitionComponent
    {
        enum class Phase
        {
            Idle,
            DelayBeforeFadeOut,
            FadingOut,
            HoldBlack,
            FadingIn
        };

        Phase phase = Phase::Idle;

        float delaySec = 0.0f;
        float durationSec = 1.0f; // legacy (fallback)

        // New: allow different timing/scale behavior for fade-out and fade-in
        float durationOutSec = 0.35f;
        float durationInSec = 0.45f;
        bool  scaleFadeOut = true;
        bool  scaleFadeIn = false;

        float timerSec = 0.0f;
        float alpha = 0.0f;
        float angleDeg = -45.0f;
        float startScaleMul = 0.32f;
        float coverScaleMul = 1.0f;

        bool autoFadeIn = true;
        bool firedOnBlack = false;
        bool firedOnFinished = false;
    };
}

#include "ECS/ComponentRegistry.h"

// dvFECSOԓœo^i C2065 ~܂j
namespace ECS
{
    REGISTER_COMPONENT_TYPE(ScreenTransitionComponent)
}
