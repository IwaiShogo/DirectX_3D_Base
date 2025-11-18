/*****************************************************************//**
 * @file	UIRenderSystem.cpp
 * @brief	UIRenderSystemクラスの実装
 * 
 * @details	
 * UIImageComponentとTransformComponentに基づいてSprite描画を行う。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/19	初回作成日
 * 			作業内容：	- 追加：UIRenderSystemクラスの実装
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/UI/UIRenderSystem.h"
#include "Systems/AssetManager.h"
#include "Systems/DirectX/Texture.h"
#include "Systems/Sprite.h"

using namespace DirectX;

/**
 * [void - Render]
 * @brief 描画処理。Depthに基づきソートしてから描画します。
 */
void UIRenderSystem::Render()
{
	if (m_entities.empty())
	{
		return;
	}

	// 1. ソート用データの一時リストを作成
	std::vector<UIRenderData> renderList;
	renderList.reserve(m_entities.size());

	for (auto const& entity : m_entities)
	{
		// ECSのシグネチャによって、UIImageComponentを持つことが保証されているが、念のためDepthを取得
		const auto& uiComp = m_coordinator->GetComponent<UIImageComponent>(entity);
		renderList.push_back({ entity, uiComp.depth });
	}

	// 2. Depthに基づいてソート (値が小さいものが奥、大きいものが手前)
	std::sort(renderList.begin(), renderList.end());

	// 3. ソートされた順に描画を実行
	for (const auto& data : renderList)
	{
		ECS::EntityID entity = data.entityID;

		// コンポーネントの取得
		const auto& transformComp = m_coordinator->GetComponent<TransformComponent>(entity);
		const auto& uiComp = m_coordinator->GetComponent<UIImageComponent>(entity);

		// AssetManagerからテクスチャリソースを取得
		Asset::AssetInfo* info = Asset::AssetManager::GetInstance().LoadTexture(uiComp.assetID);
		if (!info || !info->pResource)
		{
			std::cerr << "Warning: Failed to load UI texture for entity " << entity << " (ID: " << uiComp.assetID << ")" << std::endl;
			continue;
		}

		Texture* texture = static_cast<Texture*>(info->pResource);

		// ----------------------------------------
		// Sprite クラスを使用した描画
		// ----------------------------------------

		// 1. テクスチャを設定
		Sprite::SetTexture(texture);

		// 2. 位置とサイズを設定
		// TransformComponentのpos.xyとscale.xyを使用
		Sprite::SetOffset({ transformComp.position.x, transformComp.position.y });
		Sprite::SetSize({ transformComp.scale.x, transformComp.scale.y });

		// 3. UVと色を設定
		Sprite::SetUVPos(uiComp.uvPos);
		Sprite::SetUVScale(uiComp.uvScale);
		Sprite::SetColor(uiComp.color);

		// 4. 描画
		Sprite::Draw();

		// 描画後は次の描画に影響が出ないようテクスチャを解除（Sprite::Draw()内で解除されない場合）
		Sprite::SetTexture(nullptr);
	}
}