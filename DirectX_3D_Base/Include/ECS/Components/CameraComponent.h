/*****************************************************************//**
 * @file	CameraComponent.cpp
 * @brief	カメラの振る舞いとプロジェクション設定を定義するComponent。
 *
 * @details	追従対象のEntityID、追従オフセット、追従速度などのデータを保持する。
 *
 * ------------------------------------------------------------
 * @author	IwaiShogo
 * ------------------------------------------------------------
 *
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：カメラの追従対象、オフセット、プロジェクション設定を保持する'CameraComponent'を作成。
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___CAMERA_COMPONENT_H___
#define ___CAMERA_COMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include "ECS/Types.h" // EntityIDを使用するため
#include "Main.h" // METERマクロを使用

/**
 * @struct CameraComponent
 * @brief 追従式カメラの振る舞いと設定
 */
struct CameraComponent
{
	ECS::EntityID FocusEntityID;	///< 追従するEntityのID
	DirectX::XMFLOAT3 Offset;		///< 追従対象からの相対位置オフセット
	float FollowSpeed;				///< 追従の補間速度 (0.0f～1.0f: 値が大きいほど追従が速い)

	float FOV;						///< 視野角 (ラジアン)
	float NearClip;					///< 近クリップ平面
	float FarClip;					///< 遠クリップ平面

	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
	DirectX::XMFLOAT3	WorldPosition;

	/**
	 * @brief コンストラクタ
	 */
	CameraComponent(
		ECS::EntityID focusID = ECS::INVALID_ENTITY_ID,
		DirectX::XMFLOAT3 offset = DirectX::XMFLOAT3(0.0f, METER(3.0f), METER(-5.0f)), // プレイヤーの斜め後ろ上空を想定
		float followSpeed = 0.1f, // 緩やかに追従
		float fovDegrees = 60.0f,
		float nearClip = 0.1f,
		float farClip = 1000.0f
	) : FocusEntityID(focusID), Offset(offset), FollowSpeed(followSpeed), NearClip(nearClip), FarClip(farClip)
	{
		// 視野角を度からラジアンに変換
		FOV = DirectX::XMConvertToRadians(fovDegrees);
	}
};

#endif // !___CAMERA_COMPONENT_H___