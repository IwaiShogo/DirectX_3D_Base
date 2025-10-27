/*****************************************************************//**
 * @file	GameScene.h
 * @brief	�Q�[���̃��C�����W�b�N���܂ރV�[���N���X�B
 * 
 * @details	
 * ECS�̏������A�Ǘ��ASystem�̎��s�����̃V�[���N���X���Ŋ���������B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/21	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS::Coordinator�̃C���X�^���X��RenderSystem�ւ̃|�C���^�������o�[�ɒǉ��B
 *						- �ǉ��F���̃V�X�e������Coordinator�ɃA�N�Z�X���邽�߂̐ÓI�A�N�Z�T�֐����`�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___GAME_SCENE_H___
#define ___GAME_SCENE_H___

// ===== �C���N���[�h =====
#include "Scene.h"
#include "ECS/Coordinator.h"

#include <memory>

// ===== �O���錾 =====
class RenderSystem;
class PhysicsSystem;
class PlayerControlSystem;
class CollisionSystem;
class CameraControlSystem;

 /**
  * @class GameScene
  * @brief ���ۂ̃Q�[�����W�b�N��ECS���Ǘ�����V�[��
  */
class GameScene
	: public Scene
{
private:
	// ECS�̒��S�ƂȂ�R�[�f�B�l�[�^�[ (�V�[����ECS�̃��C�t�T�C�N�����Ǘ�)
	std::unique_ptr<ECS::Coordinator> m_coordinator;

	// ��ɗ��p����System�ւ̎Q�Ƃ�ێ� (Update/Draw�̌Ăяo����e�Ղɂ���)
	std::shared_ptr<RenderSystem>			m_renderSystem;
	std::shared_ptr<PhysicsSystem>			m_physicsSystem;
	std::shared_ptr<PlayerControlSystem>	m_playerControlSystem;
	std::shared_ptr<CollisionSystem>		m_collisionSystem;
	std::shared_ptr<CameraControlSystem>	m_cameraControlSystem;

	// ECS�̃O���[�o���A�N�Z�X�p (System�Ȃǂ�ECS������s�����߂̑���)
	static ECS::Coordinator* s_coordinator;

public:
	// �R���X�g���N�^�ƃf�X�g���N�^�iScene���p�����Ă��邽�߉��z�f�X�g���N�^��Scene���Œ�`�ς݂Ɖ���j
	GameScene();
	~GameScene() override; // ���z�f�X�g���N�^������

	// Scene�C���^�[�t�F�[�X�̎���
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	/**
	 * @brief Coordinator�C���X�^���X�ւ̃|�C���^���擾����ÓI�A�N�Z�T
	 * @return ECS::Coordinator* - ���݃A�N�e�B�u�ȃV�[����Coordinator
	 */
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }
};

#endif // !___GAME_SCENE_H___