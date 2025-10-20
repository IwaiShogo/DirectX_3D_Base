#pragma once
#include "graphics/GfxDevice.h" // DirectXデバイスへのアクセス
#include "graphics/Camera.h"   // カメラ（View/Proj行列）へのアクセス
#include "ECS/World.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"
#include <DirectXMath.h>
// ... 他に必要なDirectX/Shaderのヘッダー

/**
 * @file RenderSystem.h
 * @brief メッシュレンダリングシステム
 * @details World内のMeshRendererコンポーネントを持つエンティティを描画します。
 */

 /**
  * @struct RenderSystem
  * @brief ECSワールド内の描画可能なエンティティを描画するシステム
  * @note このシステムはWorld::Update()とは独立して、メインループから明示的にDraw()を呼び出す必要があります。
  */
struct RenderSystem
{
    // 既存コードの依存関係をメンバ変数として保持することを想定
    GfxDevice& gfx_;
    Camera& cam_;
    // ... 定数バッファやシェーダ関連のメンバもここに定義

    // 定数バッファの構造体 (RenderSystem.hのRenderSystem::Updateのコードから推測)
    struct VSConstants {
        DirectX::XMMATRIX WVP;
        DirectX::XMFLOAT4 uvTransform;
    };
    struct PSConstants {
        DirectX::XMFLOAT4 color;
        float useTexture;
        DirectX::XMFLOAT3 padding;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> cb_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> psCb_;
    // ... ShaderやInputLayoutのメンバも必要

    /**
     * @brief 初期化
     */
    RenderSystem(GfxDevice& gfx, Camera& cam) : gfx_(gfx), cam_(cam)
    {
        // 既存コードからShader/ConstantBufferの初期化ロジックを移植
        // 例: cb_ = GfxDevice::CreateConstantBuffer(sizeof(VSConstants));
    }

    /**
     * @brief 描画実行のメインエントリーポイント
     * @param[in,out] w Worldへの参照
     * * @details
     * このメソッドは、ゲームのメインループまたはGameScene::Draw()内で呼び出されます。
     */
    void Draw(World& w)
    {
        // 1. カメラ行列の取得と設定（View/Proj）
        DirectX::XMMATRIX view = cam_.GetViewMatrix();
        DirectX::XMMATRIX proj = cam_.GetProjectionMatrix();

        // 2. 描画コンテキストの設定 (InputLayout, Shaderのセットアップ)
        // ... 既存コードからシェーダ設定を移植

        // 3. 全ての描画可能なエンティティに対してループ
        Store<Transform>* transformStore = w.GetStore<Transform>();
        Store<MeshRenderer>* meshStore = w.GetStore<MeshRenderer>();

        for (const auto& [id, mr] : meshStore->GetAll())
        {
            Entity e = { id };
            Transform* t = transformStore->TryGet(e);
            if (!t) continue; // Transformがないエンティティは描画しない

            // a. World行列の計算と設定 (RenderSystem.hのコードを参考に実装)
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t->scale.x, t->scale.y, t->scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t->rotation.x),
                DirectX::XMConvertToRadians(t->rotation.y),
                DirectX::XMConvertToRadians(t->rotation.z));
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t->position.x, t->position.y, t->position.z);
            DirectX::XMMATRIX W = S * R * T;

            // WorldMatrixをTransformにキャッシュ (オプション)
            DirectX::XMStoreFloat4x4(&t->worldMatrix, W);

            // b. VS定数バッファの更新 (WVP行列, UV変換)
            VSConstants vsCbuf;
            vsCbuf.WVP = DirectX::XMMatrixTranspose(W * view * proj);
            vsCbuf.uvTransform = DirectX::XMFLOAT4{ mr.uvOffset.x, mr.uvOffset.y, mr.uvScale.x, mr.uvScale.y };
            gfx_.Ctx()->UpdateSubresource(cb_.Get(), 0, nullptr, &vsCbuf, 0, 0);

            // c. PS定数バッファの更新 (色, テクスチャ使用フラグ)
            PSConstants psCbuf;
            psCbuf.color = DirectX::XMFLOAT4{ mr.color.x, mr.color.y, mr.color.z, 1.0f };
            psCbuf.useTexture = (mr.texture != TextureManager::INVALID_TEXTURE) ? 1.0f : 0.0f;
            gfx_.Ctx()->UpdateSubresource(psCb_.Get(), 0, nullptr, &psCbuf, 0, 0);

            // d. テクスチャの設定
            // TextureManager::BindTexture(mr.texture); // 既存リソースマネージャを介してバインド

            // e. 描画実行
            // MeshManager::DrawMesh(mr.meshId); // 既存リソースマネージャを介してドローコール
        }
    }
};