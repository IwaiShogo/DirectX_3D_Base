#include "ECS/Systems/Core/ScreenTransitionSystem.h"
#include "Main.h"
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

namespace
{
    inline float EaseInOut(float t)
    {
        t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
        return t * t * (3.0f - 2.0f * t);
    }

    // Snappier curves (used for "no-rotation" transitions)
    inline float EaseOutCubic(float t)
    {
        t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
        const float u = 1.0f - t;
        return 1.0f - (u * u * u);
    }

    inline float EaseInCubic(float t)
    {
        t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
        return t * t * t;
    }

    inline float Punch(float t, float amp)
    {
        // 0->1->0 (no overshoot at ends)
        const float s = std::sin(t * DirectX::XM_PI);
        return 1.0f + amp * s;
    }
}

namespace ECS
{
    // ǉFÓI}bv`
    std::unordered_map<EntityID, ScreenTransitionSystem::Callbacks> ScreenTransitionSystem::s_callbacks;

    void ScreenTransitionSystem::SetCallbacks(EntityID overlay,
        std::function<void()> onBlack,
        std::function<void()> onFinished)
    {
        Callbacks cb;
        cb.onBlack = std::move(onBlack);
        cb.onFinished = std::move(onFinished);
        s_callbacks[overlay] = std::move(cb);
    }

    void ScreenTransitionSystem::ClearCallbacks(EntityID overlay)
    {
        auto it = s_callbacks.find(overlay);
        if (it != s_callbacks.end()) s_callbacks.erase(it);
    }

    void ScreenTransitionSystem::Init(Coordinator* coordinator)
    {
        m_coordinator = coordinator;
    }

    void ScreenTransitionSystem::Uninit()
    {
        m_coordinator = nullptr;
    }

    float ScreenTransitionSystem::Clamp01(float x)
    {
        if (x < 0.0f) return 0.0f;
        if (x > 1.0f) return 1.0f;
        return x;
    }

    float ScreenTransitionSystem::SmoothStep01(float t)
    {
        t = Clamp01(t);
        return t * t * (3.0f - 2.0f * t);
    }

    bool ScreenTransitionSystem::IsBusy(Coordinator* coordinator, EntityID overlay)
    {
        if (!coordinator) return false;
        if (overlay == (EntityID)-1) return false;
        if (!coordinator->HasComponent<ScreenTransitionComponent>(overlay)) return false;
        const auto& st = coordinator->GetComponent<ScreenTransitionComponent>(overlay);
        return st.phase != ScreenTransitionComponent::Phase::Idle;
    }

    float ScreenTransitionSystem::ComputeCoverScaleMul(float angleDeg, float screenW, float screenH, float safety)
    {
        const float theta = XMConvertToRadians(angleDeg);
        const float c = std::fabs(std::cos(theta));
        const float s = std::fabs(std::sin(theta));

        const float sx = screenW * 0.5f;
        const float sy = screenH * 0.5f;

        const float needU = sx * c + sy * s;
        const float needV = sx * s + sy * c;

        const float a0 = sx;
        const float b0 = sy;

        float mulU = (a0 > 0.0f) ? (needU / a0) : 1.0f;
        float mulV = (b0 > 0.0f) ? (needV / b0) : 1.0f;

        float mul = (mulU > mulV) ? mulU : mulV;
        if (mul < 1.0f) mul = 1.0f;
        mul *= safety;
        return mul;
    }

    void ScreenTransitionSystem::Update(float dt)
    {
        if (!m_coordinator) return;
        for (auto entity : m_entities)
        {
            UpdateOne(entity, dt);
        }
    }

