/*****************************************************************//**
 * @file	PlayerControlSystem.cpp
 * @brief	プレイヤーのキー入力に基づいてEntityの運動状態を走査するSystemの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Oda Kaito
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
#include "ECS/Systems/Gameplay/PlayerControlSystem.h"
#include "ECS/ECS.h"
#include "ECS/EntityFactory.h"

#include "ECS/Components/Rendering/AnimationComponent.h"
#include "ECS/Components/Gameplay/PlayerControlComponent.h"
#include "ECS/Components/Physics/RigidBodyComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "ECS/Systems/Core/CameraControlSystem.h"
#include <cmath>

#include <iostream>

using namespace DirectX;

/**
 * @brief 入力に応じてRigidBodyの速度を更新する
 */
void PlayerControlSystem::Update(float deltaTime)
{
	// CameraControlSystemはECSInitializerによって登録されている前提
	auto cameraSystem = ECS::ECSInitializer::GetSystem<CameraControlSystem>();
	if (!cameraSystem) return; // カメラシステムが存在しなければ処理を中断

	// 現在のカメラのYAW角を取得
	// ※ PlayerControlSystem.hでCameraControlSystemをインクルードし、
	// m_currentYaw が public/friend であるか、publicなgetterを持つことを前提とします。
	// プロトタイプのため、ここではアクセス可能と仮定します。
	float cameraYaw = cameraSystem->m_currentYaw;

	ECS::EntityID gameControllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
	
	if (gameControllerID != ECS::INVALID_ENTITY_ID)
	{
		auto& state = m_coordinator->GetComponent<GameStateComponent>(gameControllerID);

		// 条件: 
		// 1. 偵察モード (SCOUTING_MODE) の場合
		// 2. プレイ中 (Playing) ではない場合 (Entering:入場演出, Exiting:脱出演出)
		bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);
		bool isCutscene = (state.sequenceState != GameSequenceState::Playing);

		if (isScouting || isCutscene)
		{
			// プレイヤーエンティティ全ての動きを止める
			for (auto const& entity : m_entities)
			{
				auto& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
				// 慣性で滑らないように速度をゼロにする
				rigidBody.velocity.x = 0.0f;
				rigidBody.velocity.z = 0.0f;
				// rigidBody.velocity.y = 0.0f; // 重力落下はさせたい場合はYは触らない
			}

			// ここでリターンして、以降のキー入力処理を行わない
			return;
		}
	}

	// =====================================
	// 入力取得とベクトル合成
	// =====================================

	// 1. Xboxコントローラーの左スティック入力 (0.0 ~ 1.0)
	XMFLOAT2 leftStick = GetLeftStick();

	// 2. キーボード入力 (W/A/S/D) をベクトル化 (-1.0, 0.0, 1.0)
	XMFLOAT2 keyInput = XMFLOAT2(0.0f, 0.0f);

	if (IsKeyPress('W')) keyInput.y += 1.0f;
	if (IsKeyPress('S')) keyInput.y -= 1.0f;
	if (IsKeyPress('A')) keyInput.x -= 1.0f;
	if (IsKeyPress('D')) keyInput.x += 1.0f;

	// 3. 入力の合成
	XMVECTOR stickV = XMLoadFloat2(&leftStick);
	XMVECTOR keyV = XMLoadFloat2(&keyInput);

	// 単純加算 (Stick + Keyboard)
	XMVECTOR totalInputV = XMVectorAdd(stickV, keyV);

	// 入力強度 (Magnitude) の計算
	float inputMagnitude = XMVectorGetX(XMVector2Length(totalInputV));

	// デッドゾーン処理 (微小入力の無視)
	if (inputMagnitude < 0.1f)
	{
		inputMagnitude = 0.0f;
		totalInputV = XMVectorZero();
	}
	else
	{
		// 4. 正規化とクランプ (アナログ操作への対応)
		// Magnitudeが1.0を超えている場合（キー同時押しやStick+Key）は1.0に制限
		// 1.0未満の場合（スティックを少し倒した状態）は、その強度を維持する
		if (inputMagnitude > 1.0f)
		{
			totalInputV = XMVector2Normalize(totalInputV);
			inputMagnitude = 1.0f;
		}
		// ※ここでNormalizeしてしまうと「歩き」ができなくなるため、
		//  方向だけNormalizeし、長さはinputMagnitudeを使うアプローチをとるか、
		//  あるいは既にLengthが1以下ならそのまま使う。
		//  ここでは「入力ベクトルそのもの」を速度係数として扱います。
	}

	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// 必要なコンポーネントを取得
		auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		auto& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
		auto& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entity);
		auto& animComp = m_coordinator->GetComponent<AnimationComponent>(entity); 


		if (IsKeyPress('E'))
		{
			ECS::EntityFactory::CreateOneShotEffect(m_coordinator, "EFK_TEST", transform.position, 6.0f);
		}

		// =====================================
		// 1. 移動 (カメラ基準での移動)
		// =====================================

		// 1. 移動処理
		if (inputMagnitude > 0.0f)
		{
			// カメラの向きに合わせて入力ベクトルを回転
			// 入力は2D (x, y) だが、3D空間では (x, 0, z) に対応する
			// totalInputV.x -> Move X, totalInputV.y -> Move Z

			float inputX = XMVectorGetX(totalInputV);
			float inputZ = XMVectorGetY(totalInputV); // 2DのYを3DのZとして扱う

			XMVECTOR moveDirectionLocal = XMVectorSet(inputX, 0.0f, inputZ, 0.0f);

			// Y軸回転行列 (カメラのYaw)
			XMMATRIX rotationMatrix = XMMatrixRotationY(cameraYaw);

			// ワールド座標系での移動ベクトル
			XMVECTOR moveVectorWorld = XMVector3TransformNormal(moveDirectionLocal, rotationMatrix);

			// 最終速度 = 方向 * 入力強度(0~1) * プレイヤーの最高速度
			// ※ inputMagnitude は totalInputV に既に含まれているとも言えるが、
			//    totalInputVの長さそのものが速度係数になる
			XMVECTOR finalVelocity = moveVectorWorld * playerControl.moveSpeed;

			// RigidBodyに設定
			rigidBody.velocity.x = XMVectorGetX(finalVelocity);
			rigidBody.velocity.z = XMVectorGetZ(finalVelocity);

			// 2. キャラクターの向きの自動回転 (進行方向を向く)
			// 入力が十分にある場合のみ回転更新
			if (inputMagnitude > 0.1f)
			{
				float targetAngle = atan2f(XMVectorGetX(moveVectorWorld), XMVectorGetZ(moveVectorWorld));
				float currentAngle = transform.rotation.y;

				// 角度差の計算と正規化 (-PI ~ PI)
				float deltaAngle = targetAngle - currentAngle;
				while (deltaAngle > XM_PI) deltaAngle -= XM_2PI;
				while (deltaAngle < -XM_PI) deltaAngle += XM_2PI;

				// スムーズな回転 (補間)
				transform.rotation.y += deltaAngle * 0.15f;

				// 回転の正規化
				if (transform.rotation.y > XM_PI) transform.rotation.y -= XM_2PI;
				if (transform.rotation.y < -XM_PI) transform.rotation.y += XM_2PI;
			}
		}
		else
		{
			// 入力なし -> 停止
			rigidBody.velocity.x = 0.0f;
			rigidBody.velocity.z = 0.0f;
		}

		// =====================================
	   // 2. アニメーション状態の決定
	   // =====================================
	   // 速度から「動いているか」を判定（わずかな誤差も考えて閾値を設ける）
		const float moveThreshold = 0.01f;
		bool isMoving =
			(std::fabs(rigidBody.velocity.x) > moveThreshold) ||
			(std::fabs(rigidBody.velocity.z) > moveThreshold);

		PlayerAnimState desiredState = isMoving
			? PlayerAnimState::Run
			: PlayerAnimState::Idle;

		// 状態に変化があるときだけ AnimationComponent に命令を送る
		if (desiredState != playerControl.animState)
		{
			playerControl.animState = desiredState;

			if (desiredState == PlayerAnimState::Run)
			{
				// 走りアニメへ
				animComp.PlayBlend("A_PLAYER_RUN", 0.3f);
			}
			else
			{
				// 待機アニメへ
				animComp.PlayBlend("A_PLAYER_IDLE", 0.3f);
			}
		}
	}
}