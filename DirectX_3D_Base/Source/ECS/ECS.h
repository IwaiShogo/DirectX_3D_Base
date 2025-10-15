/*****************************************************************//**
 * @file	ECS.h
 * @brief	ECSコア構造と設計の実装
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

#ifndef ___ECS_H___
#define ___ECS_H___


#include <cstdint>
#include <bitset>
#include <vector>
#include <memory> // std::shared_ptr/unique_ptr を使用する場合に備えて

// --------------------------------------------------
// ECS コア定義 (Core Definitions)
// --------------------------------------------------

/**
 * @brief Entityを一意に識別するIDの型。
 * * 32ビット整数を使用。最大約40億のEntityを識別可能。
 */
using EntityID = std::uint32_t;

/**
 * @brief Componentの型を一意に識別するIDの型。
 * * 8ビット整数を使用。最大256種類のComponentを許可。
 */
using ComponentTypeID = std::uint8_t;

/**
 * @brief 許容するComponentの最大数。Signatureのビット数となります。
 * * 32個のComponentを用意します。
 */
constexpr std::size_t MAX_COMPONENTS = 32;

/**
 * @brief EntityがどのComponentを持っているかを示すビットセット。
 */
using Signature = std::bitset<MAX_COMPONENTS>;


// --------------------------------------------------
// Component Type ID ジェネレーター
// --------------------------------------------------

/**
 * @brief C++のテンプレートメタプログラミングを利用し、各Component型に一意なIDを割り当てるクラス。
 */
class ComponentTypeCounter
{
private:
    static ComponentTypeID nextID;
public:
    /**
     * @brief 特定のComponent型Tに割り当てられたIDを取得します。
     * @tparam T Componentの型
     * @return ComponentTypeID
     */
    template<typename T>
    inline static ComponentTypeID GetID() noexcept
    {
        // 静的変数で、型Tごとに一度だけインクリメントされたIDを保持
        static ComponentTypeID typeID = nextID++;

        // 開発時の安全性の確保: IDがMAX_COMPONENTSを超えていないかチェック
        if (typeID >= MAX_COMPONENTS) {
            // エラー処理（ここではコンソール出力としています）
#ifdef _DEBUG
            std::cerr << "ECS ERROR: Component count exceeded MAX_COMPONENTS (" << MAX_COMPONENTS << ")" << std::endl;
#endif
            // 実行時エラーや例外スローも検討できますが、ここでは最小構成として警告に留めます
        }

        return typeID;
    }
};

// 静的メンバ変数の実体化（ComponentTypeIDの連番を管理）
// ComponentTypeID ComponentTypeCounter::nextID = 0; // 実装ファイルに定義すべきですが、ヘッダーオンリーのために仮定義


// --------------------------------------------------
// Systemの抽象基底クラス (次ステップへの準備)
// --------------------------------------------------

/**
 * @brief 全てのSystemの抽象基底クラス。
 */
class System
{
public:
    // このSystemが処理対象とするEntityのComponent構成
    Signature componentSignature;

    // このSystemが処理するEntityのIDリスト
    std::vector<EntityID> entities;

    /**
     * @brief 毎フレーム実行されるSystemのロジック。
     */
    virtual void Update(float deltaTime) = 0;

    // 仮想デストラクタは必須
    virtual ~System() = default;
};

// --------------------------------------------------
// Component ストレージの抽象化 (Component Storage Abstraction)
// --------------------------------------------------

/**
 * @brief Component配列のインターフェースを抽象化する基底クラス。
 * ComponentManagerが型に依存せずComponent配列を操作できるようにします。
 */
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;

    /**
     * @brief Entityが破棄された際、そのEntityに紐づいたComponentデータを配列から削除します。
     * どの型（T）であっても、EntityIDに基づいて削除できるように抽象化します。
     * @param entityID 破棄されるEntityのID
     */
    virtual void EntityDestroyed(EntityID entityID) = 0;
};


/**
 * @brief 特定のComponent型Tのデータを効率的に保持するクラス。
 * Sparse Set（疎行列セット）の原理に基づき、Entity IDと配列インデックスを相互参照します。
 * @tparam T 格納するComponentの型
 */
