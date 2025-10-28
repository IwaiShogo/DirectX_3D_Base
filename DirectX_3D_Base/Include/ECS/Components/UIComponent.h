/*****************************************************************//**
 * @file	UIComponent.h
 * @brief	HUDやメニューなど、2D UIの描画に必要な情報を定義するComponent。
 * 
 * @details	
 * 画面上の位置、サイズ、描画するテクスチャIDなどを保持する
 *
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	初回作成日
 * 			作業内容：	- 追加：UI要素の位置、サイズ、テクスチャIDを保持する `UIComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___UI_COMPONENT_H___
#define ___UI_COMPONENT_H___

#include <DirectXMath.h>
#include <cstdint>

 /**
  * @struct UIComponent
  * @brief 2D UI要素の描画情報
  */
struct UIComponent
{
	uint32_t TextureID;					///< 描画するテクスチャのリソースID
	DirectX::XMFLOAT2 Position;			///< 画面上の中心座標 (ピクセル単位、例: 0, 0が左上)
	DirectX::XMFLOAT2 Size;				///< 描画サイズ (ピクセル単位)
	DirectX::XMFLOAT4 Color;			///< 描画時の色/乗算カラー (RGBA)
	float Depth;						///< 描画深度 (0.0f～1.0f)

	/**
	 * @brief コンストラクタ
	 */
	UIComponent(
		uint32_t textureId = 0,
		DirectX::XMFLOAT2 position = DirectX::XMFLOAT2(0.0f, 0.0f),
		DirectX::XMFLOAT2 size = DirectX::XMFLOAT2(100.0f, 100.0f),
		DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		float depth = 0.5f
	) : TextureID(textureId), Position(position), Size(size), Color(color), Depth(depth)
	{
	}
};

#endif // !___UI_COMPONENT_H___