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

// ===== インクルード =====
#include "Scene/Scene.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"
#include "ECS/Component/Rotator.h"
//#include "ECS/Component/InputState.h"
//#include "ECS/Component/PlayerMovement.h"
#include "Systems/DirectX/DirectX.h" // DirectXクラス（GfxDevice）
#include "Systems/DirectX/MeshBuffer.h" // MeshBuffer（MeshManager）
#include "Systems/DirectX/ShaderList.h" // ShaderList

class GameScene : public Scene
{
public:
    // 初期化時、SceneManagerから呼ばれる
    void Initialize(SceneManager& manager) override;

    // 毎フレーム呼ばれる更新
    void Update(float dt) override;

    // 毎フレーム呼ばれる描画
    void Draw() override;

    // 終了処理
    void Finalize() override;

private:
    // RenderSystem初期化に必要な依存オブジェクトを保持 (仮の依存性注入)
    DirectX* gfx_ = nullptr;
    Camera* cam_ = nullptr; // 既存コードのCameraクラスを想定
    MeshBuffer* meshMgr_ = nullptr;
    ShaderList* shaderList_ = nullptr;
};

#endif // !___GAMESCENE_H___