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
#include "Systems/Model.h"

 /**
  * @struct ModelComponent
  * @brief 3Dモデル描画情報
  */
struct ModelComponent
{
	// 3DモデルのリソースID (Model::LoadModelなどが返すIDを想定)
	uint32_t ModelID;

	std::unique_ptr<Model> pModel = std::make_unique<Model>();

	// モデル描画で使用するテクスチャのリソースID
	uint32_t TextureID;

	// アニメーションを使用する場合のアニメーションID (将来的な拡張用)
	uint32_t AnimationID;

	/**
	 * @brief コンストラクタ
	 */
	ModelComponent() = default;

	// ユーザーが作成した引数付きコンストラクタ
	ModelComponent(const char* path, float scale, Model::Flip flip)
	{
		if (!pModel->Load(path, scale, flip))
		{
			MessageBox(NULL, "モデルのロードに失敗", "Error", MB_OK);
		}
	}
};

#endif // !___MODEL_COMPONENT_H___