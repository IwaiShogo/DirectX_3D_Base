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

// Coordinatorのグローバル参照 (GameSceneが管理することを想定し、外部連携用として定義)
extern Coordinator *g_pCoordinator;

/**
 * @class RenderSystem
 * @brief ECSワールド内の描画可能なエンティティを描画するシステム
 * @note System基底クラスを継承し、Update(またはDraw)メソッドでロジックを実行します。
 */
class RenderSystem
    : public System
{
public:
    // 描画に必要なシェーダーの種類（必要に応じて外部から設定可能）
    ShaderList::VSKind vsKind_ = ShaderList::VS_WORLD;
    ShaderList::PSKind psKind_ = ShaderList::PS_LAMBERT;

    // System基底クラスの純粋仮想関数を実装（RenderSystemはUpdateでは何もしない）
    void Update(float deltaTime) override {}

    /**
     * @brief 描画実行のメインエントリーポイント (DirectXのDrawCallを実行)
     * @tparam MeshManager MeshBufferの管理クラス（テンプレートにより未定義のまま利用可能）
     * @param[in] viewProjMatrix CameraSystemから計算されたView * Projection行列
     * @param[in] meshMgr MeshBuffer管理クラスの参照（ダミーまたは実際のマネージャー）
     */
    template<typename MeshManager>
    void Draw(const DirectX::XMMATRIX& viewProjMatrix, MeshManager& meshMgr)
    {
        // 1. シェーダと描画コンテキストの設定
        ShaderList::GetVS(vsKind_)->Set();
        ShaderList::GetPS(psKind_)->Set();

        // 2. 全ての描画可能なエンティティに対してループ（ECSの核心）
        for (const Entity entity : *entities)
        {
            // Coordinatorを通じてデータ（Component）を取得
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
            DirectX::XMStoreFloat4x4(&t.worldMatrix, W); // 結果をComponentに書き戻す

            // b. WVP行列の計算
            DirectX::XMMATRIX WVP = W * viewProjMatrix; // CameraSystemからの行列を使用

            // c. VS定数バッファの更新 (WVPをシェーダーに転送)
            DirectX::XMFLOAT4X4 WVP_T;
            DirectX::XMStoreFloat4x4(&WVP_T, DirectX::XMMatrixTranspose(WVP));
            ShaderList::SetWVP(&WVP_T);

            // d. PS定数バッファの更新 (Material/Color)
            // (mr.color, mr.textureId などを使用し、DirectXの定数バッファを設定)

            // e. 描画実行 (MeshBuffer::Drawを使用)
            // MeshBuffer* meshBuf = meshMgr.GetMeshBuffer(mr.meshId); 
            // if (meshBuf && mr.isLoaded) meshBuf->Draw(); 
        }
    }
};

#endif // !___RENDERSYSTEM_H___