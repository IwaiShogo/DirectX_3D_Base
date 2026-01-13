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
#include <memory>
#include "Systems/Model.h"
#include "Systems/DirectX/ShaderList.h"
#include "Systems/AssetManager.h"

 /**
  * @struct ModelComponent
  * @brief 3Dモデル描画情報
  */
struct ModelComponent
{
	Model* pModel = nullptr;
	std::string assetID;

	/**
	 * @brief コンストラクタ
	 */
	ModelComponent() = default;

	// ユーザーが作成した引数付きコンストラクタ
	ModelComponent(const std::string& id, float scale, Model::Flip flip)
	{
		assetID = id;

		Asset::AssetInfo* pAssetInfo = Asset::AssetManager::GetInstance().LoadModel(id, scale, flip);

		if (pAssetInfo != nullptr && pAssetInfo->pResource != nullptr)
		{
			// キャッシュされたリソースポインタを取得し、Model*にキャスト
			pModel = static_cast<Model*>(pAssetInfo->pResource);
		}
		else
		{
			MessageBox(NULL, "AssetManagerからのモデルのロード/取得に失敗", "Error", MB_OK);
			pModel = nullptr;
		}
		// pModel->Load()はAssetManager::LoadModel()の中で呼び出されるため、削除
	}

	ModelComponent(const ModelComponent&) = delete;
	ModelComponent& operator=(const ModelComponent&) = delete;

	// ムーブ操作は noexcept 付きで明示的にデフォルト化（前回修正済み）
	ModelComponent(ModelComponent&&) noexcept = default;
	ModelComponent& operator=(ModelComponent&&) noexcept = default;
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(ModelComponent)

#endif // !___MODEL_COMPONENT_H___