/*****************************************************************//**
 * @file	CameraControlSystem.cpp
 * @brief	CameraControlSystemの実装。カメラの追従とビュー行列の更新を行う。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Oda Kaito
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
#include "ECS/Components/GameStateComponent.h"
#include <algorithm>

using namespace DirectX;

// ===== 定数・マクロ定義 =====
// 右スティックの感度
const float CAMERA_SENSITIVITY_X = 0.04f; // YAW感度
const float CAMERA_SENSITIVITY_Y = 0.03f; // PITCH感度 (上下は控えめに)
// PITCHのクランプ値 (上下の限界)
const float PITCH_MAX = -XM_PIDIV2 - 0.2f; // 約78度
const float PITCH_MIN = XM_PIDIV2 * 0.3f; // 約-27度 (水平より少し下まで)
// トップビューカメラの高さ定数
const float TOP_VIEW_HEIGHT = 15.0f;

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
    // 1. ゲームステートの取得 (GameController Entityは1つのみと想定)
    GameMode currentMode = GameMode::ACTION_MODE;

    // 【前提】GameController Entity IDを特定するロジック (ここでは簡略化のため最初の GameStateComponent を持つ Entityを探す)
    ECS::EntityID gameControllerID = 0;
    std::set<ECS::EntityID> allEntities = m_coordinator->GetActiveEntities();

    for (auto const& entity : allEntities) // このSystemの対象はCameraComponentだが、ここではCoordinatorに問い合わせる
    {
        if (m_coordinator->m_entityManager->GetSignature(entity).test(m_coordinator->GetComponentTypeID<GameStateComponent>()))
        {
            gameControllerID = entity; // カメラEntity自身ではないため、このロジックは不正確。
            currentMode = m_coordinator->GetComponent<GameStateComponent>(gameControllerID).currentMode;
            // より正確には、Coordinatorが保持するEntityリスト全体から探す必要があるが、ここでは簡略化。
            // ※ 適切なGameController EntityIDの取得方法を別途用意する必要がありますが、ここではデモとしてEntityIDを固定するか、別途グローバル変数に格納する前提とします。
        }
    }

    // 2. 右スティック入力の取得 (アクションモードでのみ有効)
    XMFLOAT2 rightStick = (currentMode == GameMode::ACTION_MODE) ? GetRightStick() : XMFLOAT2(0.0f, 0.0f);


    for (auto const& entity : m_entities) // CameraComponentを持つEntityを対象
    {
        auto& cameraComp = m_coordinator->GetComponent<CameraComponent>(entity);
        ECS::EntityID focusID = cameraComp.focusEntityID;

        if (focusID != ECS::INVALID_ENTITY_ID &&
            m_coordinator->m_entityManager->GetSignature(focusID).test(m_coordinator->GetComponentTypeID<TransformComponent>()))
        {
            TransformComponent& focusTrans = m_coordinator->GetComponent<TransformComponent>(focusID);

            XMVECTOR targetCamPosV;
            XMVECTOR targetLookAtV;

            if (currentMode == GameMode::SCOUTING_MODE)
            {
                // =====================================
                // 3-A. 偵察モード (トップビュー) - 視点固定
                // =====================================

                const XMFLOAT3 VOID_CAMERA_POS = XMFLOAT3(0.0f, TOP_VIEW_HEIGHT, 0.0f);
                const XMFLOAT3 VOID_LOOKAT_POS = XMFLOAT3(0.0f, 0.0f, 0.0f);

                // カメラ回転の状態を強制的にトップビューにリセット
                m_currentYaw = 0.0f;
                m_currentPitch = -XM_PIDIV2;

                // 目標カメラ位置: ターゲットの真上固定オフセット
                targetCamPosV = XMLoadFloat3(&VOID_CAMERA_POS);
                targetLookAtV = XMLoadFloat3(&VOID_LOOKAT_POS);

                m_currentCameraPos = VOID_CAMERA_POS;
                m_currentLookAt = VOID_LOOKAT_POS;

                // 【補間係数】トップビュー切り替え時は瞬時に移動
                cameraComp.followSpeed = 1.0f;
            }
            else // ACTION_MODE (三人称視点)
            {
                // =====================================
                // 3-B. アクションモード (三人称) - 右スティック制御
                // =====================================

                // YAW/PITCHを右スティックで更新
                m_currentYaw += rightStick.x * CAMERA_SENSITIVITY_X;
                m_currentPitch += rightStick.y * CAMERA_SENSITIVITY_Y;

                // PITCH角のクランプ（BUG-03修正）
                m_currentPitch = std::max(PITCH_MIN, std::min(PITCH_MAX, m_currentPitch));

                // YAW/PITCHに基づいた回転オフセットの計算
                XMVECTOR defaultOffset = XMLoadFloat3(&cameraComp.offset);
                float offsetLength = XMVectorGetX(XMVector3Length(defaultOffset));

                XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_currentPitch, m_currentYaw, 0.0f);

                XMVECTOR rotatedOffset = XMVector3Transform(
                    XMVectorSet(0.0f, 0.0f, -offsetLength, 0.0f),
                    rotationMatrix
                );

                XMVECTOR focusPos = XMLoadFloat3(&focusTrans.position);

                targetCamPosV = focusPos + rotatedOffset;
                targetLookAtV = focusPos;

                // 【補間係数】通常の追従速度に戻す
                cameraComp.followSpeed = 0.1f; // 例として初期値の0.1fに戻す
            }

            // =====================================
            // 3. カメラ位置と注視点の補間 (共通処理)
            // =====================================
            XMVECTOR currentCamPosV = XMLoadFloat3(&m_currentCameraPos);
            XMVECTOR currentLookAtV = XMLoadFloat3(&m_currentLookAt);

            // 通常の追従速度をセット
            float actualFollowSpeed = cameraComp.followSpeed;

            // Lerp（線形補間）: 変更した追従速度(actualFollowSpeed)を使用する
            XMVECTOR newCamPosV = XMVectorLerp(currentCamPosV, targetCamPosV, actualFollowSpeed);
            XMVECTOR newLookAtV = XMVectorLerp(currentLookAtV, targetLookAtV, actualFollowSpeed);

            XMStoreFloat3(&m_currentCameraPos, newCamPosV);
            XMStoreFloat3(&m_currentLookAt, newLookAtV);

            // =====================================
            // 4. ビュー行列の計算と設定 (共通処理)
            // =====================================
            XMVECTOR camPos = XMLoadFloat3(&m_currentCameraPos);
            XMVECTOR lookAt = XMLoadFloat3(&m_currentLookAt);

            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            if (currentMode == GameMode::SCOUTING_MODE)
            {
                // ★トップビューの場合、カメラの視線(Y軸)と平行でない、水平方向のベクトルをアップとする
                up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // Z軸をアップベクトルとする
            }
            else
            {
                // アクションモードの場合、通常のY軸をアップとする
                up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            }

            // View行列
            XMMATRIX viewMatrix = XMMatrixLookAtLH(newCamPosV, newLookAtV, up);

            // Projection行列
            XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
                cameraComp.FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, cameraComp.nearClip, cameraComp.farClip
            );

            // DirectXへ渡すために転置して格納
            XMFLOAT4X4 fMatView;
            XMStoreFloat4x4(&fMatView, XMMatrixTranspose(viewMatrix));

            XMFLOAT4X4 fMatProjection;
            XMStoreFloat4x4(&fMatProjection, XMMatrixTranspose(projectionMatrix));

            // 計算結果をCameraComponentに格納
            cameraComp.viewMatrix = fMatView;
            cameraComp.projectionMatrix = fMatProjection;
            cameraComp.worldPosition = m_currentCameraPos; // カメラ位置も格納
        }
    }
}