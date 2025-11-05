/*****************************************************************//**
 * @file	PlayerControlSystem.cpp
 * @brief	プレイヤーのキー入力に基づいてEntityの運動状態を走査するSystemの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：キー入力（左右移動、ジャンプ）に基づいてRigidBodyの速度を更新するロジックを実装。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/ECSInitializer.h"
#include "ECS/Systems/PlayerControlSystem.h"
#include "ECS/Systems/CameraControlSystem.h"
#include <iostream>

using namespace DirectX;

/**
 * @brief 入力に応じてRigidBodyの速度を更新する
 */
void PlayerControlSystem::Update()
{
	// CameraControlSystemはECSInitializerによって登録されている前提
	auto cameraSystem = ECS::ECSInitializer::GetSystem<CameraControlSystem>();
	if (!cameraSystem) return; // カメラシステムが存在しなければ処理を中断

	// 現在のカメラのYAW角を取得
	// ※ PlayerControlSystem.hでCameraControlSystemをインクルードし、
	// m_currentYaw が public/friend であるか、publicなgetterを持つことを前提とします。
	// プロトタイプのため、ここではアクセス可能と仮定します。
	float cameraYaw = cameraSystem->m_currentYaw;

	// Xboxコントローラーの左スティック入力
	XMFLOAT2 leftStick = GetLeftStick();

	// キーボード入力 (W/A/S/D) をベクトル化
	XMFLOAT2 keyInput = XMFLOAT2(0.0f, 0.0f);

	if (IsKeyPress('W')) // W: 前進 (Z軸+)
	{
		keyInput.y += 1.0f;
	}
	if (IsKeyPress('S')) // S: 後退 (Z軸-)
	{
		keyInput.y -= 1.0f;
	}
	if (IsKeyPress('A')) // A: 左移動 (X軸-)
	{
		keyInput.x -= 1.0f;
	}
	if (IsKeyPress('D')) // D: 右移動 (X軸+)
	{
		keyInput.x += 1.0f;
	}

	// キーボードとスティックの入力を合成
	// コントローラーが優先されるように、または単純に加算して正規化
	// ここでは単純に加算し、正規化後にスティック入力を優先するように調整します
	XMVECTOR totalInput = XMVectorSet(leftStick.x + keyInput.x, 0.0f, leftStick.y + keyInput.y, 0.0f);

	// 入力ベクトルの長さをチェック (デッドゾーン/キー押し判定)
	float inputLengthSq = XMVectorGetX(XMVector3LengthSq(totalInput));

	// Bボタン入力
	bool isBTriggered = IsButtonTriggered(BUTTON_B) || IsKeyPress(VK_SPACE);

	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// 必要なコンポーネントを取得
		auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		auto& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
		auto& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entity);

		// =====================================
		// 1. 移動 (カメラ基準での移動)
		// =====================================

		// 移動入力がある場合
		if (inputLengthSq > 0.01f) // 0.01f は許容誤差
		{
			// 1. 移動方向を正規化し、速度を乗算
			XMVECTOR normalizedInput = XMVector3Normalize(totalInput);

			// 2. カメラのYAW角に基づいた回転行列を構築
			// Playerの移動はX-Z平面で行うため、Y軸周りの回転行列を使用
			XMMATRIX rotationMatrix = XMMatrixRotationY(cameraYaw);

			// 3. 入力ベクトルを回転させて、ワールド座標系での移動方向 moveVector を得る
			XMVECTOR rotatedMoveVector = XMVector3TransformNormal(normalizedInput, rotationMatrix);

			// 速度を乗算
			XMVECTOR moveVector = rotatedMoveVector * playerControl.moveSpeed;

			// ----------------------------------------------------

			// 4. 進行方向への自動回転
			// ターゲット角度を、回転後のワールド方向ベクトルから算出
			float targetAngle = atan2f(XMVectorGetX(rotatedMoveVector), XMVectorGetZ(rotatedMoveVector));

			float currentAngle = transform.rotation.y;
			float deltaAngle = targetAngle - currentAngle;

			// 180度以上の差を補正
			if (deltaAngle > XM_PI) deltaAngle -= XM_2PI;
			if (deltaAngle < -XM_PI) deltaAngle += XM_2PI;

			// スムーズな回転（線形補間: 0.15fは応答速度）
			transform.rotation.y += deltaAngle * 0.15f;

			// 2πの範囲に収める
			if (transform.rotation.y > XM_PI) transform.rotation.y -= XM_2PI;
			if (transform.rotation.y < -XM_PI) transform.rotation.y += XM_2PI;

			// 5. RigidBodyの速度を更新
			rigidBody.velocity.x = XMVectorGetX(moveVector);
			rigidBody.velocity.z = XMVectorGetZ(moveVector);
		}
		else
		{
			// 入力がない場合は停止
			rigidBody.velocity.x = 0.0f;
			rigidBody.velocity.z = 0.0f;
		}

		// Y軸の速度（ジャンプや重力）はPhysicsSystemに委ねるため、ここでは変更しない

		// =====================================
		// 3. アイテム回収 (Bボタン)
		// =====================================
		playerControl.isItemStealTriggered = isBTriggered;
	}
}