/*****************************************************************//**
 * @file	BasicCameraComponent.h
 * @brief	固定カメラや単純移動カメラ用の設定を定義するComponent。
 * 
 * @details	
 * 追従機能を持たず、TransformComponentの位置・回転と、
 * 本コンポーネントの画角設定のみでビュー・プロジェクション行列が決まる。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/03	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___BASIC_CAMERA_COMPONENT_H___
#define ___BASIC_CAMERA_COMPONENT_H___

 // ===== インクルード =====
#include <DirectXMath.h>
#include "ECS/Types.h"
#include "Main.h" // METERマクロ等が必要な場合

/**
 * @struct BasicCameraComponent
 * @brief 追従機能を持たないシンプルなカメラ設定
 */
struct BasicCameraComponent
{
	float FOV;			///< 視野角 (ラジアン)
	float nearClip;		///< 近クリップ平面
	float farClip;		///< 遠クリップ平面

	// 計算結果保持用（描画システム等から参照）
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;
	DirectX::XMFLOAT3	worldPosition;

	/**
	 * @brief コンストラクタ
	 * @param fovDegrees 視野角（度数法）
	 * @param nearClip 近クリップ距離
	 * @param farClip 遠クリップ距離
	 */
	BasicCameraComponent(
		float fovDegrees = 60.0f,
		float nearClip = 0.1f,
		float farClip = 1000.0f
	)
		: nearClip(nearClip)
		, farClip(farClip)
		, viewMatrix(DirectX::XMFLOAT4X4())
		, projectionMatrix(DirectX::XMFLOAT4X4())
		, worldPosition(DirectX::XMFLOAT3())
	{
		// 度数法 -> ラジアン変換
		FOV = DirectX::XMConvertToRadians(fovDegrees);

		// 行列の初期化（単位行列）
		DirectX::XMStoreFloat4x4(&viewMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&projectionMatrix, DirectX::XMMatrixIdentity());
	}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(BasicCameraComponent)

#endif // !___BASIC_CAMERA_COMPONENT_H___