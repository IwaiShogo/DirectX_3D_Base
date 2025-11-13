/*****************************************************************//**
 * @file	DebugDrawSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___DEBUG_DRAW_SYSTEM_H___
#define ___DEBUG_DRAW_SYSTEM_H___

#include "ECS/ECS.h"

/**
 * @class DebugDrawSystem
 * @brief F1キーでのデバッグモードトグルと、ライン描画によるマップ可視化を担当
 * 処理対象: DebugComponent を持つ Entity
 */
class DebugDrawSystem
	: public ECS::System
{
private:
	ECS::Coordinator* m_coordinator = nullptr;
	void DrawMapStructure(ECS::EntityID mapEntityID); // 内部ヘルパー関数

public:
	void Init(ECS::Coordinator* coordinator) override { m_coordinator = coordinator; }
	void Update();
};

#endif // !___DEBUG_DRAW_SYSTEM_H___