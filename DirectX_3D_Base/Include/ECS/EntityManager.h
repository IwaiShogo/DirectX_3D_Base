/*****************************************************************//**
 * @file	EntityManager.h
 * @brief	Entityの生成とSignature管理
 * 
 * @details	Entityを単なるIDとして扱い、
 *          どのComponentを持っているかを示すSignatureを管理します。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ENTITYMANAGER_H___
#define ___ENTITYMANAGER_H___

// ===== インクルード =====
#include "Types.h"
#include <queue>
#include <array>
#include <stdexcept>

/**
 * @class EntityManager
 * @brief Entity IDの発行と、そのEntityが持つComponentのSignatureを管理する
 */
class EntityManager
{
private:
    // 再利用可能なEntity IDを保持するキュー
    std::queue<Entity> availableEntities_;

    // 各Entity IDに対応するComponent Signatureを保持する配列
    // Signatureは、どのComponent Typeを持っているかを示すビットマスク
    std::array<Signature, MAX_ENTITIES> signatures_;

    // 現在までに生成されたEntityの総数
    std::uint32_t livingEntityCount_ = 0;

public:
    EntityManager()
    {
        // 初期化：MAX_ENTITIESまでのIDをキューに投入する
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            availableEntities_.push(entity);
        }
    }

    /// @brief 新しいEntityを生成し、一意のIDを割り当てる
    /// @return Entity ID
    Entity CreateEntity()
    {
        if (livingEntityCount_ >= MAX_ENTITIES)
        {
            throw std::runtime_error("Error: Maximum number of entities reached.");
        }

        // キューからEntity IDを取得
        Entity id = availableEntities_.front();
        availableEntities_.pop();
        ++livingEntityCount_;

        return id;
    }

    /// @brief Entityを破棄し、IDを再利用可能にする
    /// @param[in] entity 破棄するEntity ID
    void DestroyEntity(Entity entity)
    {
        // Entity IDのシグネチャをリセット
        signatures_[entity].reset();

        // Entity IDをキューに戻す
        availableEntities_.push(entity);
        --livingEntityCount_;
    }

    /// @brief EntityのComponent Signatureを取得
    /// @param[in] entity Entity ID
    /// @return Component Signature (bitset)
    Signature GetSignature(Entity entity) const
    {
        return signatures_[entity];
    }

    /// @brief EntityのComponent Signatureを設定
    /// @param[in] entity Entity ID
    /// @param[in] signature 設定するSignature
    void SetSignature(Entity entity, Signature signature)
    {
        signatures_[entity] = signature;
    }
};

#endif // !___ENTITYMANAGER_H___