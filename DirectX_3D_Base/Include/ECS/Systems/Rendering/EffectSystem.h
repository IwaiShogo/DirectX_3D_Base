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
#include <DirectXMath.h>

class EffectSystem : public ECS::System
{
public:
	void Uninit();

	void Init(ECS::Coordinator* coordinator) override;
	void Update(float deltaTime) override;
	void Render();

	
	// ★追加：スクリーン座標（UI座標）用の正射影カメラを上書き
	// 左上(0,0) 〜 右下(screenW, screenH) / Y下向き
	void SetScreenSpaceCamera(float screenW, float screenH);
	void ClearOverrideCamera();

	void StopEffectImmediate(ECS::EntityID entityID);

private:
	ECS::Coordinator* m_coordinator = nullptr;
	EffekseerRenderer::RendererRef m_renderer;
	Effekseer::ManagerRef m_manager;

	// ★追加：カメラ上書き
	bool m_hasOverride = false;
	DirectX::XMFLOAT4X4 m_overrideView{};
	DirectX::XMFLOAT4X4 m_overrideProj{};
};

#endif // !___EFFECT_SYSTEM_H___
