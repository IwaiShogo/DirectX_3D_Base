/*****************************************************************//**
 * @file	Component.h
 * @brief	ECSコンポーネントシステムの基底クラスとマクロ定義
 * 
 * @details	Entity Component System (ECS) アーキテクチャにおける
 *			コンポーネントの基本定義を提供。
 *			このECSは、**データコンポーネント**と**Behaviourコンポーネント**
 *			の２種類をサポートします。
 *			このファイルはそれらの基底クラスと、定義を簡潔にするための
 *			マクロを提供します。
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

#ifndef ___COMPONENT_H___
#define ___COMPONENT_H___

// ===== インクルード =====
#include "Entity.h"		// Entity
#include <typeindex>	// ComponentTypeIDの代わりにstd::type_indexを使用することを想定

// ===== 前方宣言 =====
class World;

/**
 * @interfaace	IComponent
 * @brief	全てのコンポーネントの基底インターフェース（データコンポーネントの親）
 * 
 * @details	このクラスを継承することで、Worldがコンポーネントの方情報 (std::type_index)	を
 *			識別し、効率的に管理できるようになります。
 * 
 * @note	データのみを持つコンポーネントはこのIComponentを直接継承するか、
 *			後述の'DEFINE_DATA_COMPONENT'マクロを使用してください。
 */
struct IComponent
{
	virtual ~IComponent() = default;
};

// --------------------------------------------------
// Behaviour Component (ロジックを持つコンポーネント)
// --------------------------------------------------

/**
 * @interface   Behaviour
 * @brief   毎フレーム更新ロジックを持つコンポーネントの基底クラス
 * 
 * @details 従来のSystemのロジックをコンポーネント自身に持たせることで、
 *          エンティティの振る舞いをより直感的に、オブジェクト指向に近い形で記述できます。
 * 
 * @note    Behaviourコンポーネントは、World::Update()内で自動的に更新されます。
 */
struct Behaviour : IComponent
{
    /**
     * @brief コンポーネントがWorldに追加され、有効になったときに一度だけ呼ばれる初期化処理
     * @param[in,out] w ワールドへの参照
     * @param[in] self このコンポーネントが付いているエンティティ
     */
    virtual void OnStart(World& w, Entity self) {}

    /**
     * @brief 毎フレーム呼ばれる更新処理
     * @param[in,out] w ワールドへの参照
     * @param[in] self このコンポーネントが付いているエンティティ
     * @param[in] dt デルタタイム（前フレームからの経過秒数）
     */
    virtual void OnUpdate(World& w, Entity self, float dt) = 0;
};

// --------------------------------------------------
// 便利なマクロ定義 (使いやすさ向上のための機能)
// --------------------------------------------------

/**
 * @def     DEFINE_DATA_COMPONENT
 * @brief   データコンポーネントを簡単に定義するマクロ
 * 
 * @param   ComponentName コンポーネントの名前
 * @param   ... メンバ変数の定義（末尾のセミコロンは不要）
 * 
 * @par     使用例:
 * @code
 *  // 体力コンポーネント
 *  DEFINE_DATA_COMPONENT(Health,
 *  float hp = 100.0f;
 *  float maxHp = 100.0f;
 *  )
 *  // タグコンポーネント（データなし）
 *  DEFINE_DATA_COMPONENT(Player, )
 * @endcode
 * 
 * @note    メンバ変数の定義の末尾にセミコロンは**不要**です（マクロ内で自動的に追加されます）。
 */
#define DEFINE_DATA_COMPONENT(ComponentName, ...) \
    struct ComponentName : IComponent { \
        __VA_ARGS__ \
    }

/**
 * @def     DEFINE_BEHAVIOUR
 * @brief   Behaviourコンポーネントを簡単に定義するマクロ
 * 
 * @param   BehaviourName コンポーネントの名前
 * @param   DataMembers メンバ変数の定義（セミコロン必須）
 * @param   UpdateCode OnUpdate()内で実行するコード（セミコロン必須）
 * 
 * @details OnUpdate()の実装を直接書けるため、動的なロジックを一つのファイル内で簡潔に定義できます。
 * 
 * @par     使用例:
 * @code
 *  // 上下に揺れるBehaviour
 *  DEFINE_BEHAVIOUR(Blinker,
 *      float timer = 0.0f;
 *  ,
 *      timer += dt;
 *      if (timer > 1.0f) {
 *      // 1秒ごとに点滅ロジック
 *      timer = 0.0f;
 *      }
 *  )
 * @endcode
 * 
 * @warning DataMembersとUpdateCodeの間のカンマを忘れるとコンパイルエラーになります。
 */
#define DEFINE_BEHAVIOUR(BehaviourName, DataMembers, UpdateCode) \
    struct BehaviourName : Behaviour { \
        DataMembers \
        void OnUpdate(World& w, Entity self, float dt) override { \
            UpdateCode \
        } \
    }

// --------------------------------------------------
// 型IDヘルパー
// --------------------------------------------------

/**
 * @brief   コンポーネント型Tに対応する std::type_index を取得します。
 * 
 * @tparam  T IComponentを継承したコンポーネント型
 * @return  std::type_index コンポーネントの型を一意に示すインデックス
 * 
 * @note    このインデックスは、Worldクラス内部でコンポーネントストアを識別するためのキーとして使用されます。
 */
template<typename T>
inline std::type_index GetComponentTypeID() noexcept
{
    static_assert(std::is_base_of<IComponent, T>::value, "T must inherit from IComponent.");
    return std::type_index(typeid(T));
}

#endif // !___COMPONENT_H___