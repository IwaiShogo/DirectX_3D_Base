/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	TransformComponent��RenderComponent������Entity��`�悷��System�B
 * 
 * @details	
 * ���C�����[�v��Draw()����Ăяo����A�S�Ă̊Y��Entity�̃��[���h�s����v�Z���A
 * Geometory�N���X��p���ĕ`����s���B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS::System���p������ `RenderSystem` ���쐬�BUpdate/Draw�̃��\�b�h��ǉ��B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___RENDER_SYSTEM_H___
#define ___RENDER_SYSTEM_H___

// ===== �C���N���[�h =====
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
 * @brief Entity�̕`���S������System
 * * �����Ώ�: TransformComponent �� RenderComponent �����S�Ă�Entity
 */
class RenderSystem
	: public ECS::System
{
private:
	// ComponentManager�ւ̃A�N�Z�X�ȗ����̂��߁ACoordinator��ێ�
	ECS::Coordinator* m_coordinator;

public:
	// System�̏�����
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
	}

	/// @brief �`��ɕK�v�ȃJ�����ݒ�A�f�o�b�O�`��Ȃǂ��s��
	void DrawSetup();

	/// @brief RenderComponent������Entity��S�ĕ`�悷��
	void DrawEntities();
};

#endif // !___RENDER_SYSTEM_H___