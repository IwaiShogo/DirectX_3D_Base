/*****************************************************************//**
 * @file	World.h
 * @brief	ECSワールド管理システムとエンティティビルダーの定義
 * 
 * @details	このファイルは、ECSアーキテクチャの中核となるWorldクラスと、
 *			エンティティを簡単に作成するためのEntityBuilderクラスを定義します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/17	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___WORLD_H___
#define ___WORLD_H___

// ===== インクルード =====
#include "Entity.h"
#include "Component.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

// ===== 前方宣言 =====
class World;

/**
 * @class EntityBuilder
 * @brief エンティティ作成を簡単にするビルダーパターンクラス
 * 
 * @details
 * World::Create()によって返され、メソッドチェーンを使って、複数のコンポーネントを持つエンティティを
 * 簡潔に作成できます。Worldクラスと連携して動作します。
 * 
 * @par 使用例:
 * @code
 * // Entity player は Build() を省略しても EntityBuilder から暗黙的に変換される
 * Entity player = world.Create()
 * .With<Transform>(DirectX::XMFLOAT3{0, 0, 0}) // コンポーネントの初期値を指定
 * .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 * .With<Rotator>(45.0f); // Behaviourコンポーネントの追加も可能
 * @endcode
 * 
 * @see World エンティティとコンポーネントを管理するクラス
 */
class EntityBuilder
{
public:
    /**
     * @brief WorldクラスのCreate()からのみ呼ばれるコンストラクタ
     * @param[in,out] w Worldへの参照
     * @param[in] e 既に生成されたEntity
     */
    EntityBuilder(World& w, Entity e) : world_(w), entity_(e) {}

    /**
     * @brief エンティティにコンポーネントを追加し、初期化します。
     * @tparam T 追加するコンポーネント型 (IComponentを継承)
     * @param[in] args コンポーネントTのコンストラクタ引数
     * @return EntityBuilder& メソッドチェーンを継続するためのビルダー自身への参照
     * 
     * @par 使用例:
     * @code
     * .With<Transform>(pos, rot, scale)
     * @endcode
     */
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args)
    {
        // 1. WorldからT型のStoreを取得
        Store<T>* store = world_.GetStore<T>();

        // 2. Entityにコンポーネントを追加し、コンストラクタ引数で初期化
        T& component = store->Add(entity_, std::forward<Args>(args)...);

        // 3. Behaviourコンポーネントの場合は、WorldのBehaviourリストに登録
        if (std::is_base_of<Behaviour, T>::value) {
            // World内のprivate関数 registerBehaviour を呼び出す
            world_.registerBehaviour(entity_, static_cast<Behaviour*>(&component));
        }

        return *this;
    }

    /**
     * @brief ビルドを完了し、Entityを返します。
     * @return Entity 生成されたEntity
     */
    Entity Build()
    {
        return entity_;
    }

    /**
     * @brief EntityBuilderからEntityへの暗黙的な型変換オペレータ
     * * @details
     * これにより、Entity player = world.Create()...Build(); の Build() を省略できます。
     */
    operator Entity() const { return entity_; }

private:
    World& world_;      ///< Worldへの参照（コンポーネント追加に使用）
    Entity entity_;     ///< 現在構築中のEntity
};

// --------------------------------------------------
// 1. コンポーネントストアの基盤 (IStore, Store<T>)
// --------------------------------------------------

/**
 * @struct	IStore
 * @brief	全てのコンポーネントストアの抽象基底クラス
 * 
 * @details	Worldクラスが型を意識せずに（std::type_indexで）特定の
 *			コンポーネントストアへのポインタを保持できるように
 *			するためのインターフェースです。
 */
struct IStore
{
	virtual ~IStore() = default;

	/**
	 * [void - RemoveComponent]
	 * @brief	エンティティが破棄された際、そのコンポーネントをストアから削除する
	 * 
	 * @param	[in] e	削除対象のエンティティ 
	 */
	virtual void RemoveComponent(Entity e) = 0;
};

/**
 * @class Store<T>
 * @brief 特定のコンポーネント型Tを格納するストア
 * 
 * @tparam T IComponentを継承したコンポーネント型
 * @details
 * std::unordered_mapを使用してEntityIDとコンポーネントの実データを対応付けます。
 * これにより、コンポーネントの動的な追加・削除を柔軟に行えます。
 * 
 * @warning TはIComponentを継承している必要があります。
 */
template<typename T>
class Store : public IStore
{
public:
    /**
     * @brief 特定のエンティティにコンポーネントを追加します。
     * @param[in] e 対象エンティティ
     * @param[in] args コンポーネントTのコンストラクタ引数
     * @return T& 追加されたコンポーネントへの参照
     * 
     * @code
     * w.GetStore<Transform>()->Add(entity, position, rotation);
     * @endcode
     */
    template<typename... Args>
    T& Add(Entity e, Args&&... args)
    {
        // 配置newを使用して、既存のメモリ（もしあれば）を再利用するか、新しく構築
        auto result = data_.try_emplace(e.id, std::forward<Args>(args)...);

        // 既にコンポーネントが存在する場合は、上書きしています
        if (!result.second) {
            // 既存の要素のデストラクタを呼び出し、新しい要素を構築
            result.first->second.~T();
            new (&result.first->second) T(std::forward<Args>(args)...);
        }

        return result.first->second;
    }

