#include "Scene/FadeManager.h"
#include "Systems/AssetManager.h" 
#include "Main.h" 
#include "Systems/Sprite.h"
#include "Systems/DirectX/DirectX.h"
#include <algorithm>
#include <cmath>
#include "ECS/ECSInitializer.h"       

// }NՓˉ
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

using namespace DirectX;

//  Ă߃G[ɂȂĂ܂ 
struct WipeCB
{
    float progress;
    float centerX; // SX
    float centerY; // SY
    float aspect;  // width/height
};

//  ÓIoϐ̎̒`iK{łj 
FadeState FadeManager::m_state = FadeState::None;
float FadeManager::m_timer = 0.0f;
float FadeManager::m_duration = 1.0f;
float FadeManager::m_progress = 1.0f;
DirectX::XMFLOAT2 FadeManager::m_focusPos = { 0.5f, 0.5f };
float FadeManager::m_screenW = (float)SCREEN_WIDTH;
float FadeManager::m_screenH = (float)SCREEN_HEIGHT;
bool FadeManager::m_isPaused = false;
EasingType FadeManager::m_easingType = EasingType::Linear;

FadeStyle FadeManager::m_fadeStyle = FadeStyle::Wipe;

std::shared_ptr<PixelShader> FadeManager::m_pixelShader;
Microsoft::WRL::ComPtr<ID3D11Buffer> FadeManager::m_cdWipePram;
std::string FadeManager::m_maskTextureAlias = "UI_STAGE_FADE";


// ȈՃC[WOvZ֐
static float CalculateEasing(EasingType type, float t)
{
    // std::clamp ̂߂ std::max/min őp
    t = std::max(0.0f, std::min(t, 1.0f));

    switch (type)
    {
    case EasingType::InQuad:    return t * t;
    case EasingType::OutQuad:   return t * (2.0f - t);
    case EasingType::InOutQuad: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    case EasingType::InCubic:   return t * t * t;
    case EasingType::OutCubic:  return (--t) * t * t + 1.0f;
    case EasingType::InOutCubic:return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    case EasingType::Linear:
    default:                    return t;
    }
}

void FadeManager::Initialize()
{
    // VF[_[`
    const char* PS_SRC = R"EOT(
    struct PS_IN{
        float4 pos : SV_POSITION;
        float2 uv : TEXCOORD0;
        float4 color : COLOR0;
    };
    Texture2D tex : register(t0);
    SamplerState samp : register(s0);

    cbuffer WipeParam : register(b2) {
        float progress;
        float cx;
        float cy;
        float aspect;
        float pad;
    };

    float4 main(PS_IN pin) : SV_TARGET {
        float2 center = float2(cx, cy);
        
        // AXyNg␳i~`ۂ߁j
        float aspectRatio = aspect;
        float2 uv = pin.uv - center;
        uv.x *= aspectRatio; 

        // progress: 1(J) -> 0()
        // ɂ scale i摜gjĉʂ𕢂
        float scale = max(progress, 0.0001);
        uv = uv / (scale * 25.0); // {
        
        uv.x /= aspectRatio; // ߂
        uv = uv + 0.5;  // 摜̒S킹

        // ͈͊ÓuiՕjv
        if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
            return float4(0, 0, 0, 1); 
        }

        // 摜TvO
        float4 col = tex.Sample(samp, uv);
        
        // dvFAt@l}XNiVGbgj
        // 摜̕sgĉʂuBv
        return col * pin.color;
    }
    )EOT";

    m_pixelShader = std::make_shared<PixelShader>();
    m_pixelShader->Compile(PS_SRC);

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(WipeCB);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    auto* device = GetDevice();
    device->CreateBuffer(&desc, nullptr, m_cdWipePram.GetAddressOf());
}

void FadeManager::Finalize()
{
    m_pixelShader.reset();
    m_cdWipePram.Reset();
}

