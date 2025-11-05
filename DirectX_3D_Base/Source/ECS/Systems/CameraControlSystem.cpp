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

// ===== インクルード =====
#include "ECS/Systems/CameraControlSystem.h"
#include "Systems/Geometory.h"	// カメラ設定関数を使用
#include "Systems/Input.h"		// GetRightStick()
#include "ECS/Components/PlayerControlComponent.h"

using namespace DirectX;

// ===== 定数・マクロ定義 =====
// 右スティックの感度
const float CAMERA_SENSITIVITY_X = 0.04f; // YAW感度
const float CAMERA_SENSITIVITY_Y = 0.03f; // PITCH感度 (上下は控えめに)
// PITCHのクランプ値 (上下の限界)
const float PITCH_MAX = XM_PIDIV2 - 0.2f; // 約78度
const float PITCH_MIN = -XM_PIDIV2 * 0.3f; // 約-27度 (水平より少し下まで)

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
	XMFLOAT2 rightStick = GetRightStick(); // 右スティック入力を取得

	// CameraComponentを持つEntityは通常1つと想定
	for (auto const& entity : m_entities)
	{
		auto& camera = m_coordinator->GetComponent<CameraComponent>(entity);
		ECS::EntityID focusID = camera.focusEntityID;

		if (focusID != ECS::INVALID_ENTITY_ID &&
			m_coordinator->m_entityManager->GetSignature(focusID).test(m_coordinator->GetComponentTypeID<TransformComponent>()))
		{
			// 追従対象のTransformComponentを取得
			TransformComponent& focusTrans = m_coordinator->GetComponent<TransformComponent>(focusID);

			// =====================================
			// 1. カメラ回転角度の更新（右スティック）
			// =====================================
			// YAW (水平方向) を更新
			m_currentYaw += rightStick.x * CAMERA_SENSITIVITY_X;

			// PITCH (垂直方向) を更新
			m_currentPitch += rightStick.y * CAMERA_SENSITIVITY_Y;

			// PITCH角のクランプ（上下の回転制限）
			m_currentPitch = std::max(PITCH_MIN, std::min(PITCH_MAX, m_currentPitch));

			// =====================================
			// 2. 目標カメラ位置 (Target Camera Position) の計算
			// =====================================
			// 2-a. オフセットベクトルをロードし、長さだけを保持
			XMVECTOR defaultOffset = XMLoadFloat3(&camera.offset);
			float offsetLength = XMVectorGetX(XMVector3Length(defaultOffset));

			// 2-b. YAWとPITCHから回転行列を構築
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
				m_currentPitch, // X軸回転 (Pitch)
				m_currentYaw,   // Y軸回転 (Yaw)
				0.0f            // Z軸回転 (Roll)
			);

			// 2-c. デフォルトのオフセット位置 (Z軸に-offsetLengthだけ離れた位置)を回転
			// カメラはターゲットの-Z方向に距離offsetLengthを保つようにする
			XMVECTOR rotatedOffset = XMVector3Transform(
				XMVectorSet(0.0f, 0.0f, -offsetLength, 0.0f),
				rotationMatrix
			);

			// 2-d. 追従対象の位置をロード
			XMVECTOR focusPos = XMLoadFloat3(&focusTrans.position);

			// 2-e. 目標カメラ位置 = ターゲット位置 + 回転オフセット
			XMVECTOR targetCamPosV = focusPos + rotatedOffset;

			// 2-f. 目標注視点 = ターゲット位置
			XMVECTOR targetLookAtV = focusPos;

			// 3. カメラ位置と注視点の補間 (Smooth Following)

			// 現在のカメラ位置と注視点をロード
			XMVECTOR currentCamPosV = XMLoadFloat3(&m_currentCameraPos);
			XMVECTOR currentLookAtV = XMLoadFloat3(&m_currentLookAt);

			// Lerp（線形補間）: 既存のLerp関数をXMVECTOR版に置き換える
			XMVECTOR newCamPosV = XMVectorLerp(currentCamPosV, targetCamPosV, camera.followSpeed);
			XMVECTOR newLookAtV = XMVectorLerp(currentLookAtV, targetLookAtV, camera.followSpeed);

			// 結果をメンバ変数にストア
			XMStoreFloat3(&m_currentCameraPos, newCamPosV);
			XMStoreFloat3(&m_currentLookAt, newLookAtV);

			// 4. ビュー行列の計算と設定 (RenderSystemの役割を肩代わり)

			XMVECTOR camPos = XMLoadFloat3(&m_currentCameraPos);
			XMVECTOR lookAt = XMLoadFloat3(&m_currentLookAt);
			XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

			// View行列
			XMMATRIX viewMatrix = XMMatrixLookAtLH(camPos, lookAt, up);

			// Projection行列
			XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
				camera.FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, camera.nearClip, camera.farClip
			);

			// DirectXへ渡すために転置して格納
			XMFLOAT4X4 fMatView;
			XMStoreFloat4x4(&fMatView, XMMatrixTranspose(viewMatrix));

			XMFLOAT4X4 fMatProjection;
			XMStoreFloat4x4(&fMatProjection, XMMatrixTranspose(projectionMatrix));

			// ★ 追加: 計算結果をCameraComponentに格納
			camera.viewMatrix = fMatView;
			camera.projectionMatrix = fMatProjection;
			camera.worldPosition = m_currentCameraPos; // カメラ位置も格納
		}
	}
}