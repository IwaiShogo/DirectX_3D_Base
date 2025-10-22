#ifndef ___SYSTEMMANAGER_H___
#define ___SYSTEMMANAGER_H___

// ===== インクルード =====
#include "Types.h"
#include <map>
#include <stdexcept>

/**
 * @class SystemManager
 * @brief Systemのインスタンス、Signature、および処理対象Entityの集合を管理する
 */
class SystemManager
{
private:
    // Systemの型名とSystemインスタンスのマップ
    std::map<const char*, Signature> signatures_;

    // Systemの型名とSystemインスタンスのマップ
    std::map<const char*, std::shared_ptr<System>> systems_;

public:
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        const char* typeName = typeid(T).name();

        if (systems_.count(typeName))
        {
            throw std::runtime_error("Error: System type already registered.");
        }

        // Systemインスタンスを作成し、マップに格納
        auto system = std::make_shared<T>();
        systems_.insert({ typeName, system });
        return system;
    }

    template<typename T>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(T).name();

        if (!systems_.count(typeName))
        {
            throw std::runtime_error("Error: System not registered. Call RegisterSystem<T>() first.");
        }

        // Systemが処理すべきComponentのSignatureを登録
        signatures_.insert({ typeName, signature });
    }

    /// @brief EntityのSignatureが変更されたときに呼び出される
    void EntitySignatureChanged(Entity entity, Signature entitySignature)
    {
        // 全てのSystemに対してループし、Entityを処理対象に追加/削除するか判断する
        for (auto const& pair : systems_)
        {
            const char* typeName = pair.first;
            auto& system = pair.second;
            const Signature& systemSignature = signatures_[typeName];

            // EntityのSignatureがSystemのSignatureを包含するか？
            // (entitySignature & systemSignature) == systemSignature
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // 一致する -> Systemの処理対象Entityリストに追加
                system->entities->insert(entity);
            }
            else
            {
                // 一致しない -> Systemの処理対象Entityリストから削除
                system->entities->erase(entity);
            }
        }
    }

    /// @brief Entityが破壊されたときに呼び出される
    void EntityDestroyed(Entity entity)
    {
        // 全てのSystemの処理対象Entityリストから削除する
        for (auto const& pair : systems_)
        {
            pair.second->entities->erase(entity);
        }
    }
};

#endif // !___SYSTEMMANAGER_H___