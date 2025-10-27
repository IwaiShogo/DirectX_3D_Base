/*****************************************************************//**
 * @file	CameraControlSystem.h
 * @brief	CameraComponent��RenderSystem��DrawSetup��A�g���A�J�����̒Ǐ]�Ɛݒ���s��System�B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS::System���p������ `CameraControlSystem` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___CAMERA_CONTROL_SYSTEM_H___
#define ___CAMERA_CONTROL_SYSTEM_H___

// ===== �C���N���[�h =====
// ECS
#include "ECS/Coordinator.h"
#include "ECS/SystemManager.h"
// Components
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
// Scene
#include "Scene/GameScene.h" 
#include "Main.h" // ��ʃT�C�Y�萔�ɃA�N�Z�X
#include <DirectXMath.h>

/**
 * @class CameraControlSystem
 * @brief �J�����̒Ǐ]���W�b�N�ƃr���[�E�v���W�F�N�V�����s��̌v�Z��S��
 * 
 * �����Ώ�: CameraComponent ������ Entity
 */
class CameraControlSystem : public ECS::System
{
private:
	ECS::Coordinator* m_coordinator;

	// ���݂̃J�����̈ʒu�ƒ����_�i�O�t���[���̌��ʂ�ێ����A��ԂɎg�p�j
	DirectX::XMFLOAT3 m_currentCameraPos;
	DirectX::XMFLOAT3 m_currentLookAt;

public:
	void Init()
	{
		m_coordinator = GameScene::GetCoordinator();
		// �����l�ݒ� (�f���`�掞�̏����ʒu�ɍ��킹��)
		m_currentCameraPos = DirectX::XMFLOAT3(0.0f, 3.5f, 5.0f);
		m_currentLookAt = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	/// @brief �J�����̈ʒu���v�Z���ARenderSystem�̃J�����ݒ�֐����Ăяo��
	void Update();
};

#endif // !___CAMERA_CONTROL_SYSTEM_H___