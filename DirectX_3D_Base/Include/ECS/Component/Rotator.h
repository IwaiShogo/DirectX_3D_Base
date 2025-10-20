/*****************************************************************//**
 * @file	RotatorComponent.h
 * @brief	自動回転Behaviourコンポーネントの定義
 * 
 * @details	このファイルは、エンティティを自動的に回転させる**Behaviourコンポーネント**を定義します。
 *          Behaviourコンポーネントの基本的な実装例として、学習に最適です。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/20	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ROTATOR_H___
#define ___ROTATOR_H___

// ===== インクルード =====
#include "ECS/Component.h"
#include "ECS/Entity.h"
#include "ECS/World.h"
#include "ECS/Component/Transform.h"
#include <DirectXMath.h>

 /**
  * @struct Rotator
  * @brief エンティティを自動的にY軸中心で回転させるBehaviourコンポーネント
  * 
  * @details
  * このコンポーネントをエンティティに追加すると、毎フレーム自動的に
  * Y軸（上下軸）を中心に回転します。
  * * ### Behaviourの仕組み:
  * 1. World::Update()内でOnUpdate()メソッドが自動的に呼ばれる。
  * 2. OnUpdate()内で、自身のTransformコンポーネントを取得し、rotation.yに角度を加算。
  * 
  * @par 使用例（ビルダーパターン）:
  * @code
  * // 毎秒120度で回転する Entity を作成
  * Entity cube = world.Create()
  * .With<Transform>({0, 0, 10})
  * .With<Rotator>(120.0f)
  * .Build();
  * @endcode
  * 
  * @see Behaviour Behaviourコンポーネントの基底クラス
  */
DEFINE_BEHAVIOUR(Rotator,
    /**
     * @var speedDegY
     * @brief Y軸周りの回転速度 (度/秒)
     * @note 角度は**度数法**で指定します。
     */
    float speedDegY = 45.0f;

    /**
     * @brief 回転速度を指定するコンストラクタ
     * @param[in] s 回転速度（度/秒）
     */
    explicit Rotator(float s) : speedDegY(s) {}

    /**
     * @brief デフォルトコンストラクタ
     * @details 回転速度を45.0度/秒に設定します。
     */
    Rotator() = default;
    , // <-- データメンバと更新ロジックを区切るカンマ

        /**
         * @brief 毎フレーム呼ばれる更新処理
         * 
         * @param[in,out] w ワールドへの参照（コンポーネント取得に使用）
         * @param[in] self このコンポーネントが付いているエンティティ
         * @param[in] dt デルタタイム（前フレームからの経過秒数）
         * 
         * @details
         * この関数が毎フレーム自動的に呼ばれ、以下の処理を行います：
         * 1. 自身のTransformコンポーネントを取得
         * 2. rotation.yに speedDegY * dt を加算して回転
         * 3. 360度を超えたら正規化（0～360度の範囲に収める）
         * 
         * @note dt（デルタタイム）を掛けることで、フレームレートに依存しない**安定した動き**を実現しています。
         * @warning Transformコンポーネントがない場合、処理をスキップします。
         */
        Transform* t = w.TryGet<Transform>(self);
    if (!t) return;

    // Y軸回転をデルタタイムを考慮して加算
    t->rotation.y += speedDegY * dt;

    // 360度を超えたら正規化 (0～360度)
    if (t->rotation.y > 360.0f) {
        t->rotation.y -= 360.0f;
    }
) // DEFINE_BEHAVIOUR マクロの終わり

#endif // !___ROTATOR_H___