template<typename T>
class ComponentArray : public IComponentArray
{
public:
    ComponentArray()
    {
        // MAX_ENTITIESはCoordinatorで定義されるべきですが、ここでは仮で最大Entity数としています。
        // Entity IDからComponent配列のインデックスへのマッピングを確保
        mEntityToIndex.resize(MAX_ENTITIES);
        // Component配列のインデックスからEntity IDへのマッピングを確保
        mIndexToEntity.resize(MAX_ENTITIES);
        // mSizeは現在有効なComponentの数を保持（0から開始）
        mSize = 0;
    }

    /**
     * @brief 特定のEntityにComponentを追加します。
     * @param entityID 対象EntityのID
     * @param component 追加するComponentのデータ
     */
    void InsertData(EntityID entityID, T component)
    {
        if (HasData(entityID)) {
            // 既にComponentが存在する場合の処理（ここでは上書きとしています）
            mComponentArray[mEntityToIndex[entityID]] = component;
            return;
        }

        // 新しいComponentを配列の末尾に追加
        mComponentArray[mSize] = component;

        // マッピングの更新: ID -> Index, Index -> ID
        mEntityToIndex[entityID] = mSize;
        mIndexToEntity[mSize] = entityID;

        mSize++;
    }

    /**
     * @brief 特定のEntityからComponentを削除します。
     * 削除対象のComponentを配列の末尾のComponentと入れ替え、末尾を論理的に削除（サイズを減らす）します。
     * @param entityID 対象EntityのID
     */
    void RemoveData(EntityID entityID)
    {
        if (!HasData(entityID)) {
            // Componentが存在しない場合は何もしない
            return;
        }

        // 削除対象のComponentが存在する配列上のインデックス
        size_t indexOfRemoved = mEntityToIndex[entityID];
        // 最後のComponentが格納されているインデックス
        size_t indexOfLast = mSize - 1;

        // データの移動: 削除対象を配列末尾のComponentで上書き
        mComponentArray[indexOfRemoved] = mComponentArray[indexOfLast];

        // マッピングの更新: 入れ替えられたEntity (末尾だったEntity) のインデックスを更新
        EntityID entityOfLast = mIndexToEntity[indexOfLast];
        mEntityToIndex[entityOfLast] = indexOfRemoved;
        mIndexToEntity[indexOfRemoved] = entityOfLast;

        // 論理的にComponentを削除（サイズを1減らす）
        mSize--;

        // 削除されたEntityIDのマッピング情報は無効化（ここではクリアせず、Coordinatorでリセットされることを期待）
    }

    /**
     * @brief 特定のEntityのComponentデータを取得します。
     * @param entityID 対象EntityのID
     * @return T& Componentへの参照
     */
    T& GetData(EntityID entityID)
    {
        // 開発時チェック
        if (!HasData(entityID)) {
            // エラー処理（ここでは例外をスロー）
            throw std::runtime_error("ComponentArray::GetData(): Component not found for this Entity.");
        }
        return mComponentArray[mEntityToIndex[entityID]];
    }

    /**
     * @brief EntityがこのComponentを持っているか確認します。
     */
    bool HasData(EntityID entityID) const
    {
        // Entity IDが配列の範囲外でないこと
        if (entityID >= MAX_ENTITIES) return false;

        // Entity IDがマップされているインデックスが、現在有効なComponent配列のサイズ内にあること
        size_t index = mEntityToIndex[entityID];
        return index < mSize && mIndexToEntity[index] == entityID;
    }

    /**
     * @brief IComponentArrayのインターフェースを実装。
     */
    void EntityDestroyed(EntityID entityID) override
    {
        // Entityが破棄されたとき、このComponentが存在すれば削除する
        RemoveData(entityID);
    }

private:
    // 密な配列 (Dense Array): Componentの実データが連続して格納されます。
    std::vector<T> mComponentArray;

    // Entity ID -> ComponentArrayインデックスへの疎なマッピング (Sparse Map)
    // Entity IDがそのまま配列のインデックスとして機能します。
    std::vector<size_t> mEntityToIndex;

    // ComponentArrayインデックス -> Entity IDへのマッピング (Dense Map)
    // データの入れ替え時に、どのEntityのインデックスを更新すべきかを知るために使用します。
    std::vector<EntityID> mIndexToEntity;

    // 現在Component配列に存在するComponentの数
    size_t mSize;
};

// --------------------------------------------------
// Component Manager
// --------------------------------------------------

/**
 * @brief 全てのComponentArrayインスタンスを管理し、Coordinatorにインターフェースを提供します。
 */
