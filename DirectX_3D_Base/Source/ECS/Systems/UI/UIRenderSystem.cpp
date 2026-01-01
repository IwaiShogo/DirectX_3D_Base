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
#include "Systems/DirectX/DirectX.h"

using namespace DirectX;

/**
 * [void - Render]
 * @brief 描画処理。Depthに基づきソートしてから描画します。
 */
 /**
  * [void - Render]
  * @brief 描画処理。Depthに基づきソートしてから描画します。
  */
void UIRenderSystem::Render(bool drawBackground)
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
		if (!uiComp.isVisible) continue;

		if (drawBackground)
		{
			// 背景モード: depthが 0 以上ならスキップ（手前のものは描かない）
			if (uiComp.depth >= 0.0f) continue;
		}
		else
		{
			// 前景モード: depthが 0 未満ならスキップ（奥のものは描かない）
			if (uiComp.depth < 0.0f) continue;
		}

		renderList.push_back({ entity, uiComp.depth });
	}

	// 2. Depthに基づいてソート (値が小さいものが奥、大きいものが手前)
	std::sort(renderList.begin(), renderList.end());

	SetDepthTest(false);
	SetBlendMode(BLEND_ALPHA);

	// 3. ソートされた順に描画を実行
	for (const auto& data : renderList)
	{
		ECS::EntityID entity = data.entityID;

		// コンポーネントの取得
		const auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		const auto& uiComp = m_coordinator->GetComponent<UIImageComponent>(entity);

		// AssetManagerからテクスチャリソースを取得
		Asset::AssetInfo* info = Asset::AssetManager::GetInstance().LoadTexture(uiComp.assetID);
		if (!info || !info->pResource)
		{
			std::cerr << "Warning: Failed to load UI texture for entity " << entity << " (ID: " << uiComp.assetID << ")" << std::endl;
			continue;
		}

		Texture* texture = static_cast<Texture*>(info->pResource);

		// 1. サイズの変換 (ピクセル -> 0.0?2.0の範囲)
		// Scale.x / ScreenWidth * 2.0
		float ndcScaleX = (transform.scale.x / (float)SCREEN_WIDTH) * 2.0f;
		float ndcScaleY = (transform.scale.y / (float)SCREEN_HEIGHT) * 2.0f;

		// 2. 位置の変換 (ピクセル -> -1.0?1.0の範囲)
		// 左上が原点のピクセル座標を、中心原点のNDCへ変換
		// X: (Pos / Width) * 2 - 1
		// Y: 1 - (Pos / Height) * 2  (Y軸は反転する)
		float ndcPosX = (transform.position.x / (float)SCREEN_WIDTH) * 2.0f - 1.0f;
		float ndcPosY = 1.0f - (transform.position.y / (float)SCREEN_HEIGHT) * 2.0f;
		float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
		// ----------------------------------------
		// Sprite クラスを使用した描画
		// ----------------------------------------

		// Sprite設定
		Sprite::SetTexture(texture);

		// 変換後の座標をセット
		Sprite::SetOffset({ ndcPosX, ndcPosY });
		Sprite::SetSize({ ndcScaleX, ndcScaleY });
		Sprite::SetUVPos(uiComp.uvPos);
		Sprite::SetUVScale(uiComp.uvScale);
		Sprite::SetColor(uiComp.color);
		Sprite::SetAngle(transform.rotation.z);

		
		Sprite::SetAspect(aspect);

		Sprite::Draw();
		Sprite::SetTexture(nullptr);
	}

	SetDepthTest(true);
}