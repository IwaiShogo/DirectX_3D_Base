/*****************************************************************//**
 * @file	EffectSystem.h
 * @brief	エフェクト
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/07	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___EFFECT_SYSTEM_H___
#define ___EFFECT_SYSTEM_H___

#include "ECS/ECS.h"
#include <Effekseer/Effekseer.h>
#include <Effekseer/EffekseerRendererDX11.h>

class EffectSystem : public ECS::System
{
public:
    void Uninit(); // 終了処理

    void Init(ECS::Coordinator* coordinator) override;   // Effekseer初期化 & AssetManagerへのセット
    void Update(float deltaTime) override; // 位置同期と再生管理
    void Render(); // 描画実行

private:
    ECS::Coordinator* m_coordinator = nullptr;
    EffekseerRenderer::RendererRef m_renderer;
    Effekseer::ManagerRef m_manager;
};

#endif // !___EFFECT_SYSTEM_H___