class ComponentManager
{
public:
    /**
     * @brief 新しいComponent型TをECSに登録します。
     * @tparam T Componentの型
     */
    template<typename T>
    void RegisterComponent()
    {
        ComponentTypeID typeID = ComponentTypeCounter::GetID<T>();
        // すでに登録されていないかチェック
        if (mComponentArrays.count(typeID)) {
            // 既に登録済み
            return;
        }

        // 新しいComponent配列を作成し、マップに登録
        mComponentArrays.insert({ typeID, std::make_shared<ComponentArray<T>>() });
    }

    /**
     * @brief EntityにComponentを追加します。
     * @tparam T Componentの型
     * @param entityID 対象EntityのID
     * @param component Componentの実データ
     */
    template<typename T>
    void AddComponent(EntityID entityID, T component)
    {
        GetComponentArray<T>()->InsertData(entityID, component);
    }

    /**
     * @brief EntityからComponentを削除します。
     * @tparam T Componentの型
     * @param entityID 対象EntityのID
     */
    template<typename T>
    void RemoveComponent(EntityID entityID)
    {
        GetComponentArray<T>()->RemoveData(entityID);
    }

    /**
     * @brief EntityのComponentを取得します。
     * @tparam T Componentの型
     * @param entityID 対象EntityのID
     * @return T& Componentデータへの参照
     */
    template<typename T>
    T& GetComponent(EntityID entityID)
    {
        return GetComponentArray<T>()->GetData(entityID);
    }

    /**
     * @brief Entityが破棄されたとき、全てのComponent配列からComponentを削除するように通知します。
     * @param entityID 破棄されるEntityのID
     */
    void EntityDestroyed(EntityID entityID)
    {
        // 全てのComponent配列に対して、破棄されたEntityのデータ削除を要求
        for (auto const& pair : mComponentArrays)
        {
            pair.second->EntityDestroyed(entityID);
        }
    }

private:
    // Component Type ID -> Component Array (IComponentArray) のマッピング
    std::unordered_map<ComponentTypeID, std::shared_ptr<IComponentArray>> mComponentArrays;

    /**
     * @brief ComponentArray<T>のインスタンスをダウンキャストして取得するヘルパー関数。
     */
    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray()
    {
        ComponentTypeID typeID = ComponentTypeCounter::GetID<T>();

        // 実行時チェック: Componentが登録されているか
        if (mComponentArrays.find(typeID) == mComponentArrays.end()) {
            throw std::runtime_error("ComponentManager::GetComponentArray(): Component not registered.");
        }

        // 基底クラス(IComponentArray)から派生クラス(ComponentArray<T>)へキャスト
        return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[typeID]);
    }
};

// --------------------------------------------------
// System Manager
// --------------------------------------------------

/**
 * @brief 全てのSystemを管理し、EntityのSignature変更に応じて、
 * どのSystemがどのEntityを処理すべきかを決定します。
 */
class SystemManager
{
public:
    /**
     * @brief 新しいSystem型Tを登録し、そのインスタンスを返します。
     * @tparam T Systemの型（例: RenderSystem, PhysicsSystem）
     * @return std::shared_ptr<T> 登録されたSystemインスタンス
     */
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        // Systemの型名は文字列として取得
        const char* typeName = typeid(T).name();

        // 既に登録されていないかチェック
        if (mSystems.count(typeName)) {
            // 既に登録済み
            return std::static_pointer_cast<T>(mSystems[typeName]);
        }

