/*****************************************************************//**
 * @file	Component.h
 * @brief	Componentのデータ構造と管理ロジックのインターフェースを定義
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/15	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COMPONENT_H___
#define ___COMPONENT_H___

// ===== インクルード =====
#include "ECS.h" // コア定義とTransformComponentを参照

 // --------------------------------------------------
 // 1. Component ストレージの抽象化
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
    // ※ テンプレートクラスの実装はヘッダーに記述します
    ComponentArray()
    {
        mEntityToIndex.resize(MAX_ENTITIES);
        mIndexToEntity.resize(MAX_ENTITIES);
        mComponentArray.reserve(MAX_ENTITIES);
        mSize = 0;
    }

    /**
     * @brief 特定のEntityにComponentを追加します。
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
     * @brief 特定のEntityからComponentを削除します。
     */
    void RemoveData(EntityID entityID)
    {
        if (!HasData(entityID)) return;

        size_t indexOfRemoved = mEntityToIndex[entityID];
        size_t indexOfLast = mSize - 1;

        // データ移動（末尾を削除対象の位置に上書き）
        // NOTE: mComponentArray[indexOfRemoved] = mComponentArray[indexOfLast]; の前に
        // mComponentArrayにアクセス可能かチェックするか、mComponentArrayのサイズを保証する必要があります。
        // mComponentArray.resize(MAX_ENTITIES) をコンストラクタで実行していれば安全ですが、
        // 効率のためreserveを使っているため、ここではmComponentArrayが実際にmSize-1個の要素を持つことを前提とします。
        // InsertData()でcapacityを保証しています。

        // 最後の要素を削除対象の位置に移動
        mComponentArray[indexOfRemoved] = mComponentArray[indexOfLast];

        // マッピング更新: 入れ替えられたEntity (末尾だったEntity) のインデックスを更新
        EntityID entityOfLast = mIndexToEntity[indexOfLast];
        mEntityToIndex[entityOfLast] = indexOfRemoved;
        mIndexToEntity[indexOfRemoved] = entityOfLast;

        // 論理的にComponentを削除
        mSize--;

        // EntityID のマッピングをクリアする必要はありません。HasDataチェックで有効性を保証します。
    }

    /**
     * @brief 特定のEntityのComponentデータを取得します。
     */
    T& GetData(EntityID entityID)
    {
        if (!HasData(entityID)) {
            throw std::runtime_error("ComponentArray::GetData(): Component not found for this Entity.");
        }
        return mComponentArray[mEntityToIndex[entityID]];
    }

    /**
     * @brief EntityがこのComponentを持っているか確認します。
     */
    bool HasData(EntityID entityID) const
    {
        if (entityID >= MAX_ENTITIES) return false;

        size_t index = mEntityToIndex[entityID];

        // SparseSetの有効性チェック
        return index < mSize && mIndexToEntity[index] == entityID;
    }

    /**
     * @brief IComponentArrayのインターフェースを実装。
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

// ※ ComponentArray<T>のインライン実装（長いのでここでは省略しますが、Component.h内に記述します）


// --------------------------------------------------
// 2. Component Manager の宣言
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

    // 非テンプレート関数 (Component.cppで実装)
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