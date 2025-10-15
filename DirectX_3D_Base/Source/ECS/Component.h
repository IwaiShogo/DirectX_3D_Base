/*****************************************************************//**
 * @file	Component.h
 * @brief	Component�̃f�[�^�\���ƊǗ����W�b�N�̃C���^�[�t�F�[�X���`
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/15	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COMPONENT_H___
#define ___COMPONENT_H___

// ===== �C���N���[�h =====
#include "ECS.h" // �R�A��`��TransformComponent���Q��

 // --------------------------------------------------
 // 1. Component �X�g���[�W�̒��ۉ�
 // --------------------------------------------------
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(EntityID entityID) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray
{
public:
    // �� �e���v���[�g�N���X�̎����̓w�b�_�[�ɋL�q���܂�
    ComponentArray()
    {
        mEntityToIndex.resize(MAX_ENTITIES);
        mIndexToEntity.resize(MAX_ENTITIES);
        mComponentArray.reserve(MAX_ENTITIES);
        mSize = 0;
    }

    /**
     * @brief �����Entity��Component��ǉ����܂��B
     */
    void InsertData(EntityID entityID, T component)
    {
        if (HasData(entityID)) {
            mComponentArray[mEntityToIndex[entityID]] = component;
            return;
        }

        if (mSize >= MAX_ENTITIES) {
            throw std::runtime_error("ComponentArray::InsertData(): Component array capacity exceeded MAX_ENTITIES.");
        }

        mComponentArray[mSize] = component;

        mEntityToIndex[entityID] = mSize;
        mIndexToEntity[mSize] = entityID;

        mSize++;
    }

    /**
     * @brief �����Entity����Component���폜���܂��B
     */
    void RemoveData(EntityID entityID)
    {
        if (!HasData(entityID)) return;

        size_t indexOfRemoved = mEntityToIndex[entityID];
        size_t indexOfLast = mSize - 1;

        // �f�[�^�ړ��i�������폜�Ώۂ̈ʒu�ɏ㏑���j
        // NOTE: mComponentArray[indexOfRemoved] = mComponentArray[indexOfLast]; �̑O��
        // mComponentArray�ɃA�N�Z�X�\���`�F�b�N���邩�AmComponentArray�̃T�C�Y��ۏ؂���K�v������܂��B
        // mComponentArray.resize(MAX_ENTITIES) ���R���X�g���N�^�Ŏ��s���Ă���Έ��S�ł����A
        // �����̂���reserve���g���Ă��邽�߁A�����ł�mComponentArray�����ۂ�mSize-1�̗v�f�������Ƃ�O��Ƃ��܂��B
        // InsertData()��capacity��ۏ؂��Ă��܂��B

        // �Ō�̗v�f���폜�Ώۂ̈ʒu�Ɉړ�
        mComponentArray[indexOfRemoved] = mComponentArray[indexOfLast];

        // �}�b�s���O�X�V: ����ւ���ꂽEntity (����������Entity) �̃C���f�b�N�X���X�V
        EntityID entityOfLast = mIndexToEntity[indexOfLast];
        mEntityToIndex[entityOfLast] = indexOfRemoved;
        mIndexToEntity[indexOfRemoved] = entityOfLast;

        // �_���I��Component���폜
        mSize--;

        // EntityID �̃}�b�s���O���N���A����K�v�͂���܂���BHasData�`�F�b�N�ŗL������ۏ؂��܂��B
    }

    /**
     * @brief �����Entity��Component�f�[�^���擾���܂��B
     */
    T& GetData(EntityID entityID)
    {
        if (!HasData(entityID)) {
            throw std::runtime_error("ComponentArray::GetData(): Component not found for this Entity.");
        }
        return mComponentArray[mEntityToIndex[entityID]];
    }

    /**
     * @brief Entity������Component�������Ă��邩�m�F���܂��B
     */
    bool HasData(EntityID entityID) const
    {
        if (entityID >= MAX_ENTITIES) return false;

        size_t index = mEntityToIndex[entityID];

        // SparseSet�̗L�����`�F�b�N
        return index < mSize && mIndexToEntity[index] == entityID;
    }

    /**
     * @brief IComponentArray�̃C���^�[�t�F�[�X�������B
     */
    void EntityDestroyed(EntityID entityID) override
    {
        RemoveData(entityID);
    }

private:
    std::vector<T> mComponentArray;
    std::vector<size_t> mEntityToIndex;
    std::vector<EntityID> mIndexToEntity;
    size_t mSize;
};

// �� ComponentArray<T>�̃C�����C�������i�����̂ł����ł͏ȗ����܂����AComponent.h���ɋL�q���܂��j


// --------------------------------------------------
// 2. Component Manager �̐錾
// --------------------------------------------------
class ComponentManager
{
public:
    template<typename T>
    void RegisterComponent()
    {
        ComponentTypeID typeID = ComponentTypeCounter::GetID<T>();
        if (mComponentArrays.count(typeID)) return;
        mComponentArrays.insert({ typeID, std::make_shared<ComponentArray<T>>() });
    }

    template<typename T>
    void AddComponent(EntityID entityID, T component)
    {
        GetComponentArray<T>()->InsertData(entityID, component);
    }

    template<typename T>
    void RemoveComponent(EntityID entityID)
    {
        GetComponentArray<T>()->RemoveData(entityID);
    }

    template<typename T>
    T& GetComponent(EntityID entityID)
    {
        return GetComponentArray<T>()->GetData(entityID);
    }

    // ��e���v���[�g�֐� (Component.cpp�Ŏ���)
    void EntityDestroyed(EntityID entityID);

private:
    std::unordered_map<ComponentTypeID, std::shared_ptr<IComponentArray>> mComponentArrays;

    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray()
    {
        ComponentTypeID typeID = ComponentTypeCounter::GetID<T>();

        if (mComponentArrays.find(typeID) == mComponentArrays.end()) {
            throw std::runtime_error("ComponentManager::GetComponentArray(): Component not registered.");
        }

        return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[typeID]);
    }
};

#endif // !___COMPONENT_H___