        // Systemのインスタンスを作成
        std::shared_ptr<T> system = std::make_shared<T>();
        mSystems.insert({ typeName, system });
        return system;
    }

    /**
     * @brief Systemが処理対象とするComponentの構成 (Signature) を設定します。
     * @tparam T Systemの型
     * @param signature Systemが持つべきComponentのビットマスク
     */
    template<typename T>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(T).name();

        // Systemが登録されているかチェック
        if (mSystems.find(typeName) == mSystems.end()) {
            throw std::runtime_error("SystemManager::SetSignature(): System not registered before setting Signature.");
        }

        // SystemインスタンスにSignatureを格納
        mSignatures.insert({ typeName, signature });
        // System::componentSignature にも格納 (Systemが自身のSignatureを参照できるように)
        mSystems[typeName]->componentSignature = signature;
    }

    /**
     * @brief EntityのSignatureが変更されたとき、SystemのEntityリストを更新します。
     * EntityにComponentが追加・削除された際にCoordinatorから呼び出されます。
     * @param entityID 変更されたEntityのID
     * @param entitySignature Entityの新しいComponent Signature
     */
    void EntitySignatureChanged(EntityID entityID, Signature entitySignature)
    {
        // 全てのSystemに対してチェックを実行
        for (auto const& pair : mSystems)
        {
            const char* typeName = pair.first;
            std::shared_ptr<System> system = pair.second;

            // Systemが要求するSignatureを取得
            Signature systemSignature = mSignatures[typeName];

            // EntityのSignatureがSystemのSignatureを包含するかどうか (AND演算)
            // (entitySignature & systemSignature) == systemSignature
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // EntityがSystemの要求を満たしている場合 (SystemがEntityを処理すべき場合)

                // Entityが既にリストに含まれていないか確認
                auto& entities = system->entities;
                bool alreadyIn = false;
                for (EntityID id : entities) {
                    if (id == entityID) {
                        alreadyIn = true;
                        break;
                    }
                }

                if (!alreadyIn)
                {
                    // リストに追加
                    entities.push_back(entityID);
                }
            }
            else
            {
                // EntityがSystemの要求を満たさなくなった場合

                // リストから削除（イテレータを使用した効率的な削除）
                auto& entities = system->entities;
                for (auto it = entities.begin(); it != entities.end(); ++it)
                {
                    if (*it == entityID)
                    {
                        entities.erase(it);
                        break; // Entityはリストに最大1回しか存在しない
                    }
                }
            }
        }
    }

    /**
     * @brief Entityが破棄されたとき、全てのSystemのEntityリストから削除します。
     * @param entityID 破棄されるEntityのID
     */
    void EntityDestroyed(EntityID entityID)
    {
        // 全てのSystemのEntityリストからIDを削除
        for (auto const& pair : mSystems)
        {
            auto& entities = pair.second->entities;
            for (auto it = entities.begin(); it != entities.end(); ++it)
            {
                if (*it == entityID)
                {
                    entities.erase(it);
                    break;
                }
            }
        }
    }

    // SystemのUpdateを呼び出すためのアクセサ（Coordinatorが使用）
    const std::unordered_map<const char*, std::shared_ptr<System>>& GetSystems() const
    {
        return mSystems;
    }


private:
    // Systemの型名 -> Systemのインスタンスへのマッピング
    // std::unordered_mapのキーとして`const char*`を使う場合、カスタムハッシュ・比較関数が必要ですが、
    // `typeid(T).name()`はコンパイル時定数ではないため、ここでは簡略化のためこのまま進めます。
    // (実用的な実装では、`std::string`をキーとするか、カスタムハッシュを使用することが推奨されます)
    std::unordered_map<const char*, std::shared_ptr<System>> mSystems;

    // Systemの型名 -> Systemが要求するComponentのSignature
    std::unordered_map<const char*, Signature> mSignatures;
};

// --------------------------------------------------
// Entity Manager
// --------------------------------------------------

/**
 * @brief Entity IDの生成と再利用を管理します。
 */
class EntityManager
{
public:
    EntityManager()
    {
        // 最初のMAX_ENTITIES個のIDを予約キューに追加
        for (EntityID i = 0; i < MAX_ENTITIES; ++i)
        {
            mAvailableEntities.push(i);
        }
    }

    /**
     * @brief 新しいEntity IDを生成します。
     * @return EntityID 利用可能なEntity ID
     */
    EntityID CreateEntity()
    {
        // 利用可能なIDが残っているかチェック
        if (mAvailableEntities.empty()) {
            throw std::runtime_error("EntityManager::CreateEntity(): Max entities reached.");
        }

        // キューからIDを取り出し
        EntityID id = mAvailableEntities.front();
        mAvailableEntities.pop();

        // Entityが持つComponent Signatureをリセット
        mEntitySignatures[id].reset();

        mLivingEntityCount++;
        return id;
    }

    /**
     * @brief Entity IDを再利用のために解放します。
     * @param entityID 解放するEntityのID
     */
    void DestroyEntity(EntityID entityID)
    {
        // IDの有効性をチェック
        if (entityID >= MAX_ENTITIES) {
            throw std::runtime_error("EntityManager::DestroyEntity(): Entity ID out of range.");
        }

        // Entityが持つComponent Signatureをリセット（ComponentManagerへの通知はCoordinatorが行う）
        mEntitySignatures[entityID].reset();

        // IDを再利用キューに戻す
        mAvailableEntities.push(entityID);
        mLivingEntityCount--;
    }

