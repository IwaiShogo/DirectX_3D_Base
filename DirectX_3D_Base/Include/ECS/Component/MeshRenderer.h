/*****************************************************************//**
 * @file	MeshRenderer.h
 * @brief	Entityがどの3Dモデルを描画すべきかを指定します。
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

#ifndef ___MESHRENDERER_H___
#define ___MESHRENDERER_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include <cstdint>
#include <DirectXMath.h>

/**
 * @struct MeshRenderer
 * @brief Entityを描画するために必要なメッシュとテクスチャの情報を保持するコンポーネント
 * @note 既存のDirectXベースのコードと連携するため、IDベースでリソースを参照します。
 */
struct MeshRenderer : public IComponent
{
    // MeshBuffer/Model管理クラスからメッシュデータを取得するためのID
    // ID 0は無効なメッシュ、またはデフォルトメッシュを意味すると想定
    std::uint32_t meshId = 0;

    // TextureManagerからテクスチャを取得するためのID
    std::uint32_t textureId = 0;

    // 簡易的なマテリアルカラー (DirectXの定数バッファに渡す)
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA

    // メッシュがロードされたかどうかのフラグ
    bool isLoaded = false;
};

#endif // !___MESHRENDERER_H___