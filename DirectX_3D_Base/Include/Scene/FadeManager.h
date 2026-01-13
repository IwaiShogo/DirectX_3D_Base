/*****************************************************************//**
 * @file   FadeManager.h
 * @brief  V[Jڎ̃tF[h(ACXCv)ǗNX
 *********************************************************************/
#pragma once

 // ǉFWindows}NƂ̏Փ˂͂ɖh
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#include <memory>
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl.h>
#include "Systems/DirectX/Shader.h"

// EasingMath.h Ȃ߁AŊȈՒ`
enum class EasingType
{
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic
};

enum class FadeStyle
{
    Wipe,   // 既存の円形ワイプ
    Alpha   // 一般的な黒フェード
};

enum class FadeState
{
    None,       // ĂȂ
    FadingOut,  // ĂĂ
    FadingIn,   // JĂĂ
    Closed      // SɕĂ
};

class FadeManager
{
public:
    static void Initialize();
    static void Finalize();

    static void Update(float dt);
    static void Draw();

    // 画面サイズ（実ウインドウ/バックバッファ）を反映。未設定の場合は初期値を使用
    static void SetScreenSize(float width, float height);

    static void SetFadeStyle(FadeStyle style);
    // p֐
    static void FadeIn(float duration = 1.0f, EasingType type = EasingType::Linear);
    static void FadeOut(float duration = 1.0f, EasingType type = EasingType::Linear);

    // Ww肵ătF[hAEgJni{^̈ʒuȂǂŕ鎞Ɏgpj
    static void SetPositionFadeOut(float duration, float u, float v, EasingType type = EasingType::Linear);

    // i̒SWw肷֐(0.0`1.0UVW)
    // (0.5, 0.5)ŉʒɂȂ܂B
    static void SetFocus(float x, float y);

    // op
    static void     Pause();        // ꎞ~
    static void     Resume();       // ĊJ
    static bool     IsPaused();     // ~?
    static float    GetProgress();  // ݂̐i(0.0-1.0)擾

    // ԊmF
    static bool IsClosed(); // ^ÂH
    static bool IsOpen();   // SJH
    static bool IsFading(); // Aj[VH

private:
    FadeManager() = delete; // CX^X֎~

    // f[^
    static FadeState m_state;
    static float m_timer;
    static float m_duration;
    static float m_progress;
    static DirectX::XMFLOAT2 m_focusPos; // ڍW(l͒)
    static float m_screenW;
    static float m_screenH;
    static bool m_isPaused;
    static FadeStyle m_fadeStyle;
    // ݂̃C[WO
    static EasingType m_easingType;

    // `p\[X
    static std::shared_ptr<PixelShader> m_pixelShader;
    static Microsoft::WRL::ComPtr<ID3D11Buffer> m_cdWipePram;
    static std::string m_maskTextureAlias;
};