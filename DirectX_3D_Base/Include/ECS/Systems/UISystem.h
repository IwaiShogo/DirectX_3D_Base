/*****************************************************************//**
 * @file	UISystem.h
 * @brief	UIComponentを持つEntityを描画するSystem。
 * 
 * @details	SpriteシステムAPIを使用して2D描画を行う。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	初回作成日
 * 			作業内容：	- 追加：ECS::Systemを継承した `UISystem` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___UI_SYSTEM_H___
#define ___UI_SYSTEM_H___

#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
#include "ECS/Components/UIComponent.h"
#include "Scene/GameScene.h" 
#include "Systems/Sprite.h" // Sprite::Drawなどを使用

 /**
  * @class UISystem
  * @brief UIComponentを持つEntityの2D描画を担当するSystem
  * * 処理対象: UIComponent を持つ全てのEntity
  */
class UISystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator;

public:
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief UI要素を描画する
	void Draw();
};

#endif // !___UI_SYSTEM_H___