    /**
     * @brief 特定のエンティティからコンポーネントを取得します。
     * @param[in] e 対象エンティティ
     * @return T& コンポーネントへの参照
     * 
     * @warning コンポーネントが存在しない場合、アサート（実行時エラー）が発生します。
     */
    T& Get(Entity e)
    {
        assert(Has(e) && "Component does not exist on this Entity!");
        return data_.at(e.id);
    }

    /**
     * @brief 特定のエンティティのコンポーネントが存在するかを確認します。
     */
    bool Has(Entity e) const
    {
        return data_.count(e.id);
    }

    /**
     * @brief 特定のエンティティからコンポーネントを削除します。
     */
    void Remove(Entity e)
    {
        data_.erase(e.id);
    }

    /**
     * @brief IStoreインターフェースの実装。エンティティ破棄時に呼ばれます。
     */
    void RemoveComponent(Entity e) override
    {
        Remove(e);
    }

    // 描画などのループでイテレーションするためにストア全体へのアクセスを提供
    const std::unordered_map<Entity::ID, T>& GetAll() const { return data_; }

private:
    // Entity ID -> コンポーネントデータ のマッピング
    std::unordered_map<Entity::ID, T> data_;
};


// --------------------------------------------------
// 2. World クラス (ECSの中核)
// --------------------------------------------------

/**
 * @class World
 * @brief エンティティとコンポーネントを管理するECSの中核クラス
 * 
 * @details
 * Worldクラスは、Entity IDのライフサイクル管理、全てのコンポーネントストア、
 * およびBehaviourコンポーネントの更新を担当します。
 * 
 * @par Worldへのアクセス:
 * SystemやBehaviourコンポーネントは、このWorldクラスを通じて
 * 他のEntityのコンポーネントにアクセスします。
 * 
 * @par 使用例:
 * @code
 * World world;
 * // 初期化はSetup()などで行う
 * world.Update(deltaTime);
 * @endcode
 * 
 * @see EntityBuilder エンティティ作成のためのビルダー
 */
class World
{
public:
    World() = default;
    ~World() { /* 全てのIStoreのポインタを削除する処理が必要 */ }

    // ------------------------------------
    // Entityのライフサイクル
    // ------------------------------------

    /**
     * @brief 新しいエンティティIDを生成します。
     * @return Entity 新しく作成されたエンティティ
     * 
     * @note この関数を直接使用する代わりに、Create()メソッドを使用し、
     * EntityBuilderを通じてコンポーネントを追加することを推奨します。
     */
    Entity CreateEntity()
    {
        Entity::ID id = ++nextId_;
        alive_[id] = true;
        return { id };
    }

    /**
     * @brief エンティティを破棄し、関連する全てのコンポーネントを削除します。
     * @param[in] e 破棄対象のエンティティ
     * 
     * @warning エンティティが有効（IsValid() == true）であることを確認してください。
     */
    void DestroyEntity(Entity e)
    {
        if (!e.IsValid() || !alive_.count(e.id)) return;

        // 1. 全てのIStoreからコンポーネントを削除
        for (const auto& func : erasers_) {
            func(e);
        }

        // 2. Behaviourコンポーネントをリストから削除
        unregisterBehaviour(e);

        // 3. IDを解放
        alive_.erase(e.id);
    }

    // ------------------------------------
    // Componentの操作
    // ------------------------------------

    /**
     * @brief 特定のコンポーネント型Tのストアを取得します。
     * @tparam T IComponentを継承したコンポーネント型
     * @return Store<T>* ストアへのポインタ
     * 
     * @warning コンポーネント型が未登録の場合、ストアを新規作成し、ポインタを返します。
     * これは通常、初期化時にのみ実行されるべきです。
     */
    template<typename T>
    Store<T>* GetStore()
    {
        std::type_index typeID = GetComponentTypeID<T>();

        if (stores_.count(typeID) == 0)
        {
            // ストアがまだ存在しない場合、新しく作成し、マップに登録
            Store<T>* newStore = new Store<T>(); // new でインスタンス化
            stores_[typeID] = newStore;

            // 削除用関数（イレイサー）も登録
            erasers_.push_back([this, typeID](Entity e) {
                // RemoveComponentを呼び出すためのラムダ関数
                auto it = stores_.find(typeID);
                if (it != stores_.end()) {
                    it->second->RemoveComponent(e);
                }
                });
            return newStore;
        }
        // ダウンキャストして返す
        return static_cast<Store<T>*>(stores_.at(typeID));
    }

    /**
     * @brief 特定のエンティティからコンポーネントを取得します。
     * @tparam T IComponentを継承したコンポーネント型
     * @param[in] e 対象エンティティ
     * @return T& コンポーネントへの参照
     * 
     * @warning コンポーネントが存在しない場合、アサート（実行時エラー）が発生します。
     */
    template<typename T>
    T& Get(Entity e)
    {
        return GetStore<T>()->Get(e);
    }