void FadeManager::SetScreenSize(float width, float height)
{
    m_screenW = (width > 1.0f) ? width : 1.0f;
    m_screenH = (height > 1.0f) ? height : 1.0f;
}

void FadeManager::SetFadeStyle(FadeStyle style)
{
    m_fadeStyle = style;
}


void FadeManager::Update(float dt)
{
    if (m_state == FadeState::None || m_state == FadeState::Closed) return;
    if (m_isPaused) return;

    m_timer += dt;

    float t = std::max(0.0f, std::min(m_timer / m_duration, 1.0f));

    float rate = CalculateEasing(m_easingType, t);

    if (m_state == FadeState::FadingOut) // Ă(1->0)
    {
        m_progress = 1.0f - rate;
        if (t >= 1.0f) { m_state = FadeState::Closed; m_progress = 0.0f; }
    }
    else if (m_state == FadeState::FadingIn) // JĂ(0->1)
    {
        m_progress = rate;
        if (t >= 1.0f) { m_state = FadeState::None; m_progress = 1.0f; }
    }
}

void FadeManager::Draw()
{
    if (m_state == FadeState::None) return;

    // ★変更：スタイルによる分岐
    if (m_fadeStyle == FadeStyle::Wipe)
    {
        // ==========================================
        // 既存のワイプ描画 (変更なし)
        // ==========================================
        auto* context = GetContext();
        WipeCB cbData;
        cbData.progress = m_progress;
        cbData.centerX = m_focusPos.x;
        cbData.centerY = m_focusPos.y;
        cbData.aspect = (m_screenH > 0.0f) ? (m_screenW / m_screenH) : 1.0f;
        context->UpdateSubresource(m_cdWipePram.Get(), 0, nullptr, &cbData, 0, 0);

        using namespace DirectX;
        XMFLOAT4X4 viewIdentity, projUI;
        XMStoreFloat4x4(&viewIdentity, XMMatrixIdentity());
        XMStoreFloat4x4(&projUI, XMMatrixOrthographicOffCenterLH(0.0f, m_screenW, m_screenH, 0.0f, 0.0f, 100.0f));

        Sprite::SetView(viewIdentity);
        Sprite::SetProjection(projUI);
        Sprite::SetPixelShader(m_pixelShader.get()); // シェーダーON
        ID3D11Buffer* buffers[] = { m_cdWipePram.Get() };
        context->PSSetConstantBuffers(2, 1, buffers);

        Asset::AssetInfo* info = Asset::AssetManager::GetInstance().LoadTexture(m_maskTextureAlias);
        if (info && info->pResource)
        {
            Texture* pTex = static_cast<Texture*>(info->pResource);
            XMMATRIX W = XMMatrixScaling(m_screenW, m_screenH, 1.0f) *
                XMMatrixTranslation(m_screenW * 0.5f, m_screenH * 0.5f, 0.0f);
            XMFLOAT4X4 worldMat;
            XMStoreFloat4x4(&worldMat, XMMatrixTranspose(W));
            Sprite::SetWorld(worldMat);
            Sprite::SetOffset({ 0, 0 });
            Sprite::SetSize({ 1, 1 });
            Sprite::SetTexture(pTex);
            Sprite::SetUVPos({ 0, 0 });
            Sprite::SetUVScale({ 1, 1 });
            Sprite::SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

            Sprite::Draw();
        }
        Sprite::SetPixelShader(nullptr);
        // 使い終わった定数バッファを解除（他の描画に影響させない）
        ID3D11Buffer* nullCB[] = { nullptr };
        context->PSSetConstantBuffers(2, 1, nullCB);
    }
    else // if (m_fadeStyle == FadeStyle::Alpha)
    {
        // ==========================================
        // ★追加：シンプルなアルファフェード描画
        // ==========================================
        using namespace DirectX;
        XMFLOAT4X4 viewIdentity, projUI;
        XMStoreFloat4x4(&viewIdentity, XMMatrixIdentity());
        XMStoreFloat4x4(&projUI, XMMatrixOrthographicOffCenterLH(0.0f, m_screenW, m_screenH, 0.0f, 0.0f, 100.0f));

        Sprite::SetView(viewIdentity);
        Sprite::SetProjection(projUI);

        // シェーダーは使用しない
        Sprite::SetPixelShader(nullptr);

        // 全画面矩形
        XMMATRIX W = XMMatrixScaling(m_screenW, m_screenH, 1.0f) *
            XMMatrixTranslation(m_screenW * 0.5f, m_screenH * 0.5f, 0.0f);
        XMFLOAT4X4 worldMat;
        XMStoreFloat4x4(&worldMat, XMMatrixTranspose(W));
        Sprite::SetWorld(worldMat);
        Sprite::SetOffset({ 0, 0 });
        Sprite::SetSize({ 1, 1 });

        // テクスチャなし（単色塗りつぶし）
        Sprite::SetTexture(nullptr);

        Sprite::SetUVPos({ 0, 0 });
        Sprite::SetUVScale({ 1, 1 });

        // Alpha計算: 
        // Progress 1.0 (開) -> Alpha 0.0 (透明)
        // Progress 0.0 (閉) -> Alpha 1.0 (不透明/真っ黒)
        float alpha = 1.0f - m_progress;

        // クランプ
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        // 黒色で描画
        Sprite::SetColor({ 0.0f, 0.0f, 0.0f, alpha });

        Sprite::Draw();
    }

    // ---- 描画状態の汚染を最小化（UIの次フレームがズレるのを防ぐ） ----
    // Spriteが内部で静的なView/Projを保持している前提で、UI座標系（左上原点）に戻しておく。
    {
        XMFLOAT4X4 viewI, projUIReset, worldI;
        XMStoreFloat4x4(&viewI, XMMatrixIdentity());
        XMStoreFloat4x4(&projUIReset, XMMatrixOrthographicOffCenterLH(0.0f, m_screenW, m_screenH, 0.0f, 0.0f, 100.0f));
        XMStoreFloat4x4(&worldI, XMMatrixTranspose(XMMatrixIdentity()));
        Sprite::SetView(viewI);
        Sprite::SetProjection(projUIReset);
        Sprite::SetWorld(worldI);
        Sprite::SetPixelShader(nullptr);
        auto* context = GetContext();
        ID3D11Buffer* nullCB[] = { nullptr };
        context->PSSetConstantBuffers(2, 1, nullCB);
    }
}
void FadeManager::FadeIn(float duration, EasingType type)
{
    m_state = FadeState::FadingIn;
    m_progress = 0.0f;
    m_timer = 0.0f;
    m_duration = duration;
    m_easingType = type;
    m_focusPos = { 0.5f, 0.5f }; // tF[hC͏ɒ
}

void FadeManager::FadeOut(float duration, EasingType type)
{
    m_state = FadeState::FadingOut;
    m_progress = 1.0f;
    m_timer = 0.0f;
    m_duration = duration;
    m_easingType = type;
}

void FadeManager::SetPositionFadeOut(float duration, float u, float v, EasingType type)
{
    SetFocus(u, v);
    FadeOut(duration, type);
}

void FadeManager::SetFocus(float x, float y)
{
    m_focusPos = { x, y };
}

bool FadeManager::IsClosed() { return m_state == FadeState::Closed; }
bool FadeManager::IsOpen() { return m_state == FadeState::None; }
bool FadeManager::IsFading() { return m_state == FadeState::FadingIn || m_state == FadeState::FadingOut; }

void FadeManager::Pause() { m_isPaused = true; }
void FadeManager::Resume() { m_isPaused = false; }
bool FadeManager::IsPaused() { return m_isPaused; }
float FadeManager::GetProgress() { return m_progress; }