#ifndef ___COORDINATOR_H___
#define ___COORDINATOR_H___



#include "Types.h"
#include "ComponentManager.h" // 後で実装
#include "EntityManager.h"    // 後で実装
#include "SystemManager.h"    // 後で実装

/**
 * @class Coordinator
 * @brief Entity-Component-Systemの全てを統括するコアマネージャー（ファサード）
 * * ECSの利用者は、このクラスを通じてのみEntityの生成、Componentの追加、
 * Systemの登録などを行うことで、実装詳細から分離されます。
 */
class Coordinator
{
private:
    std::unique_ptr<ComponentManager> componentManager_;
    std::unique_ptr<EntityManager> entityManager_;
    std::unique_ptr<SystemManager> systemManager_;

public:
    void Init()
    {
        // Managerの初期化
        entityManager_ = std::make_unique<EntityManager>();
        componentManager_ = std::make_unique<ComponentManager>();
        systemManager_ = std::make_unique<SystemManager>();
    }

    // -----------------------------------------
    // Entity関数
    // -----------------------------------------
    Entity CreateEntity()
    {
        return entityManager_->CreateEntity();
    }

    void DestroyEntity(Entity entity)
    {
        entityManager_->DestroyEntity(entity);
        componentManager_->EntityDestroyed(entity);
        systemManager_->EntityDestroyed(entity);
    }

    // -----------------------------------------
    // Component関数 (可読性を高めるためにテンプレートを使用)
    // -----------------------------------------

    /// @brief Componentを登録し、ComponentType IDを割り当てる
    template<typename T>
    void RegisterComponent()
    {
        componentManager_->RegisterComponent<T>();
    }

    /// @brief EntityにComponentを追加し、EntityのSignatureを更新する
    template<typename T>
    void AddComponent(Entity entity, T component)
    {
        componentManager_->AddComponent<T>(entity, component);

        auto signature = entityManager_->GetSignature(entity);
        signature.set(componentManager_->GetComponentType<T>(), true);
        entityManager_->SetSignature(entity, signature);

        systemManager_->EntitySignatureChanged(entity, signature);
    }

    /// @brief EntityからComponentを削除し、EntityのSignatureを更新する
    template<typename T>
    void RemoveComponent(Entity entity)
    {
        componentManager_->RemoveComponent<T>(entity);

        auto signature = entityManager_->GetSignature(entity);
        signature.set(componentManager_->GetComponentType<T>(), false);
        entityManager_->SetSignature(entity, signature);

        systemManager_->EntitySignatureChanged(entity, signature);
    }

    /// @brief EntityからComponentを取得する
    template<typename T>
    T& GetComponent(Entity entity)
    {
        return componentManager_->GetComponent<T>(entity);
    }

    /// @brief Componentの型IDを取得する
    template<typename T>
    ComponentType GetComponentType()
    {
        return componentManager_->GetComponentType<T>();
    }

    // -----------------------------------------
    // System関数
    // -----------------------------------------

    /// @brief Systemを登録し、そのSystemオブジェクトを取得する
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        return systemManager_->RegisterSystem<T>();
    }

    /// @brief Systemが処理対象とするComponentのSignatureを設定する
    template<typename T>
    void SetSystemSignature(Signature signature)
    {
        systemManager_->SetSignature<T>(signature);
    }
};

// --------------------------------------------------
// グローバル変数: Coordinatorインスタンス (シングルコアCoordinatorを想定)
// --------------------------------------------------

// グローバルなCoordinatorインスタンスを作成し、どこからでもアクセスできるようにする
// Game/Sceneクラス内でCoordinatorを初期化・管理し、各System/クラスに参照を渡す方がより望ましいが、
// 既存コードとの整合性を優先し、ここではシングルトンアクセス可能なヘルパーを定義する。

// ※ 実際のプロジェクトでは依存性注入(DI)またはシーン管理クラスによる管理を推奨。
//    ECS初心者向けのため、ここではシンプルな静的アクセスとしています。

// Coordinator* g_Coordinator = nullptr; // メインループ内でnewして使用する

#endif // !___COORDINATOR_H___