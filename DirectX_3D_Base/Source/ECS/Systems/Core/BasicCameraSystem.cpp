/*****************************************************************//**
 * @file	BasicCameraSystem.cpp
 * @brief	
 * 
 * @details	
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

 // ===== インクルード =====
#include "ECS/Systems/Core/BasicCameraSystem.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Core/BasicCameraComponent.h" // 新しいコンポーネント
#include "Systems/Geometory.h" // 行列設定用
#include "Main.h" // SCREEN_WIDTH, SCREEN_HEIGHT

using namespace DirectX;

void BasicCameraSystem::Update(float deltaTime)
{
	// 登録された全エンティティ（カメラ）を処理
	for (auto const& entity : m_entities)
	{
		auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		auto& basicCamera = m_coordinator->GetComponent<BasicCameraComponent>(entity);

		// --- 1. ビュー行列の計算 ---
		// 回転行列の作成 (Pitch, Yaw, Roll)
		XMMATRIX matRotate = XMMatrixRotationRollPitchYaw(
			transform.rotation.x,
			transform.rotation.y,
			transform.rotation.z
		);

		// カメラ位置
		XMVECTOR eyePos = XMLoadFloat3(&transform.position);

		// 向いている方向 (0,0,1 を回転)
		XMVECTOR lookDir = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), matRotate);

		// 上方向 (0,1,0 を回転)
		XMVECTOR upDir = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), matRotate);

		// 注視点
		XMVECTOR focusPos = XMVectorAdd(eyePos, lookDir);

		// ビュー行列 (左手系)
		XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePos, focusPos, upDir);

		// --- 2. プロジェクション行列の計算 ---
		float aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

		XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(
			basicCamera.FOV,
			aspectRatio,
			basicCamera.nearClip,
			basicCamera.farClip
		);

		// --- 3. 結果の格納と反映 ---
		// コンポーネントへの書き戻し
		XMStoreFloat4x4(&basicCamera.viewMatrix, XMMatrixTranspose(viewMatrix));
		XMStoreFloat4x4(&basicCamera.projectionMatrix, XMMatrixTranspose(projMatrix));
		basicCamera.worldPosition = transform.position;

		// 描画システム(Geometory)へ反映
		// 複数のカメラがある場合は、最後にUpdateされたカメラが有効になります
		Geometory::SetView(basicCamera.viewMatrix);
		Geometory::SetProjection(basicCamera.projectionMatrix);
	}
}