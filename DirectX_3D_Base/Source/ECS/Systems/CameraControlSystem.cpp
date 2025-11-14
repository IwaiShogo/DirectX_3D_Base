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
#include "ECS/ECS.h"
#include "Systems/Geometory.h"	// カメラ設定関数を使用
#include "Systems/Input.h"		// GetRightStick()
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
const float TOP_VIEW_HEIGHT = 60.0f;

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
 * @brief ワールド座標をグリッド座標 (XMINT2) に変換するヘルパー関数
 * @note GuardAISystem.cpp と同様のロジック
 */
static XMINT2 GetGridPosition(const XMFLOAT3& worldPos)
{
    float x_f = (worldPos.x - X_ADJUSTMENT + MAP_CENTER_OFFSET) / TILE_SIZE;
    float y_f = (worldPos.z - Z_ADJUSTMENT + MAP_CENTER_OFFSET) / TILE_SIZE;

    int x = static_cast<int>(x_f);
    int y = static_cast<int>(y_f);

    x = std::min(std::max(0, x), MAP_GRID_SIZE - 1);
    y = std::min(std::max(0, y), MAP_GRID_SIZE - 1);

    return { x, y };
}

/**
 * @brief 2点間レイキャストを行い、最初の壁との衝突点を求める
 * @param start - 始点 (プレイヤー位置)
 * @param end - 終点 (目標カメラ位置)
 * @param mapComp - マップデータ
 * @param [out] hitPoint - 衝突点 (壁の手前) のワールド座標
 * @return bool - 衝突した場合は true
 */
static bool RaycastToWall(
    const XMFLOAT3& start,
    const XMFLOAT3& end,
    const MapComponent& mapComp,
    XMFLOAT3& hitPoint
)
{
    // 距離が近すぎる場合は衝突チェックをスキップ
    XMVECTOR startV = XMLoadFloat3(&start);
    XMVECTOR endV = XMLoadFloat3(&end);
    if (XMVectorGetX(XMVector3LengthSq(startV - endV)) < 0.1f * 0.1f)
    {
        hitPoint = end;
        return false;
    }

    // 1. DDAアルゴリズムの初期化 (XZ平面のみをチェック)
    XMINT2 startGrid = GetGridPosition(start);
    XMINT2 endGrid = GetGridPosition(end);

    // X, Y (Z軸) 方向のデルタ
    float deltaX = end.x - start.x;
    float deltaY = end.z - start.z; // Z軸をYとして扱う

    // ステップ方向とグリッド上の現在位置
    int stepX = (deltaX > 0) ? 1 : -1;
    int stepY = (deltaY > 0) ? 1 : -1;
    int currentX = startGrid.x;
    int currentY = startGrid.y;

    // 2. グリッドトラバーサル
    // 衝突点から最も近い壁セルの中心までを衝突と見なす
    while (currentX != endGrid.x || currentY != endGrid.y)
    {
        // 境界チェック
        if (currentX < 0 || currentX >= MAP_GRID_SIZE || currentY < 0 || currentY >= MAP_GRID_SIZE)
        {
            break;
        }

        // 壁チェック: 現在のセルが壁なら衝突
        if (mapComp.grid[currentY][currentX].type == CellType::Wall)
        {
            // 衝突点までの距離を計算 (プレイヤー位置から現在の壁セルまでの距離を利用)

            // 壁の中心座標を計算（カメラを壁の中心から少し手前に引き戻すための基準点）
            float wallCenterWorldX = (float)currentX * TILE_SIZE - MAP_CENTER_OFFSET + X_ADJUSTMENT + TILE_SIZE / 2.0f;
            float wallCenterWorldZ = (float)currentY * TILE_SIZE - MAP_CENTER_OFFSET + Z_ADJUSTMENT + TILE_SIZE / 2.0f;

            XMVECTOR wallCenterV = XMVectorSet(wallCenterWorldX, start.y, wallCenterWorldZ, 1.0f);

            // プレイヤーから壁の中心への方向ベクトル
            XMVECTOR dirToWall = wallCenterV - startV;

            // 壁の手前に引き戻す距離（例：セルの半分 1.0f、またはそれ以上）
            float safetyOffset = TILE_SIZE * 0.75f;

            // プレイヤーから目標カメラ位置への方向ベクトルを正規化
            XMVECTOR dirNormalized = XMVector3Normalize(endV - startV);

            // 衝突点: プレイヤー位置 + (壁までの距離 - 安全オフセット) * 方向ベクトル
            float t = XMVectorGetX(XMVector3Length(wallCenterV - startV)) - safetyOffset;

            // カメラを壁の手前、かつプレイヤーとの距離が最小距離以上になるように調整
            float minDistance = TILE_SIZE * 1.5f; // プレイヤーの体積やオフセットを考慮した最小距離

            if (t < minDistance) {
                t = minDistance; // 最小距離を確保
            }

            XMVECTOR adjustedPos = startV + dirNormalized * t;

            XMStoreFloat3(&hitPoint, adjustedPos);

            return true; // 衝突検出
        }

        // 次のグリッドセルへ移動 (単純化のため、毎回1セルずつ移動)
        // Y軸がZ方向に対応していることに注意
        currentX += stepX;
        currentY += stepY;
    }

    hitPoint = end; // 衝突なし
    return false;
}

/**
 * @brief カメラの位置を計算し、ビュー・プロジェクション行列を設定する
 */
