/*****************************************************************//**
 * @file	SwitchStateSystem.cpp
 * @brief	カメラ切り替え
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/Core/StateSwitchSystem.h"

void StateSwitchSystem::Update(float deltaTime)
{
	// SpaceキーまたはXbox Aボタンで切り替え
	bool isSwitchTriggered = IsKeyTrigger(VK_SPACE) || IsButtonTriggered(BUTTON_A);

	if (!isSwitchTriggered) return;

	for (auto const& entity : m_entities) // GameStateComponentを持つEntity（GameController）を対象
	{
		GameStateComponent& stateComp = m_coordinator->GetComponent<GameStateComponent>(entity);

		if (stateComp.currentMode == GameMode::SCOUTING_MODE)
		{
			stateComp.currentMode = GameMode::ACTION_MODE;
		}
		else if (stateComp.currentMode == GameMode::ACTION_MODE)
		{
			stateComp.currentMode = GameMode::SCOUTING_MODE;
		}
	}
}