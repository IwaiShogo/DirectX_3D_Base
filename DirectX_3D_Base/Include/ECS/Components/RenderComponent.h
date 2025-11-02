/*****************************************************************//**
 * @file	RenderComponent.h
 * @brief	Entityの描画方法に関する情報を定義するComponent。
 * 
 * @details	
 * 描画する形状の種類（ボックス、スフィアなど）や、色情報、テクスチャIDなどを保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：描画に必要な形状と色を保持する `RenderComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___RENDER_COMPONENT_H___
#define ___RENDER_COMPONENT_H___

#include <DirectXMath.h>
#include <cstdint>

 /**
  * @enum MeshType
  * @brief 描画するメッシュの形状を識別するための列挙型。
  */
enum MeshType : uint8_t
{
	MESH_BOX,			///< 箱（立方体）
	MESH_SPHERE,		///< 球
	MESH_MODEL,			///< 外部ファイルからのロードモデル (未実装)
	MESH_NONE,			///< 描画を行わない（デバッグ用など）
};

/**
 * @struct RenderComponent
 * @brief Entityの外観（形状、色、テクスチャ）に関するデータ
 */
struct RenderComponent
{
	MeshType type;				///< 描画するメッシュの形状
	DirectX::XMFLOAT4 color;	///< 描画時の色 (R, G, B, A)

	/**
	 * @brief コンストラクタ
	 * @param type - メッシュ形状
	 * @param color - 描画色
	 */
	RenderComponent(
		MeshType type = MESH_BOX,
		DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
	)
		: type(type)
		, color(color)
	{}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(RenderComponent)

#endif // !___RENDER_COMPONENT_H___