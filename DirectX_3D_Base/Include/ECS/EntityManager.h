/*****************************************************************//**
 * @file	EntityManager.h
 * @brief	ECS��Entity�̐����E�Ǘ����s���N���X�B
 * 
 * @details	
 * EntityID�̃v�[���Ǘ��ƁA�eEntity��Component Signature��ێ�����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ENTITY_MANAGER_H___
#define ___ENTITY_MANAGER_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include <queue>
#include <array>
#include <stdexcept>
#include <string>

namespace ECS
{
	/**
	 * @class EntityManager
	 * @brief EntityID�̐����A�ė��p�A�����Signature�̊Ǘ����s���B
	 */
	class EntityManager
	{
	private:
		// �ė��p�\��EntityID��ێ�����L���[�i�v�[�����O�j
		std::queue<EntityID> m_availableEntities;

		// EntityID�ɑΉ�����Component Signature�i�ǂ�Component�������������r�b�g�}�X�N�j
		std::array<Signature, MAX_ENTITIES> m_signatures;

		// ���݂܂łɍ쐬���ꂽEntity�̑���
		EntityID m_livingEntityCount = 0;

	public:
		EntityManager()
		{
			// MAX_ENTITIES�܂ł̑S�Ă�ID���A���p�\�ȃL���[�ɒǉ�����
			for (EntityID i = 0; i < MAX_ENTITIES; ++i)
			{
				m_availableEntities.push(i);
			}
		}

		/// @brief �V����EntityID�𐶐����A���蓖�Ă�
		EntityID CreateEntity()
		{
			// ���p�\��ID���c���Ă��邩�`�F�b�N
			if (m_livingEntityCount >= MAX_ENTITIES)
			{
				// TODO: �G���[�����B���O�ɏo�͂��ׂ�
				throw std::runtime_error("Error: Entity limit reached!");
			}

			// �L���[���痘�p�\��EntityID���擾
			EntityID id = m_availableEntities.front();
			m_availableEntities.pop();
			m_livingEntityCount++;

			// ������Ԃł�Signature���N���A���Ă���
			m_signatures[id].reset();

			return id;
		}

		/// @brief EntityID��j�����A�ė��p�L���[�ɖ߂�
		void DestroyEntity(EntityID entityID)
		{
			// �Ó���ID�͈͂��`�F�b�N
			if (entityID >= MAX_ENTITIES)
			{
				// TODO: �G���[����
				throw std::runtime_error("Error: Invalid entityID for destruction!");
			}

			// Signature���N���A����
			m_signatures[entityID].reset();

			// ID���ė��p�L���[�ɖ߂�
			m_availableEntities.push(entityID);
			m_livingEntityCount--;
		}

		/// @brief Entity��Component Signature��ݒ肷��
		void SetSignature(EntityID entityID, Signature signature)
		{
			// �Ó���ID�͈͂��`�F�b�N
			if (entityID >= MAX_ENTITIES)
			{
				// TODO: �G���[����
				throw std::runtime_error("Error: Invalid entityID for signature setting!");
			}

			m_signatures[entityID] = signature;
		}

		/// @brief Entity��Component Signature���擾����
		Signature GetSignature(EntityID entityID) const
		{
			// �Ó���ID�͈͂��`�F�b�N
			if (entityID >= MAX_ENTITIES)
			{
				// TODO: �G���[����
				throw std::runtime_error("Error: Invalid entityID for signature retrieval!");
			}

			return m_signatures[entityID];
		}
	};
}

#endif // !___ENTITY_MANAGER_H___