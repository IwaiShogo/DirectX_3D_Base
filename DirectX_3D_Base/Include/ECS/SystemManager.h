/*****************************************************************//**
 * @file	SystemManager.h
 * @brief	ECS��System���Ǘ�����}�l�[�W���N���X�B
 * 
 * @details	
 * System�̓o�^�ASignature�̐ݒ�A�����Entity��Signature���ύX���ꂽ�ۂ�
 * System�ւ̓o�^/�����������I�ɍs�����W�b�N����������B
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

#ifndef ___SYSTEM_MANAGER_H___
#define ___SYSTEM_MANAGER_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include <memory>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <typeindex>

namespace ECS
{
	class Coordinator;

	/**
	 * @class System
	 * @brief �S�Ă�System�̒��ۊ��N���X�B
	 * * System��EntityID�̃Z�b�g�im_entities�j�������ACoordinator�ɂ����
	 * Entity�̒ǉ��E�폜���ʒm����邱�Ƃł��̃Z�b�g���X�V�����B
	 */
	class System
	{
	public:
		/// @brief ����System���������ׂ�EntityID�̏W��
		std::set<EntityID> m_entities;

		/**
		 * @brief System�̏�������Coordinator�̈ˑ����������s��
		 * @param coordinator - Coordinator�C���X�^���X�ւ̐��|�C���^
		 */
		virtual void Init(Coordinator* coordinator) {}
	};

	/**
	 * @class SystemManager
	 * @brief System�̃C���X�^���X�ƁA����System���v������Component Signature���Ǘ�����B
	 */
	class SystemManager
	{
	private:
		// System�̌^���istd::type_index�j�� System�̃C���X�^���X�ւ̃|�C���^�̃}�b�s���O
		std::unordered_map<std::type_index, std::shared_ptr<System>> m_systems;

		// System�̌^���ƁA����System���v������Component Signature�̃}�b�s���O
		std::unordered_map<std::type_index, Signature> m_signatures;

	public:
		SystemManager() = default;

		/// @brief System��o�^���A�C���X�^���X�𐶐�����
		/// @tparam T - �o�^����System�̋�ی^�iSystem�N���X���p�����Ă��邱�Ɓj
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			std::type_index type = std::type_index(typeid(T));

			if (m_systems.count(type))
			{
				// ���ɓo�^�ς�
				throw std::runtime_error("Error: System T is already registered!");
			}

			// System�̃C���X�^���X�𐶐����A�}�b�v�Ɋi�[
			std::shared_ptr<T> system = std::make_shared<T>();
			m_systems[type] = system;
			return system;
		}

		/// @brief System���������ׂ�Entity��Component Signature��ݒ肷��
		/// @tparam T - System�̋�ی^
		/// @param signature - ����System���������邽�߂ɕK�v��Component�̃r�b�g�}�X�N
		template<typename T>
		void SetSignature(Signature signature)
		{
			std::type_index type = std::type_index(typeid(T));

			if (!m_systems.count(type))
			{
				throw std::runtime_error("Error: System T not registered before setting signature!");
			}

			// Signature��o�^
			m_signatures[type] = signature;
		}

		/// @brief Entity���쐬���ꂽ�A�܂���Component���ǉ�/�폜���ꂽ�ۂɌĂяo��
		/// @param entityID - �ύX��������Entity��ID
		/// @param entitySignature - �ύX���Entity��Component Signature
		void EntitySignatureChanged(EntityID entityID, Signature entitySignature)
		{
			// �S�Ă�System�ɑ΂��āAEntity��Signature���}�b�`���邩�`�F�b�N
			for (auto const& pair : m_systems)
			{
				// System�̌^������
				std::type_index type = pair.first;

				// System�̃C���X�^���X�|�C���^
				std::shared_ptr<System> system = pair.second;

				// System���v������Signature���擾
				Signature systemSignature = m_signatures[type];

				// System���������ׂ�Entity�ł��邩�ǂ����̔��胍�W�b�N
				// Entity��Signature��System�̗v������Signature��AND���Z���ʂ��A
				// System�̗v������Signature�Ɗ��S�Ɉ�v����ꍇ�ASystem�̏����ΏۂƂȂ�B
				// �܂�ASystem���K�v�Ƃ���Component��Entity�����ׂĎ����Ă���B
				if ((entitySignature & systemSignature) == systemSignature)
				{
					// �}�b�`����ꍇ: System�̏����Ώۂɒǉ�
					system->m_entities.insert(entityID);
				}
				else
				{
					// �}�b�`���Ȃ��ꍇ: System�̏����Ώۂ���폜�i���ɓo�^����Ă��Ă����S�j
					system->m_entities.erase(entityID);
				}
			}
		}

		/// @brief Entity���j�����ꂽ���Ƃ�S�Ă�System�ɒʒm����
		void EntityDestroyed(EntityID entityID)
		{
			// �S�Ă�System����EntityID���폜
			// System��Signature�Ƀ}�b�`���邩�ۂ��Ɋւ�炸�A�����I�ɍ폜����
			for (auto const& pair : m_systems)
			{
				pair.second->m_entities.erase(entityID);
			}
		}
	};
}

#endif // !___SYSTEM_MANAGER_H___