#pragma once

#include "ECS/Coordinator.h"
#include "ECS/ECS.h"
// ˑ𖾎iECS/ECS.h ɗȂj
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/UIImageComponent.h"
#include "ECS/Components/Core/ScreenTransitionComponent.h"
#include "ECS/Systems/Core/ScreenTransitionSystem.h"

#include "Main.h"
#include <functional>

namespace ScreenTransition
{
    inline ECS::EntityID CreateOverlay(ECS::Coordinator* coordinator, const char* texId,
        float centerX, float centerY, float z, float drawOrder)
    {
        if (!coordinator) return (ECS::EntityID)-1;

        ECS::EntityID e = coordinator->CreateEntity(
            TransformComponent({ centerX, centerY, z }, { 0,0,0 }, { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1.0f }),
            // IMPORTANT: Keep RGB=1 so the texture (UI_STAGE_FADE) is not multiplied to black.
            UIImageComponent(texId, drawOrder, true, { 1,1,1,0 }),
            ECS::ScreenTransitionComponent()
        );

        auto& ui = coordinator->GetComponent<UIImageComponent>(e);
        ui.isVisible = false;
        ui.color = { 1,1,1,0 };
        return e;
    }

    // R[obN System ɓo^iComponentɎȂj
        // Extended API (recommended): separate out/in timing and control whether scale animates per phase.
    inline void RequestFadeOutEx(ECS::Coordinator* coordinator, ECS::EntityID overlay,
        float delaySec,
        float durationOutSec,
        float durationInSec,
        std::function<void()> onBlack,
        bool autoFadeIn,
        std::function<void()> onFinished = nullptr,
        float angleDeg = -60.0f,
        float startScaleMul = 0.32f,
        bool scaleFadeOut = true,
        bool scaleFadeIn = false)
    {
        if (!coordinator) return;
        if (overlay == (ECS::EntityID)-1) return;
        if (!coordinator->HasComponent<ECS::ScreenTransitionComponent>(overlay)) return;

        auto& st = coordinator->GetComponent<ECS::ScreenTransitionComponent>(overlay);
        st.phase = ECS::ScreenTransitionComponent::Phase::DelayBeforeFadeOut;
        st.delaySec = (delaySec < 0.0f) ? 0.0f : delaySec;

        st.durationOutSec = (durationOutSec <= 0.0f) ? 0.001f : durationOutSec;
        st.durationInSec = (durationInSec <= 0.0f) ? 0.001f : durationInSec;
        st.durationSec = st.durationOutSec; // legacy fallback

        st.scaleFadeOut = scaleFadeOut;
        st.scaleFadeIn = scaleFadeIn;

        st.timerSec = 0.0f;
        st.alpha = 0.0f;
        st.autoFadeIn = autoFadeIn;
        st.firedOnBlack = false;
        st.firedOnFinished = false;
        st.angleDeg = angleDeg;
        st.startScaleMul = startScaleMul;
        st.coverScaleMul = ECS::ScreenTransitionSystem::ComputeCoverScaleMul(angleDeg, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

        ECS::ScreenTransitionSystem::SetCallbacks(overlay, std::move(onBlack), std::move(onFinished));
    }

    inline void RequestFadeInEx(ECS::Coordinator* coordinator, ECS::EntityID overlay,
        float durationInSec,
        std::function<void()> onFinished = nullptr,
        float angleDeg = -60.0f,
        bool scaleFadeIn = false)
    {
        if (!coordinator) return;
        if (overlay == (ECS::EntityID)-1) return;
        if (!coordinator->HasComponent<ECS::ScreenTransitionComponent>(overlay)) return;

        auto& st = coordinator->GetComponent<ECS::ScreenTransitionComponent>(overlay);
        st.phase = ECS::ScreenTransitionComponent::Phase::FadingIn;
        st.delaySec = 0.0f;

        st.durationInSec = (durationInSec <= 0.0f) ? 0.001f : durationInSec;
        st.durationOutSec = st.durationInSec;
        st.durationSec = st.durationInSec; // legacy fallback

        st.scaleFadeIn = scaleFadeIn;
        st.scaleFadeOut = true;

        st.timerSec = 0.0f;
        st.alpha = 1.0f;
        st.autoFadeIn = true;
        st.firedOnBlack = true;
        st.firedOnFinished = false;
        st.angleDeg = angleDeg;
        st.coverScaleMul = ECS::ScreenTransitionSystem::ComputeCoverScaleMul(angleDeg, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

        ECS::ScreenTransitionSystem::SetCallbacks(overlay, std::function<void()>{}, std::move(onFinished));

        if (coordinator->HasComponent<UIImageComponent>(overlay))
        {
            auto& ui = coordinator->GetComponent<UIImageComponent>(overlay);
            ui.isVisible = true;
            ui.color.w = 1.0f;
        }
    }

    // Legacy APIs (kept for compatibility): out/in share the same duration and scale animates in both directions.
    inline void RequestFadeOut(ECS::Coordinator* coordinator, ECS::EntityID overlay,
        float delaySec, float durationSec,
        std::function<void()> onBlack,
        bool autoFadeIn,
        std::function<void()> onFinished = nullptr,
        float angleDeg = -60.0f,
        float startScaleMul = 0.32f)
    {
        RequestFadeOutEx(coordinator, overlay, delaySec, durationSec, durationSec,
            std::move(onBlack), autoFadeIn, std::move(onFinished), angleDeg, startScaleMul, true, true);
    }

    inline void RequestFadeIn(ECS::Coordinator* coordinator, ECS::EntityID overlay,
        float durationSec,
        std::function<void()> onFinished = nullptr,
        float angleDeg = -60.0f,
        float startScaleMul = 0.32f)
    {
        // startScaleMul is ignored in the extended fade-in mode; kept for signature compatibility.
        (void)startScaleMul;
        RequestFadeInEx(coordinator, overlay, durationSec, std::move(onFinished), angleDeg, true);
    }

    inline bool IsBusy(ECS::Coordinator* coordinator, ECS::EntityID overlay)
    {
        return ECS::ScreenTransitionSystem::IsBusy(coordinator, overlay);
    }
} // K