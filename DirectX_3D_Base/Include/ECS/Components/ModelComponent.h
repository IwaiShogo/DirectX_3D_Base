/*****************************************************************//**
 * @file	ModelComponent.h
 * @brief	3Dモデルの描画に必要な情報を定義するComponent。
 * 
 * @details	ロードされたモデルのリソースID、テクスチャIDなどを保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/28	初回作成日
 * 			作業内容：	- 追加：モデルIDとテクスチャIDを保持する `ModelComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MODEL_COMPONENT_H___
#define ___MODEL_COMPONENT_H___

// ===== インクルード =====
#include <cstdint>

 /**
  * @struct ModelComponent
  * @brief 3Dモデル描画情報
  */
struct ModelComponent
{
	// 3DモデルのリソースID (Model::LoadModelなどが返すIDを想定)
	uint32_t ModelID;

	// モデル描画で使用するテクスチャのリソースID
	uint32_t TextureID;

	// アニメーションを使用する場合のアニメーションID (将来的な拡張用)
	uint32_t AnimationID;

	/**
	 * @brief コンストラクタ
	 */
	ModelComponent(
		uint32_t modelId = 0,
		uint32_t textureId = 0,
		uint32_t animationId = 0
	) : ModelID(modelId), TextureID(textureId), AnimationID(animationId)
	{}
};

#endif // !___MODEL_COMPONENT_H___