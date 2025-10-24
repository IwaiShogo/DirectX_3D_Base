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
#include "ECS/Coordinator.h" // ECSコア管理クラス

// シーン内で利用する全てのSystemを前方宣言 (Coordinatorに登録するため)
class MovementSystem;
class RenderSystem;
class InputSystem;
class CollisionSystem;

/**
 * @class GameScene
 * @brief 実際のゲームロジックとECSのライフサイクルを管理するシーン
 */
class GameScene
    : public Scene
{
private:
    // --------------------------------------------------
    // ECS管理コア
    // --------------------------------------------------
    Coordinator coordinator_;

    // --------------------------------------------------
    // Systemのインスタンス (Coordinatorから取得し、毎フレーム実行のために保持)
    // --------------------------------------------------
    std::shared_ptr<MovementSystem> movementSystem_;
    std::shared_ptr<RenderSystem> renderSystem_;
    std::shared_ptr<InputSystem> inputSystem_;
    std::shared_ptr<CollisionSystem> collisionSystem_;

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