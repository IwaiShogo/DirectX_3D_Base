/*****************************************************************//**
 * @file	Types.h
 * @brief	ECSで利用する基本となる方の定義
 * 
 * @details	
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

#ifndef ___TYPES_H___
#define ___TYPES_H___

// ===== インクルード =====
#include <cstdint>
#include <bitset>
#include <memory>
#include <typeinfo>
#include <set>

// --------------------------------------------------
// ECSの基本型定義 (初心者でも理解しやすいようにシンプルなエイリアスを使用)
// --------------------------------------------------

/// @brief EntityのID。0から順に割り振る。
using Entity = std::uint32_t;
/// @brief Entityの最大数。ゲームの規模に応じて調整する。
constexpr Entity MAX_ENTITIES = 5000;

/// @brief ComponentのID。0から順に割り振る。
using ComponentType = std::uint8_t;
/// @brief Componentの最大数。最大64種類まで定義可能（ビットセットサイズに依存）。
constexpr ComponentType MAX_COMPONENTS = 32; // 64も可能だが、まずはコンパクトに32で
// Componentの最大数を超えた場合、ビットセットのサイズを変更する必要がある。

/// @brief Componentの集合（署名/マスク）。どのComponentをEntityが持つか、どのSystemが処理するかを示す。
using Signature = std::bitset<MAX_COMPONENTS>;

// --------------------------------------------------
// SystemとComponentの基底クラス (純粋なインターフェース)
// --------------------------------------------------

/// @brief 全てのComponentが継承するマーカーインターフェース（データコンテナの目印）
struct IComponent
{
    virtual ~IComponent() = default;
};

/// @brief 全てのSystemが継承する基底インターフェース
class System
{
public:
    /// @brief このSystemが処理すべきEntityの集合
    Signature componentSignature;

    /// @brief このSystemが処理するEntityのID集合
    std::shared_ptr<std::set<Entity>> entities;

    System() : entities(std::make_shared<std::set<Entity>>()) {}
    virtual ~System() = default;

    /// @brief 初期化処理 (Coordinatorで初期化時に一度だけ呼び出される)
    virtual void Initialize() {}

    /// @brief 更新処理 (メインループで毎フレーム呼び出される)
    virtual void Update(float deltaTime) = 0;
};

#endif // !___TYPES_H___