    /**
     * @brief 特定のエンティティからコンポーネントを取得します。存在しない場合はnullptrを返します。
     * @tparam T IComponentを継承したコンポーネント型
     * @param[in] e 対象エンティティ
     * @return T* コンポーネントへのポインタ。存在しない場合はnullptr。
     */
    template<typename T>
    T* TryGet(Entity e)
    {
        Store<T>* store = GetStore<T>();
        if (store->Has(e)) {
            return &store->Get(e);
        }
        return nullptr;
    }

    // ------------------------------------
    // Worldの更新ロジック (後のステップで実装)
    // ------------------------------------
    void Update(float dt)
    {
        // 1. WorldMatrixの計算
        // 2. Behaviour ComponentのOnStart/OnUpdate呼び出し (次のステップ)
    }

    /**
     * @brief 新しいEntityを生成し、ビルダーを開始します。
     * @return EntityBuilder Entity構築のためのビルダーインスタンス
     */
    EntityBuilder Create()
    {
        Entity e = CreateEntity();
        return { *this, e };
    }

    // ------------------------------------
// Worldの更新ロジック
// ------------------------------------

/**
 * @brief Worldに存在するすべてのBehaviourコンポーネントを更新します。
 * 
 * @param[in] dt デルタタイム（前フレームからの経過秒数）
 * 
 * @details
 * この関数はゲームのメインループから毎フレーム呼ばれるべきです。
 * 以下の処理をこの順序で実行します：
 * 1. WorldMatrixの計算（Transformの更新）
 * 2. 未実行のBehaviourコンポーネントに対してOnStart()を呼び出す
 * 3. 全てのBehaviourコンポーネントに対してOnUpdate(dt)を呼び出す
 * 
 * @par 使用例:
 * @code
 * // メインループ内
 * float deltaTime = timer.GetDeltaTime();
 * world.Update(deltaTime);
 * @endcode
 */
    void Update(float dt)
    {
        // 1. WorldMatrixの計算（★後のステップでRenderSystemと連携するが、ここではTransformコンポーネントのみを更新）
        // 現状の設計では、Transformの更新（回転など）はBehaviourコンポーネント（Rotatorなど）が担当し、
        // WorldMatrixの計算はRenderSystemのような専用システムが担当します。
        // しかし、RotatorのようなBehaviourコンポーネントが自身のTransformを更新した場合、
        // 描画前にWorldMatrixが更新される必要があります。

        // ※ この設計では、WorldMatrixの計算はRenderSystem実行直前にRenderSystem内で行うのが最適です。
        //    ここではBehaviourの更新に集中し、Transformの更新はBehaviourコンポーネントに任せます。

        // 2. Behaviour ComponentのOnStart/OnUpdate呼び出し
        for (auto& entry : behaviours_)
        {
            // Behaviourが付いているエンティティがまだ生存しているかを確認（※簡易実装。DestroyEntityでリストから削除されるため通常は不要）
            if (!alive_.count(entry.e.id)) continue;

            // 2-1. 初回実行時: OnStartを呼び出し
            if (!entry.started)
            {
                entry.b->OnStart(*this, entry.e);
                entry.started = true;
            }

            // 2-2. 毎フレーム実行: OnUpdateを呼び出し
            entry.b->OnUpdate(*this, entry.e, dt);
        }
    }

    // EntityBuilderからのみアクセスされるプライベート関数やメンバ
private:
    // Worldクラスのprivateセクションに追加
    // Behaviour管理用エントリ
    struct BEntry {
        Entity e;           ///< エンティティ
        Behaviour* b;       ///< Behaviourへのポインタ
        bool started = false; ///< OnStartが呼ばれたか
    };

    // Behaviourリスト
    std::vector<BEntry> behaviours_;

    /**
     * @brief BehaviourコンポーネントをWorldの更新リストに登録します。
     * @param[in] e 対象エンティティ
     * @param[in] b 登録するBehaviourコンポーネントへのポインタ
     */
    void registerBehaviour(Entity e, Behaviour* b)
    {
        behaviours_.push_back({ e, b, false });
    }

    /**
     * @brief Behaviourの登録を解除します。（Entity破棄時に使用）
     * @param[in] e 登録解除するエンティティ
     */
    void unregisterBehaviour(Entity e)
    {
        // EntityIDが一致するものをリストから削除（安定性のためerase-removeイディオムの簡易版）
        for (auto it = behaviours_.begin(); it != behaviours_.end(); ++it) {
            if (it->e.id == e.id) {
                // Behaviourはポインタなので、ここでメモリ解放は行いません（Storeが担当）
                behaviours_.erase(it);
                break;
            }
        }
    }

    uint32_t nextId_ = 0;                              ///< 次のエンティティID
    std::unordered_map<uint32_t, bool> alive_;         ///< エンティティの生存状態
    std::unordered_map<std::type_index, IStore*> stores_; ///< コンポーネントストア
    std::vector<std::function<void(Entity)>> erasers_;  ///< 削除用関数リスト

    // Behaviourリストは次のステップで実装

    friend class EntityBuilder;
};

#endif // !___WORLD_H___