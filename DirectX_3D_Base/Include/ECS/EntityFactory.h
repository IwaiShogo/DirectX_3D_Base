/*****************************************************************//**
 * @file	EntityFactory.h
 * @brief	����̃G���e�B�e�B�i�v���C���[�A�n�ʂȂǁj�̐������W�b�N���W�񂷂�w���p�[�N���X�B
 * 
 * @details	
 * Scene::Init()�̃G���e�B�e�B�����R�[�h�𕪗����A�V�[���̐Ӗ����y������B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�G���e�B�e�B�������W�b�N�𕪗����邽�߂̃N���X���쐬
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ENTITY_FACTORY_H___
#define ___ENTITY_FACTORY_H___

// ===== �C���N���[�h =====
#include "Coordinator.h"
#include "Types.h"
#include <DirectXMath.h> // �R���|�[�l���g�̏����l�ݒ�ɕK�v

namespace ECS
{
	/**
	 * @class EntityFactory
	 * @brief Coordinator���󂯎��A��`�ς݂̃G���e�B�e�B�i�v���Z�b�g�j�𐶐�����ÓI�w���p�[
	 */
	class EntityFactory final
	{
	public:
		/**
		 * @brief �S�Ẵf���p�G���e�B�e�B�𐶐����AECS�ɓo�^����
		 * @param coordinator - �G���e�B�e�B�̐����Ɠo�^���s��Coordinator
		 */
		static void CreateAllDemoEntities(Coordinator* coordinator);

		/**
		 * @brief �v���C���[�G���e�B�e�B�𐶐�����
		 */
		static EntityID CreatePlayer(Coordinator* coordinator, const DirectX::XMFLOAT3& position);

		/**
		 * @brief �Q�[�����[���h�̐ÓI�Ȓn�ʃG���e�B�e�B�𐶐�����
		 */
		static EntityID CreateGround(Coordinator* coordinator, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale);

		/**
		 * @brief �J�����G���e�B�e�B�𐶐�����
		 */
		static EntityID CreateCamera(Coordinator* coordinator, EntityID focusID);

	private:
		// �ÓI�N���X�̂��߁A�v���C�x�[�g�R���X�g���N�^�ŃC���X�^���X�����֎~
		EntityFactory() = delete;
	};
}

#endif // !___ENTITY_FACTORY_H___