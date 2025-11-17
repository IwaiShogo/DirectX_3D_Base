/*****************************************************************//**
 * @file	UISystem.cpp
 * @brief	UISystemの実装。UIComponentを持つEntityの2D描画を行う。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	初回作成日
 * 			作業内容：	- 追加：UIComponentに基づき、Sprite::Drawを使用してUIを描画するロジックを実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/UISystem.h"
#include "Systems/DirectX/DirectX.h" // SetDepthTest関数を使用
#include "Systems/DirectX/Texture.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/ResourceManager.h"

using namespace DirectX;

/**
 * @brief UI要素を描画する
 */
void UISystem::Draw()
{
	// 1. UI描画のための準備（深度テストの無効化）
	SetDepthTest(false);

	// Systemが保持するEntityセットをイテレート
	// UI描画の効率化のため、Depth値でソートしてから描画することが推奨されるが、ここでは実装の単純化のため省略。
	for (auto const& entity : m_entities)
	{
		UIComponent& uiComp = m_coordinator->GetComponent<UIComponent>(entity);

		//IsVisibleがfalseなら、このUIを描画しない
			if (!uiComp.IsVisible)
			{
				continue;
			}

		Texture* pTexture = ResourceManager::GetTexture(uiComp.TextureID);
		if (!pTexture)
		{
			continue;
		}

		Sprite::SetTexture(pTexture);

		Sprite::SetOffset(uiComp.Position);
		Sprite::SetSize(uiComp.Size);
		Sprite::SetColor(uiComp.Color);

		Sprite::Draw();
		// Sprite APIを使用して描画
		// Sprite::Draw(テクスチャID, 画面位置, サイズ, 描画カラー, 深度)
		/*Sprite::Draw(
			uiComp.TextureID,
			uiComp.Position,
			uiComp.Size,
			uiComp.Color*/
		//);

		// ※ Sprite::DrawのAPIシグネチャはプロジェクトのSprite.hに依存します。
		// ここでは、一般的な引数（TextureID, x, y, width, height, r, g, b, a）を想定しています。
	}

	// 2. UI描画の終了処理（深度テストの再有効化）
	SetDepthTest(true);
}