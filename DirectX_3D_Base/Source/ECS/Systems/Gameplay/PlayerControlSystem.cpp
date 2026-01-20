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
	if (m_coordinator)
	{
		ECS::EntityID stateID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
		if (stateID != ECS::INVALID_ENTITY_ID)
		{
			if (m_coordinator->GetComponent<GameStateComponent>(stateID).isPaused) return;
		}
	}

	auto cameraSystem = ECS::ECSInitializer::GetSystem<CameraControlSystem>();
	if (!cameraSystem) return;

	float cameraYaw = cameraSystem->m_currentYaw;

	// =====================================
	// 1. 必要な変数と入力状態を「関数の最初」で取得
	// =====================================
	ECS::EntityID gameControllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
	if (gameControllerID == ECS::INVALID_ENTITY_ID) return;

	// ここで定義することで、下のループ内でもエラーにならずに使用可能
	auto& state = m_coordinator->GetComponent<GameStateComponent>(gameControllerID);
	bool isScouting = (state.currentMode == GameMode::SCOUTING_MODE);
	bool isCutscene = (state.sequenceState != GameSequenceState::Playing);

	// Space/Aボタンのトリガー判定（正常に動いているやり方）
	bool pressedSpace = IsKeyTrigger(VK_SPACE);
	bool pressedA = IsButtonTriggered(BUTTON_A);

	// 偵察中・演出中は移動のみ制限して終了
	if (isScouting || isCutscene)
	{
		for (auto const& entity : m_entities)
		{
			auto& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
			rigidBody.velocity.x = 0.0f;
			rigidBody.velocity.z = 0.0f;
		}
		return;
	}

	// =====================================
	// 2. 移動入力の計算
	// =====================================
	XMFLOAT2 leftStick = GetLeftStick();
	XMFLOAT2 keyInput = XMFLOAT2(0.0f, 0.0f);
	if (IsKeyPress('W')) keyInput.y += 1.0f;
	if (IsKeyPress('S')) keyInput.y -= 1.0f;
	if (IsKeyPress('A')) keyInput.x -= 1.0f;
	if (IsKeyPress('D')) keyInput.x += 1.0f;

	XMVECTOR totalInputV = XMVectorAdd(XMLoadFloat2(&leftStick), XMLoadFloat2(&keyInput));
	float inputMagnitude = XMVectorGetX(XMVector2Length(totalInputV));

	if (inputMagnitude < 0.1f) {
		inputMagnitude = 0.0f;
		totalInputV = XMVectorZero();
	}
	else if (inputMagnitude > 1.0f) {
		totalInputV = XMVector2Normalize(totalInputV);
		inputMagnitude = 1.0f;
	}

	// =====================================
	// 3. エンティティごとの処理ループ
	// =====================================
	for (auto const& entity : m_entities)
	{
		auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		auto& rigidBody = m_coordinator->GetComponent<RigidBodyComponent>(entity);
		auto& playerControl = m_coordinator->GetComponent<PlayerControlComponent>(entity);
		auto& animComp = m_coordinator->GetComponent<AnimationComponent>(entity);

		// --- ギミック判定 ＋ 入力判定 ---
		if (pressedSpace || pressedA)
		{
			auto const& allEntities = m_coordinator->GetActiveEntities();
			for (auto const& otherEntity : allEntities)
			{
				if (otherEntity == entity) continue;
				if (!m_coordinator->HasComponent<TagComponent>(otherEntity)) continue;

				auto& tagComp = m_coordinator->GetComponent<TagComponent>(otherEntity);

				if (tagComp.tag == "TopViewTrigger" || tagComp.tag == "map_gimmick")
				{
					auto& otherTransform = m_coordinator->GetComponent<TransformComponent>(otherEntity);

					// 板のサイズ（Scale）の半分を取得
					float halfW = otherTransform.scale.x * 0.5f;
					float halfD = otherTransform.scale.z * 0.5f;

					// プレイヤーと板の中心座標の差を計算
					float diffX = std::abs(transform.position.x - otherTransform.position.x);
					float diffZ = std::abs(transform.position.z - otherTransform.position.z);

					// ? 判定：板の範囲内（遊びとして+0.5fの余裕を持たせる）にプレイヤーがいれば実行
					if (diffX <= (halfW + 0.5f) && diffZ <= (halfD + 0.5f))
					{
						// モード切替
						state.currentMode = GameMode::SCOUTING_MODE;

						// カメラの設定
						cameraSystem->m_currentPitch = -1.57f; // 真下を向く
						cameraSystem->m_currentYaw = 0.0f;

						// 見た目をトップビュー用に一新
						auto gameControlSystem = ECS::ECSInitializer::GetSystem<GameControlSystem>();
						if (gameControlSystem) {
							gameControlSystem->ApplyModeVisuals(gameControllerID);
						}

						// ギミックを消す（もし一度きりにしたいなら）
						m_coordinator->DestroyEntity(otherEntity);

						std::cout << "[SUCCESS] Scouting Mode ON!" << std::endl;
						return; // 処理完了
					}
				}
			}
		}

		// トップビューに切り替わった直後なら移動をスキップ
		if (isScouting) continue;

		// --- 移動処理 ---
		if (inputMagnitude > 0.0f)
		{
			float inputX = XMVectorGetX(totalInputV);
			float inputZ = XMVectorGetY(totalInputV);
			XMVECTOR moveDirectionLocal = XMVectorSet(inputX, 0.0f, inputZ, 0.0f);
			XMMATRIX rotationMatrix = XMMatrixRotationY(cameraYaw);
			XMVECTOR moveVectorWorld = XMVector3TransformNormal(moveDirectionLocal, rotationMatrix);
			XMVECTOR finalVelocity = moveVectorWorld * playerControl.moveSpeed;

			rigidBody.velocity.x = XMVectorGetX(finalVelocity);
			rigidBody.velocity.z = XMVectorGetZ(finalVelocity);

			// 回転
			float targetAngle = atan2f(XMVectorGetX(moveVectorWorld), XMVectorGetZ(moveVectorWorld));
			float currentAngle = transform.rotation.y;
			float deltaAngle = targetAngle - currentAngle;
			while (deltaAngle > XM_PI) deltaAngle -= XM_2PI;
			while (deltaAngle < -XM_PI) deltaAngle += XM_2PI;
			transform.rotation.y += deltaAngle * 0.15f;
		}
		else
		{
			rigidBody.velocity.x = 0.0f;
			rigidBody.velocity.z = 0.0f;
		}

		// --- アニメーション ---
		const float moveThreshold = 0.01f;
		bool isMoving = (std::fabs(rigidBody.velocity.x) > moveThreshold) || (std::fabs(rigidBody.velocity.z) > moveThreshold);
		PlayerAnimState dState = isMoving ? PlayerAnimState::Run : PlayerAnimState::Idle;

		if (dState != playerControl.animState)
		{
			playerControl.animState = dState;
			if (dState == PlayerAnimState::Run) animComp.PlayBlend("A_PLAYER_RUN", 0.3f);
			else animComp.PlayBlend("A_PLAYER_IDLE", 0.3f);
		}
	}
}