/*****************************************************************//**
 * @file	MeshRendererComponent.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/20	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MESHRENDERER_H___
#define ___MESHRENDERER_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include "../Component.h"
// 既存のDirectXリソース管理システムへの依存を宣言
// 実際のプロジェクト構造に合わせてパスを調整してください。
#include "Systems/DirectX/Texture.h" 
#include "Systems/DirectX/MeshBuffer.h" // ※ 描画に必要なメッシュ情報も必要と想定し追加

/**
 * @file MeshRenderer.h
 * @brief メッシュ描画コンポーネントの定義
 * @details このコンポーネントは3Dオブジェクトの「見た目」を制御するデータコンポーネントです。
 * @note 描画自体は、このコンポーネントを持つエンティティに対してRenderSystemが行います。
 */

/**
 * @struct MeshRenderer
 * @brief オブジェクトの見た目（色・テクスチャ・メッシュ）を管理するデータコンポーネント
 * @par 使用例（ビルダーパターン）:
 * @code
 * Entity cube = world.Create()
 * .With<Transform>(...)
 * .With<MeshRenderer>(MeshManager::CUBE_MESH_ID, DirectX::XMFLOAT3{1, 0, 0}) // 赤いキューブ
 * .Build();
 * @endcode
 */
DEFINE_DATA_COMPONENT(MeshRenderer,
    /**
     * @var meshId
     * @brief 描画に使用するメッシュのID
     */
    MeshManager::MeshHandle meshId = MeshManager::INVALID_MESH;

    /**
     * @var color
     * @brief 単色描画時の色、またはテクスチャの色調
     */
    DirectX::XMFLOAT3 color{ 0.3f, 0.7f, 1.0f };
    
    /**
     * @var texture
     * @brief 表面に貼り付けるテクスチャ画像のハンドル
     * @note デフォルトはINVALID_TEXTURE（テクスチャなし）
     */
    TextureManager::TextureHandle texture = TextureManager::INVALID_TEXTURE;
    
    /**
     * @var uvOffset
     * @brief UV座標のオフセット（テクスチャ位置のずらし）。アニメーションに使用
     */
    DirectX::XMFLOAT2 uvOffset{ 0.0f, 0.0f };

    /**
     * @var uvScale
     * @brief UV座標のスケール（テクスチャの繰り返し回数）
     */
    DirectX::XMFLOAT2 uvScale{ 1.0f, 1.0f };

    // コンストラクタ
    MeshRenderer(
        MeshManager::MeshHandle mesh = MeshManager::INVALID_MESH,
        DirectX::XMFLOAT3 col = { 0.3f, 0.7f, 1.0f },
        TextureManager::TextureHandle tex = TextureManager::INVALID_TEXTURE)
        : meshId(mesh), color(col), texture(tex) {}
);

#endif // !___MESHRENDERER_H___