/*****************************************************************//**
 * @file	UIImageComponent.h
 * @brief	UIの画像描画に必要な情報を保持するコンポーネント
 * 
 * @details	
 * 描画するテクスチャのアセットId、UV情報、描画色、および
 * 描画順序（深度）を定義する。UIRenderSystemによって処理されます。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/19	初回作成日
 * 			作業内容：	- 追加：UIImageComponent構造体の定義
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___UI_IMAGE_COMPONENT_H___
#define ___UI_IMAGE_COMPONENT_H___

// ===== インクルード =====
#include <string>
#include <DirectXMath.h>

/**
 * @struct	UIImageComponent
 * @brief	
 */
struct UIImageComponent
{
	// ----------------------------------------
	// 描画指示データ
	// ----------------------------------------
	std::string assetID = "";				// AssetManagerに登録されたテクスチャID
	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 描画色 (R, G, B, A)

	// 描画するテクスチャの切り出し範囲 (UV情報)
	DirectX::XMFLOAT2 uvPos = { 0.0f, 0.0f };		// UVの開始位置 (0.0f - 1.0f)
	DirectX::XMFLOAT2 uvScale = { 1.0f, 1.0f };		// UVのサイズ/スケール (0.0f - 1.0f)

	// 描画順序（手前/奥のZ順序を制御。値が小さいほど奥）
	// UIRenderSystemがこの値で描画順序をソートします。
	float depth = 0.5f;

	bool isVisible;

	// ----------------------------------------
	// コンストラクタ
	// ----------------------------------------
	UIImageComponent() = default;

	/**
	 * @brief	テクスチャIDを指定するコンストラクタ
	 * @param	[in] id アセットID
	 * @param	[in] c 描画色 (デフォルトは白、不透明)
	 */
	UIImageComponent(const std::string& id, float d = 0.5f, bool v = true, const DirectX::XMFLOAT4& c = { 1.0f, 1.0f, 1.0f, 1.0f })
		: assetID(id), depth(d), isVisible(v), color(c)
	{
	}
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(UIImageComponent)

#endif // !___UI_IMAGE_COMPONENT_H___