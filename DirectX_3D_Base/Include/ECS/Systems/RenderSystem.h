/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	TransformComponentとRenderComponentを持つEntityを描画するSystem。
 * 
 * @details	
 * メインループのDraw()から呼び出され、全ての該当Entityのワールド行列を計算し、
 * Geometoryクラスを用いて描画を行う。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：ECS::Systemを継承した `RenderSystem` を作成。Update/Drawのメソッドを追加。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___RENDER_SYSTEM_H___
#define ___RENDER_SYSTEM_H___

// ===== インクルード =====
// ECS
#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
// Components
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
// Scene
#include "Scene/GameScene.h"

#include <DirectXMath.h>
#include <memory>

/**
 * @class RenderSystem
 * @brief Entityの描画を担当するSystem
 * * 処理対象: TransformComponent と RenderComponent を持つ全てのEntity
 */
class RenderSystem
	: public ECS::System
{
private:
	// ComponentManagerへのアクセス簡略化のため、Coordinatorを保持
	ECS::Coordinator* m_coordinator;

public:
	// Systemの初期化
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief 描画に必要なカメラ設定、デバッグ描画などを行う
	void DrawSetup();

	/// @brief RenderComponentを持つEntityを全て描画する
	void DrawEntities();
};

#endif // !___RENDER_SYSTEM_H___