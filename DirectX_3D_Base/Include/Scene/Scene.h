/*****************************************************************//**
 * @file	Scene.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/21	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SCENE_H___
#define ___SCENE_H___

/**
 * @class   Scene
 * @brief   シーン管理のための抽象基底クラス
 */
class Scene
{
public:
    virtual ~Scene() = default;

    /**
     * @brief シーンの初期化処理
     * @note ECSコンポーネントとシステム、およびシーン固有のリソースの読み込みを行う
     */
    virtual void Initialize() = 0;

    /**
     * @brief シーンの更新処理
     * @param[in] deltaTime 前フレームからの経過時間（秒）
     * @note ECSシステム（MovementSystemなど）のUpdateメソッドを呼び出す
     */
    virtual void Update(float deltaTime) = 0;

    /**
     * @brief シーンの描画処理
     * @note ECSシステム（RenderSystemなど）のDrawメソッドを呼び出す
     */
    virtual void Draw() = 0;

    /**
     * @brief シーンの終了処理
     */
    virtual void Finalize() = 0;
};

#endif // !___SCENE_H___