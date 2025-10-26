/*****************************************************************//**
 * @file	ComponentManager.h
 * @brief	ECS��Component���Ǘ�����}�l�[�W���N���X�Q���`�B
 * 
 * @details	
 * ���ׂĂ�Component��ێ����钊�ۊ��N���X�ƁA
 * �ʂ�Component�^���������߂̃e���v���[�g�N���X�A
 * �����Ă����𓝍����ĊǗ�����ComponentManager���`����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FComponent�̒��ۃX�g���[�W�Ƌ�ۃX�g���[�W�A�����ComponentManager�N���X���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COMPONENT_MANAGER_H___
#define ___COMPONENT_MANAGER_H___

#include "Types.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace ECS
{
	/**
	 * @class IComponentArray
	 * @brief Component�̔z��i�X�g���[�W�j�𒊏ۉ�����C���^�[�t�F�[�X
	 * * ComponentManager���^�����������ɑ���ł���悤�ɂ��邽�߂̊��N���X�B
	 * EntityID�ɑ΂���Component�̒ǉ�/�폜�ƁACoordinator�ւ̒ʒm�@�\�����B
	 */
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		/// @brief �����Entity��Component���폜����i�^����Ȃ��j
		virtual void EntityDestroyed(EntityID entityID) = 0;
	};

	/**
	 * @class ComponentArray
	 * @brief �����Component�^ T ��ێ������ۃN���X
	 * * Component T �̃C���X�^���X�𖧏W�����z��ivector�j�Ƃ��ĕێ����邱�ƂŁA
	 * �V�X�e���������̃L���b�V�����������߂�B
	 * T�̃C���X�^���X�ƁAEntityID�̑Ή��t�����Ǘ�����B
	 * * @tparam T - �Ǘ�����Component�̌^
	 */
	template<typename T>
	class ComponentArray : public IComponentArray
	{
	private:
		// Component�C���X�^���X�̖��Ȕz��
		std::vector<T> m_componentArray;

		// EntityID����z��C���f�b�N�X�ւ̃}�b�s���O (Component�̍����Q�Ɨp)
		std::unordered_map<EntityID, size_t> m_entityToIndexMap;

		// �z��C���f�b�N�X����EntityID�ւ̃}�b�s���O (Component�̍����폜�p)
		std::unordered_map<size_t, EntityID> m_indexToEntityMap;

		// ���݂̔z��̗v�f��
		size_t m_size = 0;

	public:
		ComponentArray()
		{
			// MAX_ENTITIES�ɍ��킹�ė\�ߗ\�񂷂邱�ƂŁA���T�C�Y�ɂ��|�C���^�̖�������h��
			m_componentArray.resize(MAX_ENTITIES);
		}

		/// @brief Entity��Component��ǉ����A�����l��ݒ肷��
		/// @tparam Args - �R���|�[�l���gT�̃R���X�g���N�^����
		template<typename... Args>
		void AddComponent(EntityID entityID, Args&&... args)
		{
			// �G���e�B�e�B�����ɃR���|�[�l���g�������Ă���ꍇ�͗�O�𔭐�������
			if (m_entityToIndexMap.count(entityID))
			{
				// TODO: �G���[����������
				return;
			}

			// �z��̖����ɐV����Component��z�u
			size_t newIndex = m_size;

			// �C���v���[�X�ŃR���X�g���N�^�Ăяo��
			m_componentArray[newIndex] = T(std::forward<Args>(args)...);

			// �}�b�s���O�̍X�V
			m_entityToIndexMap[entityID] = newIndex;
			m_indexToEntityMap[newIndex] = entityID;

			m_size++;
		}

		/// @brief Entity����Component���폜����
		void RemoveComponent(EntityID entityID)
		{
			// �G���e�B�e�B���R���|�[�l���g�������Ă��Ȃ��ꍇ�͗�O�𔭐�������
			if (!m_entityToIndexMap.count(entityID))
			{
				// TODO: �G���[����������
				return;
			}

			// �폜�Ώۂ�Component�̃C���f�b�N�X���擾
			size_t indexOfRemoved = m_entityToIndexMap[entityID];

			// �z��̖����v�f���폜�Ώۂ̈ʒu�Ɉړ�������iO(1)�̍폜�������j
			// �������A�����v�f���폜�Ώێ��g�łȂ��ꍇ�̂�
			if (indexOfRemoved != m_size - 1)
			{
				// �Ō�̗v�f���폜�ʒu�Ɉړ�
				EntityID lastEntityID = m_indexToEntityMap[m_size - 1];
				m_componentArray[indexOfRemoved] = m_componentArray[m_size - 1];

				// �}�b�s���O�̍X�V
				m_entityToIndexMap[lastEntityID] = indexOfRemoved;
				m_indexToEntityMap[indexOfRemoved] = lastEntityID;
			}

			// �폜�Ώۂ�Entity�̃}�b�v�����N���A
			m_entityToIndexMap.erase(entityID);
			m_indexToEntityMap.erase(m_size - 1); // �����̃C���f�b�N�X���폜

			m_size--;
		}

		/// @brief Entity��Component���Q�ƂƂ��Ď擾����
		T& GetComponent(EntityID entityID)
		{
			// �G���e�B�e�B���R���|�[�l���g�������Ă��Ȃ��ꍇ�͗�O�𔭐�������
			if (!m_entityToIndexMap.count(entityID))
			{
				// TODO: �G���[����������
				// �G���[���O���o�͂��Ă���A�_�~�[��Component��Ԃ��A�܂��͗�O�𓊂���
			}

			// EntityID�ɑΉ�����z��C���f�b�N�X���擾���AComponent���Q��
			return m_componentArray[m_entityToIndexMap[entityID]];
		}

		/// @brief IComponentArray�̃C���^�[�t�F�[�X������ (Entity���j�����ꂽ�ۂ̏���)
		void EntityDestroyed(EntityID entityID) override
		{
			if (m_entityToIndexMap.count(entityID))
			{
				// �G���e�B�e�B������Component�������Ă���Ȃ�폜����
				RemoveComponent(entityID);
			}
		}

		/// @brief ComponentArray�S�̂ւ̃|�C���^���擾 (System�ł̌����I�ȃC�e���[�V�����p)
		T* GetArrayPointer()
		{
			return m_componentArray.data();
		}

		/// @brief Component�̐����擾
		size_t GetSize() const
		{
			return m_size;
		}

		/// @brief �C���f�b�N�X����EntityID���擾�iSystem�������Ɏg�p�j
		EntityID GetEntityIDFromIndex(size_t index) const
		{
			return m_indexToEntityMap.at(index);
		}
	};

	/**
	 * @class ComponentManager
	 * @brief ComponentArray �̃C���X�^���X�i�^�ʃX�g���[�W�j���Ǘ�����N���X
	 * * Coordinator����Ăяo����AComponent�̓o�^�A�ǉ��A�폜�̏������s���B
	 * ���ׂĂ�ComponentArray�� std::map<ComponentTypeID, std::shared_ptr<IComponentArray>> �ŊǗ�����B
	 */
	class ComponentManager
	{
	private:
		// Component�̌^�����ʂ��邽�߂�ID�J�E���^�[
		ComponentTypeID m_nextComponentTypeID = 0;

		// Component�̌^ID�ƁA���̌^�ʃX�g���[�W�ւ̃|�C���^�i���ۃN���X�j�̃}�b�s���O
		std::unordered_map<ComponentTypeID, std::shared_ptr<IComponentArray>> m_componentArrays;

		// Component�̌^���istd::type_index�j��ComponentTypeID�̃}�b�s���O
		std::unordered_map<std::type_index, ComponentTypeID> m_componentTypes;

		// �w���p�[�֐�: ComponentTypeID�𐶐�/�擾����
		template<typename T>
		ComponentTypeID GetTypeID()
		{
			// C++�̕W���@�\���g���āA�^ T �̈�ӂȎ��ʎq���擾
			std::type_index type = std::type_index(typeid(T));

			if (m_componentTypes.find(type) == m_componentTypes.end())
			{
				// �V�����^�Ȃ�ID�����蓖�Ăēo�^
				ComponentTypeID newID = m_nextComponentTypeID++;
				m_componentTypes[type] = newID;
				return newID;
			}

			// �����̌^�Ȃ�ID��Ԃ�
			return m_componentTypes[type];
		}

		// �w���p�[�֐�: �^���S��ComponentArray�̃|�C���^���擾
		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			ComponentTypeID typeID = GetTypeID<T>();

			// ���݂��Ȃ��ꍇ��nullptr��Ԃ��ׂ������ACoordinator�ŕK�����O��RegisterComponentType���Ă΂��O��Ƃ���
			if (m_componentArrays.find(typeID) == m_componentArrays.end())
			{
				// TODO: �G���[����
				//throw std::runtime_error("ComponentType T is not registered!");
			}

			// ���ۃ|�C���^�����ۃ|�C���^�ւ̃L���X�g�iComponentArray<T>�ւ̃_�E���L���X�g�j
			return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeID]);
		}

	public:
		ComponentManager() = default;

		/// @brief �V����Component�^���V�X�e���ɓo�^����
		template<typename T>
		void RegisterComponentType()
		{
			ComponentTypeID typeID = GetTypeID<T>();

			if (m_componentArrays.count(typeID))
			{
				// ���ɓo�^�ς�
				return;
			}

			// �V����ComponentArray�𐶐����A���ۃ}�b�v�Ɋi�[
			m_componentArrays[typeID] = std::make_shared<ComponentArray<T>>();
		}

		/// @brief Entity��Component��ǉ�����
		template<typename T, typename... Args>
		void AddComponent(EntityID entityID, Args&&... args)
		{
			// ComponentArray<T> ���擾���AAddComponent���Ăяo��
			GetComponentArray<T>()->AddComponent(entityID, std::forward<Args>(args)...);
		}

		/// @brief Entity����Component���폜����
		template<typename T>
		void RemoveComponent(EntityID entityID)
		{
			// ComponentArray<T> ���擾���ARemoveComponent���Ăяo��
			GetComponentArray<T>()->RemoveComponent(entityID);
		}

		/// @brief Entity��Component���Q�ƂƂ��Ď擾����
		template<typename T>
		T& GetComponent(EntityID entityID)
		{
			// ComponentArray<T> ���擾���AGetComponent���Ăяo��
			return GetComponentArray<T>()->GetComponent(entityID);
		}

		/// @brief �����Component��TypeID���擾����
		template<typename T>
		ComponentTypeID GetComponentTypeID()
		{
			return GetTypeID<T>();
		}

		/// @brief Entity���j�����ꂽ���Ƃ�S�Ă�ComponentArray�ɒʒm����
		void EntityDestroyed(EntityID entityID)
		{
			// �S�Ă�ComponentArray�ɑ΂���EntityID���폜����悤�ʒm
			for (auto const& pair : m_componentArrays)
			{
				pair.second->EntityDestroyed(entityID);
			}
		}
	};
}

#endif // !___COMPONENT_MANAGER_H___