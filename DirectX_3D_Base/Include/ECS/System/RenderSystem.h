/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	ECSワールド内の描画可能なエンティティを描画するシステム
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___RENDERSYSTEM_H___
#define ___RENDERSYSTEM_H___

// ===== インクルード =====
// --------------------------------------------------
// ECS コア
// --------------------------------------------------
#include "ECS/Types.h"
#include "ECS/Coordinator.h" // Coordinatorの参照を必要とするため、ここでインクルード

// 描画に必要なComponent
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"

// --------------------------------------------------
// DirectX 依存システム (RenderSystemの実行に必要な外部サービス)
// --------------------------------------------------
#include "Systems/DirectX/DirectX.h"    
#include "Systems/DirectX/ShaderList.h" 
#include "Systems/DirectX/MeshBuffer.h"  
#include "Systems/DirectX/Texture.h"     

#include <DirectXMath.h>
#include <wrl/client.h>
#include <map>

// MeshBufferの管理クラスはまだ定義されていませんが、Drawメソッドの引数として必要です。
class MeshManager; // モデル・メッシュを管理するクラス (仮)
class Camera;      // カメラ情報を提供するクラス (仮)

// Coordinatorのグローバル参照 (GameSceneが管理することを想定し、外部連携用として定義)
extern Coordinator *g_pCoordinator;

/**
 * @class RenderSystem
 * @brief ECSワールド内の描画可能なエンティティを描画するシステム
 * @note System基底クラスを継承し、Update(またはDraw)メソッドでロジックを実行します。
 */
class RenderSystem : public System
{
public:
    // 描画に必要なリソースの状態
    ShaderList::VSKind vsKind_ = ShaderList::VS_WORLD;
    ShaderList::PSKind psKind_ = ShaderList::PS_LAMBERT;

    // ECSのSystemクラスは、抽象基底クラスのUpdateメソッドを実装する義務がありますが、
    // RenderSystemは描画パイプラインの最後に呼ばれることが多いため、Drawメソッドで代替します。
    // そのため、Updateメソッドはここでは空で定義します。（純粋仮想関数を実装するため）
    void Update(float deltaTime) override {}

    /**
     * @brief 描画実行のメインエントリーポイント (DirectXのDrawCallを実行)
     * @param[in] cam 現在のシーンカメラ
     * @param[in] meshMgr MeshBuffer管理クラス
     */
    template<typename MeshManager>
    void Draw(Camera& cam, MeshManager& meshMgr)
    {
        // 1. シェーダと描画コンテキストの設定 (Systemの初期化または外部から一度だけ設定)
        ShaderList::GetVS(vsKind_)->Set();
        ShaderList::GetPS(psKind_)->Set();

        // 2. カメラ行列の取得
        DirectX::XMMATRIX view = cam.GetViewMatrix();
        DirectX::XMMATRIX proj = cam.GetProjectionMatrix();
        DirectX::XMMATRIX viewProj = view * proj;

        // 3. 全ての描画可能なエンティティに対してループ
        // this->entitiesには、RenderSystemのSignatureに一致するEntity IDのみが含まれています。
        for (const Entity entity : *entities)
        {
            // Coordinatorを通じてComponentデータを取得 (ロジックとデータを分離)
            // RenderSystemのSignatureが Transform と MeshRenderer を要求するため、
            // ここで取得が失敗することはないはずですが、保険として Try-Catch やアサートを使用する方が望ましいです。

            Transform& t = g_pCoordinator->GetComponent<Transform>(entity);
            MeshRenderer& mr = g_pCoordinator->GetComponent<MeshRenderer>(entity);

            // a. World行列の計算とTransformへのキャッシュ
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t.rotation.x),
                DirectX::XMConvertToRadians(t.rotation.y),
                DirectX::XMConvertToRadians(t.rotation.z)
            );
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
            DirectX::XMMATRIX W = S * R * T;
            DirectX::XMStoreFloat4x4(&t.worldMatrix, W); // 計算結果をComponentに書き戻す

            DirectX::XMMATRIX WVP = W * viewProj;

            // b. VS定数バッファの更新 (WVP)
            DirectX::XMFLOAT4X4 WVP_T;
            DirectX::XMStoreFloat4x4(&WVP_T, DirectX::XMMatrixTranspose(WVP));
            ShaderList::SetWVP(&WVP_T);

            // c. PS定数バッファの更新 (Material/Color)
            // Material定数バッファにWVP以外の情報(ワールド行列など)が必要な場合、ここでセット
            // ShaderList::SetMaterial(mr.color, (mr.textureId != 0)); // 擬似コード

            // d. テクスチャの設定
            // Texture* tex = TextureManager::GetTexture(mr.textureId); // TextureManagerクラスが別途必要
            // if (tex) GetContext()->PSSetShaderResources(0, 1, tex->GetResource()); // 擬似コード

            // e. 描画実行 (MeshBuffer::Drawを使用)
            // MeshBuffer* meshBuf = meshMgr.GetMeshBuffer(mr.meshId); 
            // if (meshBuf && mr.isLoaded) meshBuf->Draw(); // 擬似コード
        }
    }
};

#endif // !___RENDERSYSTEM_H___