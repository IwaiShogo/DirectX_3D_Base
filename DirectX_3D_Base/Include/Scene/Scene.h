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

// ===== インクルード =====
#include <string>
#include <memory>
#include "ECS/World.h"
#include "ECS/System/RenderSystem.h" // RenderSystemを各シーンで保持・実行するため

// ===== 前方宣言 =====
class SceneManager;

/**
 * @interface   Scene
 * @brief       各シーンの抽象基底クラス
 */
class Scene
{
public:
    virtual ~Scene() = default;

    /**
     * @brief   シーン開始時の初期化処理
     * @details Worldの初期化、Entityの生成、RenderSystemなどのセットアップを行う
     */
    virtual void Initialize(SceneManager& manager) = 0;

    /**
     * @brief 毎フレームの更新処理
     * @param[in] dt デルタタイム
     */
    virtual void Update(float dt) = 0;

    /**
     * @brief 毎フレームの描画処理
     */
    virtual void Draw() = 0;

    /**
     * @brief シーン終了時の後処理
     */
    virtual void Finalize() = 0;

protected:
    // 各シーンが独立したWorldとRenderSystemのインスタンスを所有
    std::unique_ptr<World> world_;
    std::unique_ptr<RenderSystem> renderer_;
};

/**
 * @class SceneManager
 * @brief シーン遷移と現在のシーンの更新・描画を管理するクラス
 */
class SceneManager
{
public:
    void RegisterScene(const std::string& name, std::unique_ptr<Scene> scene);
    void ChangeScene(const std::string& name);
    void Update(float deltaTime);
    void Draw();

private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> mScenes;
    Scene* mCurrentScene = nullptr;
    std::string mNextSceneName;
};

#endif // !___SCENE_H___