    void ScreenTransitionSystem::UpdateOne(EntityID e, float dt)
    {
        if (!m_coordinator) return;
        if (!m_coordinator->HasComponent<ScreenTransitionComponent>(e)) return;
        if (!m_coordinator->HasComponent<TransformComponent>(e)) return;
        if (!m_coordinator->HasComponent<UIImageComponent>(e)) return;

        auto& st = m_coordinator->GetComponent<ScreenTransitionComponent>(e);
        auto& tr = m_coordinator->GetComponent<TransformComponent>(e);
        auto& ui = m_coordinator->GetComponent<UIImageComponent>(e);

        // 【重要】以前のコードにあった「ここで角度を固定する処理」は削除します
        // tr.rotation.z = XMConvertToRadians(st.angleDeg);  <-- これがあると回転しません！

        // 必要な拡大率の計算
        if (st.coverScaleMul <= 0.0f)
        {
            st.coverScaleMul = ComputeCoverScaleMul(st.angleDeg, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
        }

        // --- Transition style ---
        // angleDeg==0 を「回転なしスタイル」とみなし、代わりに
        //  ・アルファのカーブをスナッピーに
        //  ・スケールに軽い“パンチ”を入れる
        // で地味さを解消します。
        const bool noRotationStyle = (std::fabs(st.angleDeg) < 0.01f);
        const float spinRangeDeg = noRotationStyle ? 0.0f : 90.0f;

        switch (st.phase)
        {
        case ScreenTransitionComponent::Phase::Idle:
        {
            st.alpha = 0.0f;
            ui.isVisible = false;
            ui.color.w = 0.0f;
            st.firedOnBlack = false;
            st.firedOnFinished = false;
            return;
        }

        case ScreenTransitionComponent::Phase::DelayBeforeFadeOut:
        {
            // 待機中
            ui.isVisible = true;
            st.alpha = 0.0f;
            ui.color.w = 0.0f;

            // ★初期角度：目標角度 + 回転分 だけ傾けておく
            float startAngle = st.angleDeg + spinRangeDeg;
            tr.rotation.z = XMConvertToRadians(startAngle);

            // スケール設定
            if (st.scaleFadeOut)
            {
                const float mul = (st.startScaleMul > 0.0f ? st.startScaleMul : 0.001f);
                tr.scale.x = (float)SCREEN_WIDTH * mul;
                tr.scale.y = (float)SCREEN_HEIGHT * mul;
            }
            else
            {
                tr.scale.x = (float)SCREEN_WIDTH * st.coverScaleMul;
                tr.scale.y = (float)SCREEN_HEIGHT * st.coverScaleMul;
            }

            st.timerSec += dt;
            if (st.timerSec >= st.delaySec)
            {
                st.phase = ScreenTransitionComponent::Phase::FadingOut;
                st.timerSec = 0.0f;
                st.firedOnBlack = false;
            }
            return;
        }

        case ScreenTransitionComponent::Phase::FadingOut:
        {
            // 画面を隠していく（出現）
            ui.isVisible = true;
            st.timerSec += dt;
            const float dur = (st.durationOutSec > 0.0f) ? st.durationOutSec : ((st.durationSec > 0.0f) ? st.durationSec : 0.001f);
            const float t = Clamp01(st.timerSec / dur);
            const float k = noRotationStyle ? EaseOutCubic(t) : EaseInOut(t);

            st.alpha = k;

            // ★ここが回転計算です
            // k=0(開始)のときは傾いていて、k=1(終了)で目標角度(st.angleDeg)にピタッと合う
            float currentDeg = st.angleDeg + (spinRangeDeg * (1.0f - k));
            tr.rotation.z = XMConvertToRadians(currentDeg);

            float baseMul = st.coverScaleMul;
            if (st.scaleFadeOut)
            {
                baseMul = st.startScaleMul + (st.coverScaleMul - st.startScaleMul) * k;
            }

            // No-rotation style: add a subtle punch so it doesn't feel like a flat fade.
            const float mul = noRotationStyle ? (baseMul * Punch(t, 0.10f)) : baseMul;
            tr.scale.x = (float)SCREEN_WIDTH * mul;
            tr.scale.y = (float)SCREEN_HEIGHT * mul;

            ui.color.w = st.alpha;

            if (t >= 1.0f)
            {
                st.alpha = 1.0f;
                ui.color.w = 1.0f;
                // 完了時はズレないように目標角度をセット
                tr.rotation.z = XMConvertToRadians(st.angleDeg);

                if (!st.firedOnBlack)
                {
                    st.firedOnBlack = true;
                    auto it = s_callbacks.find(e);
                    if (it != s_callbacks.end() && it->second.onBlack) it->second.onBlack();
                }

                if (st.autoFadeIn)
                {
                    st.phase = ScreenTransitionComponent::Phase::FadingIn;
                    st.timerSec = 0.0f;
                }
                else
                {
                    st.phase = ScreenTransitionComponent::Phase::HoldBlack;
                }
            }
            return;
        }

        case ScreenTransitionComponent::Phase::HoldBlack:
        {
            ui.isVisible = true;
            st.alpha = 1.0f;
            ui.color.w = 1.0f;

            // ホールド中は目標角度で固定
            tr.rotation.z = XMConvertToRadians(st.angleDeg);

            tr.scale.x = (float)SCREEN_WIDTH * st.coverScaleMul;
            tr.scale.y = (float)SCREEN_HEIGHT * st.coverScaleMul;
            return;
        }

        case ScreenTransitionComponent::Phase::FadingIn:
        {
            // 画面が見えてくる（消失）
            ui.isVisible = true;
            st.timerSec += dt;
            const float dur = (st.durationInSec > 0.0f) ? st.durationInSec : ((st.durationSec > 0.0f) ? st.durationSec : 0.001f);
            const float t = Clamp01(st.timerSec / dur);
            const float k = noRotationStyle ? EaseInCubic(t) : EaseInOut(t);

            st.alpha = 1.0f - k;

            // ★ここも回転計算です
            // 画面を覆っている状態(k=0)から、さらに回転して消えていく
            float currentDeg = st.angleDeg - (spinRangeDeg * k);
            tr.rotation.z = XMConvertToRadians(currentDeg);

            float baseMul = st.coverScaleMul;
            if (st.scaleFadeIn)
            {
                // 縮小しながら消える
                const float endMul = (st.startScaleMul > 0.0f) ? st.startScaleMul : 0.001f;
                baseMul = st.coverScaleMul + (endMul - st.coverScaleMul) * k;
            }

            // No-rotation style: subtle punch on fade-in too.
            const float mul = noRotationStyle ? (baseMul * Punch(t, 0.06f)) : baseMul;
            tr.scale.x = (float)SCREEN_WIDTH * mul;
            tr.scale.y = (float)SCREEN_HEIGHT * mul;

            ui.color.w = st.alpha;

            if (t >= 1.0f)
            {
                ui.isVisible = false;
                st.phase = ScreenTransitionComponent::Phase::Idle;

                if (!st.firedOnFinished)
                {
                    st.firedOnFinished = true;
                    auto it = s_callbacks.find(e);
                    if (it != s_callbacks.end() && it->second.onFinished) it->second.onFinished();
                }

                ClearCallbacks(e);
            }
            return;
        }

        default:
            return;
        }
    }
}
