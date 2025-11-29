/*****************************************************************//**
 * @file	CameraControlSystem.h
 * @brief	CameraComponentとRenderSystemのDrawSetupを連携し、カメラの追従と設定を行うSystem。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Oda Kaito
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	初回作成日
 * 			作業内容：	- 追加：ECS::Systemを継承した `CameraControlSystem` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___CAMERA_CONTROL_SYSTEM_H___
#define ___CAMERA_CONTROL_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
// Scene
#include "Scene/GameScene.h" 
#include "Main.h" // 画面サイズ定数にアクセス
#include "ECS/Components/Core/GameStateComponent.h"
#include <DirectXMath.h>

/**
 * @class CameraControlSystem
 * @brief カメラの追従ロジックとビュー・プロジェクション行列の計算を担当
 * 
 * 処理対象: CameraComponent を持つ Entity
 */
class CameraControlSystem : public ECS::System
{
	friend class PlayerControlSystem;

private:
	ECS::Coordinator* m_coordinator = nullptr;

	// 現在のカメラの位置と注視点（前フレームの結果を保持し、補間に使用）
	DirectX::XMFLOAT3 m_currentCameraPos;
	DirectX::XMFLOAT3 m_currentLookAt;

	// 【追加】カメラの回転角度を保持する変数
	float m_currentYaw = 0.0f;     // Y軸回転 (水平方向)
	float m_currentPitch = 0.0f;   // X軸回転 (垂直方向)

	GameMode m_lastGameMode = GameMode::ACTION_MODE;

public:
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
		// 初期値設定 (デモ描画時の初期位置に合わせる)
		m_currentCameraPos = DirectX::XMFLOAT3(0.0f, 3.5f, 5.0f);
		m_currentLookAt = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

		// カメラ回転の初期化
		m_currentYaw = 0.0f;
		m_currentPitch = DirectX::XM_PIDIV4 * 0.5f; // やや見下ろし気味に初期化

		// モード初期化
		m_lastGameMode = GameMode::ACTION_MODE;
	}

	/// @brief カメラの位置を計算し、RenderSystemのカメラ設定関数を呼び出す
	void Update(float deltaTime) override;
};

#endif // !___CAMERA_CONTROL_SYSTEM_H___