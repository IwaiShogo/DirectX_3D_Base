#ifndef ___COMPONENTMANAGER_H___
#define ___COMPONENTMANAGER_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include <map>
#include <stdexcept>
#include <vector>
#include <string>

// --------------------------------------------------
// Base Component Pool (ComponentManager���Ǘ�����R���|�[�l���g�f�[�^�̒��ۉ�)
// --------------------------------------------------

class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    // Entity���j�����ꂽ�Ƃ��ɁA����Entity��Component�f�[�^���폜����
    virtual void EntityDestroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray
{
public:
    // Sparse Set / Dense Set �̎���:
    // T: Component�f�[�^�{��
    // entityToIndex_: Entity ID����Component Array���̃C���f�b�N�X������ (Sparse)
    // indexToEntity_: Component Array���̃C���f�b�N�X����Entity ID������ (Dense)
    // componentArray_: Component�̃f�[�^��Entity ID�̋l�ߑւ����ɕ��ׂ��z�� (Dense)

    std::array<T, MAX_ENTITIES> componentArray_;
    std::map<Entity, size_t> entityToIndex_;
    std::array<Entity, MAX_ENTITIES> indexToEntity_;
    size_t size_;

public:
    ComponentArray() : size_(0) {}

    void InsertData(Entity entity, T component)
    {
        if (entityToIndex_.count(entity))
        {
            throw std::runtime_error("Error: Component already exists.");
        }

        // �z��̖�����Component�f�[�^��Entity ID��ǉ�
        size_t newIndex = size_;
        entityToIndex_[entity] = newIndex;
        indexToEntity_[newIndex] = entity;
        componentArray_[newIndex] = component;
        ++size_;
    }

    void RemoveData(Entity entity)
    {
        if (!entityToIndex_.count(entity))
        {
            throw std::runtime_error("Error: Attempting to remove non-existent component.");
        }

        // �폜����v�f�ƍŌ�̗v�f�����ւ���iO(1)�̍폜�j
        size_t indexOfRemoved = entityToIndex_[entity];
        size_t indexOfLast = size_ - 1;

        // �Ō�̗v�f���폜����ʒu�ɃR�s�[
        componentArray_[indexOfRemoved] = componentArray_[indexOfLast];

        // �}�b�s���O�����X�V
        Entity entityOfLast = indexToEntity_[indexOfLast];
        entityToIndex_[entityOfLast] = indexOfRemoved;
        indexToEntity_[indexOfRemoved] = entityOfLast;

        // �폜���ꂽEntity�̏����N���[���A�b�v
        entityToIndex_.erase(entity);
        --size_;
    }

    T& GetData(Entity entity)
    {
        if (!entityToIndex_.count(entity))
        {
            throw std::runtime_error("Error: Component not found for entity.");
        }
        return componentArray_[entityToIndex_[entity]];
    }

    void EntityDestroyed(Entity entity) override
    {
        if (entityToIndex_.count(entity))
        {
            RemoveData(entity);
        }
    }
};

// --------------------------------------------------
// Component Manager
// --------------------------------------------------

class ComponentManager
{
private:
    // Component�̌^����ID�̃}�b�v (��: "Transform" -> 0)
    std::map<const char*, ComponentType> componentTypes_;

    // Component Type ID��Component Array�̃}�b�v
    std::map<ComponentType, std::shared_ptr<IComponentArray>> componentArrays_;

    // ���Ɋ��蓖�Ă�Component Type ID
    ComponentType nextComponentType_ = 0;

public:
    template<typename T>
    void RegisterComponent()
    {
        const char* typeName = typeid(T).name();

        if (componentTypes_.count(typeName))
        {
            throw std::runtime_error("Error: Component type already registered.");
        }

        if (nextComponentType_ >= MAX_COMPONENTS)
        {
            throw std::runtime_error("Error: Exceeded maximum component types.");
        }

        // �^ID�����蓖��
        componentTypes_.insert({ typeName, nextComponentType_ });

        // Component Array�𐶐����AType ID�Ɋ֘A�t���Ċi�[
        componentArrays_.insert({ nextComponentType_, std::make_shared<ComponentArray<T>>() });

        ++nextComponentType_;
    }

    template<typename T>
    ComponentType GetComponentType()
    {
        const char* typeName = typeid(T).name();
        if (!componentTypes_.count(typeName))
        {
            // �v���W�F�N�g�̈��S�����l�����A���o�^�̏ꍇ�̓G���[�ł͂Ȃ��o�^�𑣂�
            // ���ۂ̃Q�[���J���ł�RegisterComponent��K�{�Ƃ��ׂ������A�ȈՉ��̂��߂����ŃG���[
            throw std::runtime_error("Error: Component type not registered. Call RegisterComponent<T>() first.");
        }
        return componentTypes_[typeName];
    }

    template<typename T>
    void AddComponent(Entity entity, T component)
    {
        GetComponentArray<T>()->InsertData(entity, component);
    }

    template<typename T>
    void RemoveComponent(Entity entity)
    {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template<typename T>
    T& GetComponent(Entity entity)
    {
        return GetComponentArray<T>()->GetData(entity);
    }

    /// @brief Entity���j�󂳂ꂽ�Ƃ��ɁA�S�Ă�Component Array����f�[�^���폜����
    void EntityDestroyed(Entity entity)
    {
        for (auto const& pair : componentArrays_)
        {
            pair.second->EntityDestroyed(entity);
        }
    }

private:
    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray()
    {
        ComponentType type = GetComponentType<T>();
        // IComponentArray* �� ComponentArray<T>* �Ƀ_�E���L���X�g
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays_[type]);
    }
};

#endif // !___COMPONENTMANAGER_H___