void CameraControlSystem::Update()
{
    // 1. ゲームステートとデバッグ状態の取得
    ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    GameMode currentMode = GameMode::ACTION_MODE;
    bool isDebugMode = false;

    if (controllerID != ECS::INVALID_ENTITY_ID)
    {
        currentMode = m_coordinator->GetComponent<GameStateComponent>(controllerID).currentMode;
        // DebugComponentもGameController Entityに付与されている前提
        if (m_coordinator->m_entityManager->GetSignature(controllerID).test(m_coordinator->GetComponentTypeID<DebugComponent>()))
        {
            isDebugMode = m_coordinator->GetComponent<DebugComponent>(controllerID).isDebugModeActive;
        }
    }

    // 2. 【フリーカメラ】デバッグモードが有効な場合
    if (isDebugMode)
    {
        // カメラの現在の位置と注視点を直接操作するためのロジック
        // ※ 既存のm_currentCameraPosとm_currentLookAtを直接更新します。

        // 右スティック（またはマウス）でYAW/PITCHを更新 (既存のロジックを再利用)
        XMFLOAT2 rightStick = GetRightStick();
        m_currentYaw += rightStick.x * CAMERA_SENSITIVITY_X * 2.0f; // 感度を上げる
        m_currentPitch -= rightStick.y * CAMERA_SENSITIVITY_Y * 2.0f;
        m_currentPitch = std::max(PITCH_MIN, std::min(PITCH_MAX, m_currentPitch));

        // YAW/PITCHから前方/右方向ベクトルを計算
        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_currentPitch, m_currentYaw, 0.0f);
        XMVECTOR forwardV = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
        XMVECTOR rightV = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix);
        XMVECTOR upV = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        // キーボード（または左スティック）でカメラ位置を移動
        XMVECTOR currentPosV = XMLoadFloat3(&m_currentCameraPos);
        float moveSpeed = 0.5f; // デバッグカメラの移動速度

        if (IsKeyPress(VK_UP)) currentPosV += forwardV * moveSpeed;
        if (IsKeyPress(VK_DOWN)) currentPosV -= forwardV * moveSpeed;
        if (IsKeyPress(VK_LEFT)) currentPosV -= rightV * moveSpeed;
        if (IsKeyPress(VK_RIGHT)) currentPosV += rightV * moveSpeed;
        if (IsKeyPress(VK_SHIFT)) currentPosV += upV * moveSpeed; // 上昇
        if (IsKeyPress(VK_CONTROL)) currentPosV -= upV * moveSpeed; // 下降

        XMStoreFloat3(&m_currentCameraPos, currentPosV);
        m_currentLookAt = XMFLOAT3(
            m_currentCameraPos.x + XMVectorGetX(forwardV),
            m_currentCameraPos.y + XMVectorGetY(forwardV),
            m_currentCameraPos.z + XMVectorGetZ(forwardV)
        );

        // 追従ロジックをスキップし、ビュー行列を直接計算して終了
        // ※ ここでビュー行列とプロジェクション行列を計算し、RenderSystemのAPIに設定します。
        // ... (View/Projection行列の計算ロジックはACTION_MODEのステップ4を再利用) ...




        return; // 通常のカメラロジックは実行しない
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

                // --- 衝突回避前の目標カメラ位置 ---
                XMVECTOR rawTargetCamPosV = focusPos + rotatedOffset;
                targetLookAtV = focusPos;

                // ----------------------------------------------------
                // ★★★ BUG-02修正: ウォールクリッピング回避 ★★★
                // ----------------------------------------------------
                XMFLOAT3 rawTargetCamPosF;
                XMStoreFloat3(&rawTargetCamPosF, rawTargetCamPosV);

                XMFLOAT3 adjustedCamPosF;

                // MapComponentを持つEntityを取得
                ECS::EntityID mapEntity = ECS::FindFirstEntityWithComponent<MapComponent>(m_coordinator);
                if (mapEntity != ECS::INVALID_ENTITY_ID) {
                    const MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);

                    // プレイヤー位置から目標カメラ位置へレイキャスト
                    XMFLOAT3 focusPosF = focusTrans.position;

                    if (RaycastToWall(focusPosF, rawTargetCamPosF, mapComp, adjustedCamPosF))
                    {
                        // 衝突した場合、調整された位置を新しい目標位置とする
                        targetCamPosV = XMLoadFloat3(&adjustedCamPosF);
                    }
                    else
                    {
                        // 衝突しない場合、元の目標位置を使用
                        targetCamPosV = rawTargetCamPosV;
                    }
                }
                else {
                    // MapComponentがない場合、元の目標位置を使用
                    targetCamPosV = rawTargetCamPosV;
                }

                // 【補間係数】通常の追従速度に戻す
                cameraComp.followSpeed = 0.1f;
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
            XMMATRIX projectionMatrix;
            if (currentMode == GameMode::SCOUTING_MODE)
            {
                // 平行投影のパラメータ
                // 画面サイズ (SCREEN_WIDTH/HEIGHT) とカメラの視野の大きさを元に計算
                // 例: カメラコンポーネントのFOVを、平行投影のビューポートの高さとして利用する
                float viewHeight = cameraComp.FOV * TOP_VIEW_HEIGHT; // FOVを基準に平行投影の視野高さを決定 (調整が必要)
                float viewWidth = viewHeight * ((float)SCREEN_WIDTH / SCREEN_HEIGHT);

                projectionMatrix = XMMatrixOrthographicLH(
                    viewWidth,
                    viewHeight,
                    cameraComp.nearClip,
                    cameraComp.farClip
                );
            }
            else
            {
                projectionMatrix = XMMatrixPerspectiveFovLH(
                    cameraComp.FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, cameraComp.nearClip, cameraComp.farClip
                );
            }

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