    /**
     * @brief EntityのComponent Signatureを設定します。
     */
    void SetSignature(EntityID entityID, Signature signature)
    {
        // IDの有効性をチェック
        if (entityID >= MAX_ENTITIES) {
            throw std::runtime_error("EntityManager::SetSignature(): Entity ID out of range.");
        }
        mEntitySignatures[entityID] = signature;
    }

    /**
     * @brief EntityのComponent Signatureを取得します。
     */
    Signature GetSignature(EntityID entityID) const
    {
        // IDの有効性をチェックはCreateEntity時に確保されているものとします。
        return mEntitySignatures[entityID];
    }

    /**
     * @brief 現在の生存Entity数を取得します。
     */
    std::uint32_t GetLivingEntityCount() const
    {
        return mLivingEntityCount;
    }


private:
    // 再利用可能なEntity IDを格納するキュー
    std::queue<EntityID> mAvailableEntities;

    // 全てのEntityの現在のComponent Signature
    std::vector<Signature> mEntitySignatures{ MAX_ENTITIES };

    // 現在生存しているEntityの数
    std::uint32_t mLivingEntityCount{};
};

// --------------------------------------------------
// Coordinator (World)
// --------------------------------------------------

/**
 * @brief Entity, Component, Systemの各Managerを統合し、ECSの操作インターフェースを提供するクラス。
 */
class Coordinator
{
public:
    Coordinator()
    {
        // 全てのManagerを初期化
        mEntityManager = std::make_unique<EntityManager>();
        mComponentManager = std::make_unique<ComponentManager>();
        mSystemManager = std::make_unique<SystemManager>();
    }

    // ------------------------------------
    // Entity の管理
    // ------------------------------------
    EntityID CreateEntity()
    {
        return mEntityManager->CreateEntity();
    }

    void DestroyEntity(EntityID entityID)
    {
        // 1. Component Managerに通知：Componentデータを削除
        mComponentManager->EntityDestroyed(entityID);

        // 2. System Managerに通知：SystemのEntityリストから削除
        mSystemManager->EntityDestroyed(entityID);

        // 3. Entity Managerに通知：IDを解放
        mEntityManager->DestroyEntity(entityID);
    }

    // ------------------------------------
    // Component の管理
    // ------------------------------------
    template<typename T>
    void RegisterComponent()
    {
        mComponentManager->RegisterComponent<T>();
    }

    template<typename T>
    void AddComponent(EntityID entityID, T component)
    {
        mComponentManager->AddComponent<T>(entityID, component);

        // EntityのSignatureを更新し、System Managerに通知
        Signature signature = mEntityManager->GetSignature(entityID);
        signature.set(ComponentTypeCounter::GetID<T>());
        mEntityManager->SetSignature(entityID, signature);

        mSystemManager->EntitySignatureChanged(entityID, signature);
    }

    template<typename T>
    void RemoveComponent(EntityID entityID)
    {
        mComponentManager->RemoveComponent<T>(entityID);

        // EntityのSignatureを更新し、System Managerに通知
        Signature signature = mEntityManager->GetSignature(entityID);
        signature.set(ComponentTypeCounter::GetID<T>(), false); // ビットをOFF
        mEntityManager->SetSignature(entityID, signature);

        mSystemManager->EntitySignatureChanged(entityID, signature);
    }

    template<typename T>
    T& GetComponent(EntityID entityID)
    {
        return mComponentManager->GetComponent<T>(entityID);
    }

    // ------------------------------------
    // System の管理
    // ------------------------------------
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        return mSystemManager->RegisterSystem<T>();
    }

    template<typename T>
    void SetSystemSignature(Signature signature)
    {
        mSystemManager->SetSignature<T>(signature);
    }

    // Systemの実行が必要な際にメインループから呼び出すアクセサ
    const std::unordered_map<const char*, std::shared_ptr<System>>& GetSystems() const
    {
        return mSystemManager->GetSystems();
    }


private:
    std::unique_ptr<EntityManager> mEntityManager;
    std::unique_ptr<ComponentManager> mComponentManager;
    std::unique_ptr<SystemManager> mSystemManager;
};

// Coordinatorをグローバルまたはシングルトンとしてアクセス可能にするための宣言をここで追加することも検討できます。
// 例: extern Coordinator gCoordinator;

#endif // !___ECS_H___