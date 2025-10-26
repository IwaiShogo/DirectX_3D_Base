/*****************************************************************//**
 * @file	GameScene.h
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

#ifndef ___GAMESCENE_H___
#define ___GAMESCENE_H___

#include "Scene.h"           // 基底クラス Scene


// シーン内で利用する全てのSystemを前方宣言 (Coordinatorに登録するため)
class MovementSystem;
class RenderSystem;
class InputSystem;
class CollisionSystem;
class CameraSystem;

/**
 * @class GameScene
 * @brief 実際のゲームロジックとECSのライフサイクルを管理するシーン
 */
class GameScene
    : public Scene
{
private:


public:
    GameScene() = default;
    ~GameScene() override = default;

    // Sceneインターフェースの実装
    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Finalize() override;

private:
    /**
     * @brief ECSコア、Component、Systemの初期化と登録を行う
     */
    void SetupECS();

    /**
     * @brief ECS Entityの初期配置（テスト用Entityの生成など）を行う
     */
    void SetupEntities();
};

#endif // !___GAMESCENE_H___