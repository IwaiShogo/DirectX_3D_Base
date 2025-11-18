/*****************************************************************//**
 * @file	UIRenderSystem.h
 * @brief	UIImageComponentを持つエンティティを描画するECSシステム
 * 
 * @details	
 * TransformComponentから位置・サイズを取得し、UIImageComponentから
 * テクスチャID、色、描画順序（Depth）を取得してSpriteクラスで描画します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/19	初回作成日
 * 			作業内容：	- 追加：UIRenderSystemクラスの定義
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___UI_RENDER_SYSTEM_H___
#define ___UI_RENDER_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include <vector>
#include <algorithm>

class UIRenderSystem
	: public ECS::System
{
public:
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

	/**
	 * [void - Render]
	 * @brief	描画処理
	 *
	 * @note	TransformComponentとUIImageComponentを持つエンティティのみを処理する。
	 */
	void Render();

private:
	ECS::Coordinator* m_coordinator = nullptr;

	/**
	 * @struct UIRenderData
	 * @brief 描画順序ソートのための一時的なデータ構造
	 */
	struct UIRenderData
	{
		ECS::EntityID entityID;
		float depth;

		// 描画順序を定義するための比較演算子
		// 値が小さいものが奥（先に描画）、値が大きいものが手前（後に描画）
		bool operator<(const UIRenderData& other) const
		{
			return depth < other.depth;
		}
	};
};

#endif // !___UI_RENDER_SYSTEM_H___