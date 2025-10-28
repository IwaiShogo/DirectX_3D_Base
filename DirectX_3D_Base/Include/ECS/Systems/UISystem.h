/*****************************************************************//**
 * @file	UISystem.h
 * @brief	UIComponent������Entity��`�悷��System�B
 * 
 * @details	Sprite�V�X�e��API���g�p����2D�`����s���B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS::System���p������ `UISystem` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___UI_SYSTEM_H___
#define ___UI_SYSTEM_H___

#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
#include "ECS/Components/UIComponent.h"
#include "Scene/GameScene.h" 
#include "Systems/Sprite.h" // Sprite::Draw�Ȃǂ��g�p

 /**
  * @class UISystem
  * @brief UIComponent������Entity��2D�`���S������System
  * * �����Ώ�: UIComponent �����S�Ă�Entity
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

	/// @brief UI�v�f��`�悷��
	void Draw();
};

#endif // !___UI_SYSTEM_H___