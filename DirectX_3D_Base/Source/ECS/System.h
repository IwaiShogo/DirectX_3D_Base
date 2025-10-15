/*****************************************************************//**
 * @file	System.h
 * @brief	
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

#ifndef ___SYSTEM_H___
#define ___SYSTEM_H___

// ===== インクルード =====
#include "ECS.h" // コア定義を参照
#include <typeinfo> // typeid().name() のために必要

 // --------------------------------------------------
 // 1. Systemの抽象基底クラス
 // --------------------------------------------------
class System
{
public:
    // このSystemが処理対象とするEntityのComponent構成
    Signature componentSignature;

    // このSystemが処理するEntityのIDリスト
    std::vector<EntityID> entities;

    // 毎フレーム実行されるSystemのロジック (純粋仮想関数)
    virtual void Update(float deltaTime) = 0;

    virtual ~System() = default;
};


// --------------------------------------------------
// 2. System Manager の宣言
// --------------------------------------------------
class SystemManager
{
public:
    // テンプレート関数: Systemの登録とインスタンス取得
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        const char* typeName = typeid(T).name();

        if (mSystems.count(typeName)) {
            return std::static_pointer_cast<T>(mSystems[typeName]);
        }

        std::shared_ptr<T> system = std::make_shared<T>();
        mSystems.insert({ typeName, system });
        return system;
    }

    // テンプレート関数: Systemの署名設定
    template<typename T>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(T).name();

        if (mSystems.find(typeName) == mSystems.end()) {
            throw std::runtime_error("SystemManager::SetSignature(): System not registered before setting Signature.");
        }

        mSignatures[typeName] = signature;
        mSystems[typeName]->componentSignature = signature;
    }

    // 非テンプレート関数 (System.cppで実装)
    /**
     * @brief EntityのSignatureが変更されたとき、SystemのEntityリストを更新します。
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

            // EntityのSignatureがSystemのSignatureを包含するかどうか
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // EntityがSystemの要求を満たしている場合 (リストに追加/維持)

                auto& entities = system->entities;
                bool alreadyIn = false;
                for (EntityID id : entities) {
                    if (id == entityID) {
                        alreadyIn = true;
                        break;
                    }
                }
                if (!alreadyIn) {
                    entities.push_back(entityID);
                }
            }
            else
            {
                // EntityがSystemの要求を満たさなくなった場合 (リストから削除)

                auto& entities = system->entities;
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
    }
    /**
     * @brief Entityが破棄されたとき、全てのSystemのEntityリストから削除します。
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

    // CoordinatorがSystemをループするために使用するアクセサ
    const std::unordered_map<const char*, std::shared_ptr<System>>& GetSystems() const
    {
        return mSystems;
    }

private:
    std::unordered_map<const char*, std::shared_ptr<System>> mSystems;
    std::unordered_map<const char*, Signature> mSignatures;
};

#endif // !___SYSTEM_H___