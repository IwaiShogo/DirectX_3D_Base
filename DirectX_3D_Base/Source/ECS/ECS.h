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

// ===== インクルード =====
#include <cstdint>        // std::uint32_t, std::uint8_t
#include <bitset>         // std::bitset, Signature
#include <stdexcept>      // std::runtime_error
#include <vector>         // std::vector
#include <memory>         // std::shared_ptr, std::unique_ptr
#include <queue>          // std::queue
#include <unordered_map>  // std::unordered_map
#include <DirectXMath.h>  // TransformComponent のために必要

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
 * @brief 許容するEntityの最大数。
 * * 5000個のEntityを用意します。
 */
constexpr EntityID MAX_ENTITIES = 5000;

/**
 * @brief EntityがどのComponentを持っているかを示すビットセット。
 */
using Signature = std::bitset<MAX_COMPONENTS>;

// --------------------------------------------------
// 2. Component Type ID ジェネレーター (宣言)
// --------------------------------------------------
class ComponentTypeCounter
{
private:
    // IDジェネレータはインライン静的変数としてヘッダー内に定義
    inline static ComponentTypeID nextID = 0;
public:
    template<typename T>
    inline static ComponentTypeID GetID() noexcept
    {
        // ... (実装は変更なし)
        static ComponentTypeID typeID = nextID++;
        if (typeID >= MAX_COMPONENTS) {
            // エラー処理を省略
        }
        return typeID;
    }
};

// --------------------------------------------------
// 3. Component 構造体 (データのみ)
// --------------------------------------------------
struct TransformComponent
{
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4X4 WorldMatrix;

    TransformComponent()
    {
        DirectX::XMStoreFloat4x4(&WorldMatrix, DirectX::XMMatrixIdentity());
    }
};

#endif // !___ECS_H___