/*****************************************************************//**
 * @file	CameraControlSystem.cpp
 * @brief	CameraControlSystemの実装。カメラの追従とビュー行列の更新を行う。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	初回作成日
 * 			作業内容：	- 追加：プレイヤーのTransformComponentに基づき、カメラ位置を補間し、ビュー・プロジェクション行列をGeometoryシステムに設定するロジックを実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "ECS/Systems/CameraControlSystem.h"
#include "Systems/Geometory.h" // カメラ設定関数を使用

using namespace DirectX;

/**
 * @brief 2つの位置を線形補間する (Lerp)
 * @param start - 開始位置
 * @param end - 終了位置
 * @param t - 補間係数 (0.0f～1.0f)
 * @return XMFLOAT3 - 補間された位置
 */
static XMFLOAT3 Lerp(const XMFLOAT3& start, const XMFLOAT3& end, float t)
{
	t = std::min(1.0f, std::max(0.0f, t)); // tを0～1にクランプ
	XMFLOAT3 result;
	result.x = start.x + (end.x - start.x) * t;
	result.y = start.y + (end.y - start.y) * t;
	result.z = start.z + (end.z - start.z) * t;
	return result;
}

/**
 * @brief カメラの位置を計算し、ビュー・プロジェクション行列を設定する
 */
void CameraControlSystem::Update()
{
	// CameraComponentを持つEntityは通常1つと想定
	for (auto const& entity : m_entities)
	{
		CameraComponent& cameraComp = m_coordinator->GetComponent<CameraComponent>(entity);

		ECS::EntityID focusID = cameraComp.focusEntityID;

		// 追従対象のEntityが存在し、Transformを持っているかチェック
		if (focusID != ECS::INVALID_ENTITY_ID &&
			m_coordinator->m_entityManager->GetSignature(focusID).test(m_coordinator->GetComponentTypeID<TransformComponent>()))
		{
			// 追従対象のTransformComponentを取得
			TransformComponent& focusTrans = m_coordinator->GetComponent<TransformComponent>(focusID);

			// 1. 目標カメラ位置 (Target Camera Position) の計算
			// ターゲットの位置 + オフセット
			XMFLOAT3 targetPos;
			targetPos.x = focusTrans.position.x + cameraComp.offset.x;
			targetPos.y = focusTrans.position.y + cameraComp.offset.y;
			targetPos.z = focusTrans.position.z + cameraComp.offset.z;

			// 2. カメラ位置の補間 (Smooth Following)
			// 現在位置から目標位置へ向かって緩やかに移動
			m_currentCameraPos = Lerp(m_currentCameraPos, targetPos, cameraComp.followSpeed);

			// 3. 注視点（LookAt）の補間
			// 追従対象の位置が新しい注視点
			XMFLOAT3 targetLookAt = focusTrans.position;
			m_currentLookAt = Lerp(m_currentLookAt, targetLookAt, cameraComp.followSpeed);
		}

		// 4. ビュー行列の計算と設定 (RenderSystemの役割を肩代わり)

		XMVECTOR camPos = XMLoadFloat3(&m_currentCameraPos);
		XMVECTOR lookAt = XMLoadFloat3(&m_currentLookAt);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		// View行列
		XMMATRIX viewMatrix = XMMatrixLookAtLH(camPos, lookAt, up);

		// Projection行列
		XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
			cameraComp.FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, cameraComp.nearClip, cameraComp.farClip
		);

		// DirectXへ渡すために転置して格納
		XMFLOAT4X4 fMatView;
		XMStoreFloat4x4(&fMatView, XMMatrixTranspose(viewMatrix));

		XMFLOAT4X4 fMatProjection;
		XMStoreFloat4x4(&fMatProjection, XMMatrixTranspose(projectionMatrix));

		// ★ 追加: 計算結果をCameraComponentに格納
		cameraComp.viewMatrix = fMatView;
		cameraComp.projectionMatrix = fMatProjection;
		cameraComp.worldPosition = m_currentCameraPos; // カメラ位置も格納
	}
}