/*****************************************************************//**
 * @file	Coordinator.h
 * @brief	ECS�̂��ׂĂ̋@�\�𓝍�����Facade (����) �N���X�B
 * 
 * @details	
 * EntityManager, ComponentManager, SystemManager��3�����L���A
 * �O������̃A�N�Z�X�𒇉�邱�ƂŁAECS�̃��[������т��ēK�p����B
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

#ifndef ___COORDINATOR_H___
#define ___COORDINATOR_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

namespace ECS
{
	/**
	 * @class Coordinator
	 * @brief ECS�̒��S�ƂȂ�t�@�T�[�h�N���X�B
	 * * �S�Ă�Entity�AComponent�ASystem�̑���́A����Coordinator���o�R���čs����B
	 */
	class Coordinator
	{
	public:
		Coordinator() = default;

		// �}�l�[�W����Coordinator���ӔC�������ď������E�Ǘ�����
		std::unique_ptr<ComponentManager> m_componentManager;
		std::unique_ptr<EntityManager> m_entityManager;
		std::unique_ptr<SystemManager> m_systemManager;

		/// @brief ECS�V�X�e���̏�����
		void Init()
		{
			// �e�}�l�[�W���̏������i�C���X�^���X�����j
			m_componentManager = std::make_unique<ComponentManager>();
			m_entityManager = std::make_unique<EntityManager>();
			m_systemManager = std::make_unique<SystemManager>();
		}

		// =================================
		//       Component �Ǘ����\�b�h
		// =================================

		/// @brief �V����Component�^��o�^����
		template<typename T>
		void RegisterComponentType()
		{
			m_componentManager->RegisterComponentType<T>();
		}

		/// @brief Entity��Component��ǉ����A�����l��ݒ肷��
		template<typename T, typename... Args>
		void AddComponent(EntityID entityID, Args&&... args)
		{
			// 1. ComponentManager��Component�̒ǉ����˗�
			m_componentManager->template AddComponent(entityID, std::forward<Args>(args)...);

			// 2. Entity��Signature���X�V
			Signature signature = m_entityManager->GetSignature(entityID);
			signature.set(m_componentManager->GetComponentTypeID<T>());
			m_entityManager->SetSignature(entityID, signature);

			// 3. SystemManager��Signature�̕ύX��ʒm���ASystem�ւ̓o�^/�����𑣂�
			m_systemManager->EntitySignatureChanged(entityID, signature);
		}

		template<typename T>
		void AddComponent(EntityID entityID, T&& component)
		{
			// ComponentArray<T> ���擾���A���[�u����AddComponent���Ăяo��
			// ���̌Ăяo���� ComponentArray<T>::AddComponent<T&&> �Ɍq����
			m_componentManager->template AddComponent(entityID, std::forward<T>(component));
		}

		/// @brief Entity����Component���폜����
		template<typename T>
		void RemoveComponent(EntityID entityID)
		{
			// 1. ComponentManager����Component�̍폜���˗�
			m_componentManager->RemoveComponent<T>(entityID);

			// 2. Entity��Signature���X�V
			Signature signature = m_entityManager->GetSignature(entityID);
			signature.reset(m_componentManager->GetComponentTypeID<T>());
			m_entityManager->SetSignature(entityID, signature);

			// 3. SystemManager��Signature�̕ύX��ʒm���ASystem����̉����𑣂�
			m_systemManager->EntitySignatureChanged(entityID, signature);
		}

		/// @brief Entity��Component���Q�ƂƂ��Ď擾����
		template<typename T>
		T& GetComponent(EntityID entityID)
		{
			return m_componentManager->GetComponent<T>(entityID);
		}

		/// @brief �����Component��TypeID���擾����
		template<typename T>
		ComponentTypeID GetComponentTypeID()
		{
			return m_componentManager->GetComponentTypeID<T>();
		}

		// =================================
		//       Entity �Ǘ����\�b�h
		// =================================

		/// @brief �V����EntityID���擾����
		EntityID CreateEntity()
		{
			return m_entityManager->CreateEntity();
		}

		// --- �y�V�K�z�ꊇ�ǉ��w���p�[�̖{�́i�ċA�I�[�֐��j ---
		// �R���|�[�l���g���X�g����ɂȂ����Ƃ��ɌĂ΂��
		void AddComponentsInternal(EntityID entityID)
		{
			// �x�[�X�P�[�X�F�������Ȃ�
		}

		// --- �y�V�K�z�ꊇ�ǉ��w���p�[�̖{�́i�ċA�֐��j ---
		// �ŏ��̃R���|�[�l���g��ǉ����A�c����ċA�I�ɏ�������
		template<typename T, typename... Rest>
		void AddComponentsInternal(EntityID entityID, T&& component, Rest&&... rest)
		{
			// 1. ComponentManager��Component�C���X�^���X�̒ǉ����˗�
			// ComponentManager�Ɏ��������I�[�o�[���[�h���g�p
			m_componentManager->template AddComponent<T>(entityID, std::forward<T>(component));

			// 2. Entity��Signature���X�V�iAddComponent�̃��W�b�N���R�s�[�j
			Signature signature = m_entityManager->GetSignature(entityID);
			signature.set(m_componentManager->GetComponentTypeID<T>());
			m_entityManager->SetSignature(entityID, signature);

			// 3. SystemManager��Signature�̕ύX��ʒm
			m_systemManager->EntitySignatureChanged(entityID, signature);

			// �c��̃R���|�[�l���g�������i�ċA�Ăяo���j
			AddComponentsInternal(entityID, std::forward<Rest>(rest)...);
		}

		// --- �y�V�K�z�ꊇ���� + �ꊇ�ǉ��̃w���p�[�֐� (CreateEntity�Ɍq����C���[�W) ---
		/**
		 * @brief Entity�𐶐����A�ό̃R���|�[�l���g�C���X�^���X����x�ɒǉ�����B
		 * @tparam Components - �ǉ�����R���|�[�l���g�̌^���X�g�B
		 * @param components - �R���|�[�l���g�̃C���X�^���X�B
		 * @return EntityID - �������ꂽ�V����EntityID�B
		 */
		template<typename... Components>
		EntityID CreateEntity(Components&&... components)
		{
			EntityID entityID = CreateEntity(); // �܂�ID�𐶐�

			// �ċA�w���p�[�֐����Ăяo���A�S�ẴR���|�[�l���g��ǉ�
			AddComponentsInternal(entityID, std::forward<Components>(components)...);

			return entityID;
		}

		/// @brief Entity��j������
		void DestroyEntity(EntityID entityID)
		{
			// 1. SystemManager��Entity�̔j����ʒm
			m_systemManager->EntityDestroyed(entityID);

			// 2. ComponentManager��Entity�̎��S�Ă�Component�̍폜��ʒm
			m_componentManager->EntityDestroyed(entityID);

			// 3. EntityManager��ID�̍ė��p���˗�
			m_entityManager->DestroyEntity(entityID);
		}

		// =================================
		//       System �Ǘ����\�b�h
		// =================================

		/// @brief System��o�^���A�C���X�^���X���擾����
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			return m_systemManager->RegisterSystem<T>();
		}

		/// @brief System�������ΏۂƂ���Component Signature��ݒ肷��
		template<typename T>
		void SetSystemSignature(Signature signature)
		{
			m_systemManager->SetSignature<T>(signature);
		}
	};
}

#endif // !___COORDINATOR_H___