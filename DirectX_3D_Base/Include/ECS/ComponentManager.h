#ifndef ___COMPONENTMANAGER_H___
#define ___COMPONENTMANAGER_H___

// ===== インクルード =====
#include "Types.h"
#include <map>
#include <stdexcept>
#include <vector>
#include <string>

// --------------------------------------------------
// Base Component Pool (ComponentManagerが管理するコンポーネントデータの抽象化)
// --------------------------------------------------

class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    // Entityが破棄されたときに、そのEntityのComponentデータを削除する
    virtual void EntityDestroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray
{
public:
    // Sparse Set / Dense Set の実装:
    // T: Componentデータ本体
    // entityToIndex_: Entity IDからComponent Array内のインデックスを引く (Sparse)
    // indexToEntity_: Component Array内のインデックスからEntity IDを引く (Dense)
    // componentArray_: ComponentのデータをEntity IDの詰め替え順に並べた配列 (Dense)

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

        // 配列の末尾にComponentデータとEntity IDを追加
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

        // 削除する要素と最後の要素を入れ替える（O(1)の削除）
        size_t indexOfRemoved = entityToIndex_[entity];
        size_t indexOfLast = size_ - 1;

        // 最後の要素を削除する位置にコピー
        componentArray_[indexOfRemoved] = componentArray_[indexOfLast];

        // マッピング情報を更新
        Entity entityOfLast = indexToEntity_[indexOfLast];
        entityToIndex_[entityOfLast] = indexOfRemoved;
        indexToEntity_[indexOfRemoved] = entityOfLast;

        // 削除されたEntityの情報をクリーンアップ
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
    // Componentの型名とIDのマップ (例: "Transform" -> 0)
    std::map<const char*, ComponentType> componentTypes_;

    // Component Type IDとComponent Arrayのマップ
    std::map<ComponentType, std::shared_ptr<IComponentArray>> componentArrays_;

    // 次に割り当てるComponent Type ID
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

        // 型IDを割り当て
        componentTypes_.insert({ typeName, nextComponentType_ });

        // Component Arrayを生成し、Type IDに関連付けて格納
        componentArrays_.insert({ nextComponentType_, std::make_shared<ComponentArray<T>>() });

        ++nextComponentType_;
    }

    template<typename T>
    ComponentType GetComponentType()
    {
        const char* typeName = typeid(T).name();
        if (!componentTypes_.count(typeName))
        {
            // プロジェクトの安全性を考慮し、未登録の場合はエラーではなく登録を促す
            // 実際のゲーム開発ではRegisterComponentを必須とすべきだが、簡易化のためここでエラー
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

    /// @brief Entityが破壊されたときに、全てのComponent Arrayからデータを削除する
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
        // IComponentArray* を ComponentArray<T>* にダウンキャスト
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays_[type]);
    }
};

#endif // !___COMPONENTMANAGER_H___