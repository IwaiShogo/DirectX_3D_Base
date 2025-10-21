#pragma once
// --------------------------------------------------
// インクルードパス修正
// RenderSystem.h の位置: Include/ECS/System/
// DirectX.h の位置: Include/Systems/DirectX/
// --------------------------------------------------
#include "Systems/DirectX/DirectX.h"    // GetContext(), GetDevice()
#include "Systems/DirectX/ShaderList.h" // SetWVP, SetMaterial, SetLight...
#include "Systems/DirectX/MeshBuffer.h"  // MeshBuffer::Draw()
#include "Systems/DirectX/Texture.h"     // TextureManager::INVALID_TEXTURE

// ECSコア (同じ Include/ECS/ からの相対パス)
#include "ECS/World.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"

// Cameraの依存を解決
//#include "Camera.h" // 上記で定義したCamera.h (Include/ECS/System/Camera.h)

#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstring>
#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")

// 既存のMeshBufferのDrawを呼ぶためのラッパー（ここでは単なる前方宣言）
class MeshBuffer;
class Camera; // RenderSystemが依存するCameraクラス

/**
 * @struct RenderSystem
 * @brief ECSワールド内の描画可能なエンティティを描画するシステム
 */
struct RenderSystem
{
    // 依存関係を既存クラス名と整合
    // DirectX.h がグローバル関数を使用するため、ここでは依存を最小限にします
    // CameraはDrawメソッド内で使用

    // ※ 既存のMeshBufferクラスはインスタンス管理が必要なため、ここではDraw時に取得することを想定

    // 定数バッファはShaderListが管理しているため、RenderSystemではWRL::ComPtrは不要です。
    // ShaderList::SetWVP() がグローバルな定数バッファを更新すると想定されます。

    // RenderSystemがRenderSystem::Draw()外で管理すべきリソース（シェーダ、定数バッファ）
    // ShaderList::VSKind/PSKind を保持
    ShaderList::VSKind vsKind_ = ShaderList::VS_WORLD;
    ShaderList::PSKind psKind_ = ShaderList::PS_LAMBERT;

    // ※ Drawメソッド内でDrawCallを最適化するために、描画に使用するメッシュのポインタやIDを保持する機構が必要です。
    //    ここでは、Drawメソッドの引数としてMeshBufferの管理クラスを渡すことを前提とします。

    /**
     * @brief 描画実行のメインエントリーポイント
     * @param[in,out] w Worldへの参照
     * @param[in] cam 現在のシーンカメラ
     * @param[in] meshMgr MeshBuffer管理クラス（例: MeshManager）
     */
    template<typename MeshManager>
    void Draw(World& w, Camera& cam, MeshManager& meshMgr)
    {
        // 1. シェーダと描画コンテキストの設定 (ShaderList.hを使用)
        ShaderList::GetVS(vsKind_)->Set();
        ShaderList::GetPS(psKind_)->Set();

        // 2. カメラ行列の取得
        DirectX::XMMATRIX view = cam.GetViewMatrix();
        DirectX::XMMATRIX proj = cam.GetProjectionMatrix();
        DirectX::XMMATRIX viewProj = view * proj;

        // 3. 全ての描画可能なエンティティに対してループ
        Store<Transform>* transformStore = w.GetStore<Transform>();
        Store<MeshRenderer>* meshStore = w.GetStore<MeshRenderer>();

        for (const auto& [id, mr] : meshStore->GetAll())
        {
            Entity e = { id };
            Transform* t = transformStore->TryGet(e);
            if (!t) continue;

            // a. World行列の計算とTransformへのキャッシュ
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t->scale.x, t->scale.y, t->scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t->rotation.x),
                DirectX::XMConvertToRadians(t->rotation.y),
                DirectX::XMConvertToRadians(t->rotation.z));
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t->position.x, t->position.y, t->position.z);
            DirectX::XMMATRIX W = S * R * T;
            DirectX::XMStoreFloat4x4(&t->worldMatrix, W);

            DirectX::XMMATRIX WVP = W * viewProj;

            // b. VS定数バッファの更新 (ShaderList::SetWVPを使用)
            // ShaderList::SetWVPはXMFLOAT4X4を期待しているため変換
            DirectX::XMFLOAT4X4 WVP_T;
            DirectX::XMStoreFloat4x4(&WVP_T, DirectX::XMMatrixTranspose(WVP));
            ShaderList::SetWVP(&WVP_T);

            // c. PS定数バッファの更新 (ShaderList::SetMaterialを使用)
            // MeshRendererのデータからShaderList::SetMaterialに渡すMaterial構造体を構築 (Model::Materialが必要)
            // Model::Materialの定義がないため、ここでは色とテクスチャの使用フラグのみを渡すと仮定
            // ShaderList::SetMaterial(mr.color, (mr.texture != TextureManager::INVALID_TEXTURE));

            // ※ ShaderList::SetMaterialはModel::Materialを期待しています。ここではRenderSystemのコードを簡略化し、MeshRendererのデータを利用することに重点を置きます。
            // 既存のSetMaterialをECSデータに合わせるために、既存のModel::Material構造体を定義するか、ShaderListを変更する必要があります。
            // ここでは、MeshRendererを直接利用するよう、簡略化された擬似的な呼び出しを使用します。

            // d. テクスチャの設定（Texture::GetResource()とDirectX::SetShaderResourcesを使用）
            // Texture* tex = TextureManager::GetTexture(mr.texture); // TextureManagerクラスが別途必要
            // if (tex) GetContext()->PSSetShaderResources(0, 1, tex->GetResource());

            // e. 描画実行 (MeshBuffer::Drawを使用)
            // MeshBuffer* meshBuf = meshMgr.GetMeshBuffer(mr.meshId); // MeshManagerクラスが別途必要
            // if (meshBuf) meshBuf->Draw();
        }
    }
};