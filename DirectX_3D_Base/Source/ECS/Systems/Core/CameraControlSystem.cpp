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
const float CAMERA_SENSITIVITY_Y = 0.02f; // PITCH感度 (上下は控えめに)
// マウスの感度（ピクセル単位なのでスティックより小さく設定）
const float MOUSE_SENSITIVITY = 0.005f;
// PITCHのクランプ値 (上下の限界)
const float PITCH_MAX = XM_PIDIV2 - 0.2f; // 約78度
const float PITCH_MIN = XM_PIDIV2 * 0.0f; // 約-27度 (水平より少し下まで)
// トップビューカメラの高さ定数
const float TOP_VIEW_HEIGHT = 100.0f;

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
static XMINT2 GetGridPosition(const XMFLOAT3& worldPos, const MapComponent& mapComp)
{
    const float MAP_CENTER_OFFSET_X = (mapComp.gridSizeX / 2.0f) * mapComp.tileSize; // 20.0f
    const float MAP_CENTER_OFFSET_Y = (mapComp.gridSizeY / 2.0f) * mapComp.tileSize;
    const float X_ADJUSTMENT = 0.5f * mapComp.tileSize; // 1.0f
    const float Z_ADJUSTMENT = 1.0f * mapComp.tileSize; // 2.0f

    float x_f = (worldPos.x - X_ADJUSTMENT + MAP_CENTER_OFFSET_X) / mapComp.tileSize;
    float y_f = (worldPos.z - Z_ADJUSTMENT + MAP_CENTER_OFFSET_Y) / mapComp.tileSize;

    int x = static_cast<int>(x_f);
    int y = static_cast<int>(y_f);

    x = std::min(std::max(0, x), mapComp.gridSizeX - 1);
    y = std::min(std::max(0, y), mapComp.gridSizeY - 1);

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
    const float CAMERA_SAFETY_OFFSET = mapComp.tileSize * 0.25f;
    const float MAP_CENTER_OFFSET_X = (mapComp.gridSizeX / 2.0f) * mapComp.tileSize; // 20.0f
    const float MAP_CENTER_OFFSET_Y = (mapComp.gridSizeY / 2.0f) * mapComp.tileSize;
    const float X_ADJUSTMENT = 0.5f * mapComp.tileSize; // 1.0f
    const float Z_ADJUSTMENT = 1.0f * mapComp.tileSize; // 2.0f

    // 距離が近すぎる場合は衝突チェックをスキップ
    XMVECTOR startV = XMLoadFloat3(&start);
    XMVECTOR endV = XMLoadFloat3(&end);

    // プレイヤーと目標カメラ位置の距離ベクトルと高さ
    XMVECTOR directionV = endV - startV;
    float distance = XMVectorGetX(XMVector3Length(directionV));

    if (distance < CAMERA_SAFETY_OFFSET)
    {
        hitPoint = end;
        return false;
    }

    // 方向ベクトルを正規化
    XMVECTOR dirNormalized = XMVector3Normalize(directionV);

    // Y軸（高さ）は無視し、XZ平面のみで考える (Z軸はY軸として計算)
    float dx = XMVectorGetX(dirNormalized);
    float dy = XMVectorGetZ(dirNormalized);

    // 距離パラメータ t (t=0がstart, t=distanceがend)
    float t = 0.0f;

    XMINT2 currentGrid = GetGridPosition(start, mapComp);

    // 衝突判定ループ (t が total distance を超えるまで)
    while (t < distance)
    {
        // 1. 次にレイが交差するグリッド境界までの距離を計算 (t_x, t_y)
        float t_x = std::numeric_limits<float>::max();
        float t_y = std::numeric_limits<float>::max();

        // グリッド線がワールド座標のどこにあるか
        float nextGridBoundaryX = 0.0f;
        float nextGridBoundaryY = 0.0f;

        // X軸方向の次のグリッド境界線を計算
        if (std::abs(dx) > 1e-6)
        {
            if (dx > 0) // 正方向 (右)
            {
                // 現在のセルの右側の境界線
                nextGridBoundaryX = ((float)currentGrid.x + 1.0f) * mapComp.tileSize - MAP_CENTER_OFFSET_X + X_ADJUSTMENT;
            }
            else // 負方向 (左)
            {
                // 現在のセルの左側の境界線
                nextGridBoundaryX = (float)currentGrid.x * mapComp.tileSize - MAP_CENTER_OFFSET_X + X_ADJUSTMENT;
            }

            // 始点から境界線までの距離 (t)
            t_x = (nextGridBoundaryX - start.x) / dx;
        }

        // Z軸（Yグリッド）方向の次のグリッド境界線を計算
        if (std::abs(dy) > 1e-6)
        {
            if (dy > 0) // 正方向 (奥)
            {
                // 現在のセルの奥側の境界線
                nextGridBoundaryY = ((float)currentGrid.y + 1.0f) * mapComp.tileSize - MAP_CENTER_OFFSET_Y + Z_ADJUSTMENT;
            }
            else // 負方向 (手前)
            {
                // 現在のセルの手前側の境界線
                nextGridBoundaryY = (float)currentGrid.y * mapComp.tileSize - MAP_CENTER_OFFSET_Y + Z_ADJUSTMENT;
            }

            // 始点から境界線までの距離 (t)
            t_y = (nextGridBoundaryY - start.z) / dy;
        }

        // 2. t_x と t_y のうち小さい方を選択 (次の交差点)
        float t_next = std::min(t_x, t_y);

        // ループの終点チェック (レイが終点を通り過ぎた場合)
        if (t_next >= distance)
        {
            // 終点まで壁がなかった
            hitPoint = end;
            return false;
        }

        // 3. 次のセルへ移動
        if (t_x < t_y) // X方向へ移動
        {
            if (dx > 0) currentGrid.x++;
            else currentGrid.x--;
        }
        else // Y(Z)方向へ移動 (t_x == t_y の場合は斜め移動)
        {
            if (dy > 0) currentGrid.y++;
            else currentGrid.y--;
        }

        // パラメータ t を更新
        t = t_next;

        // 4. 新しいセルが壁かどうかチェック
        if (currentGrid.x >= 0 && currentGrid.x < mapComp.gridSizeX &&
            currentGrid.y >= 0 && currentGrid.y < mapComp.gridSizeY)
        {
            // 移動先のセルが壁 (Wall) かどうか
            if (mapComp.grid[currentGrid.y][currentGrid.x].type == CellType::Wall)
            {
                // 衝突検出: 衝突した境界の手前 CAMERA_SAFETY_OFFSET 分の位置を計算
                float t_adjust = t - CAMERA_SAFETY_OFFSET;

                // カメラがプレイヤーに埋まらないように最小距離を確保
                float t_min = mapComp.tileSize * 0.5f; // プレイヤーのサイズを考慮した最小距離
                t_adjust = std::max(t_adjust, t_min);

                // 衝突点のワールド座標を計算
                XMVECTOR collisionPointV = startV + dirNormalized * t_adjust;

                XMStoreFloat3(&hitPoint, collisionPointV);

                return true; // 衝突検出
            }
        }
    }

    hitPoint = end; // 衝突なし
    return false;
}

/**
 * @brief カメラの位置を計算し、ビュー・プロジェクション行列を設定する
 */
void CameraControlSystem::Update(float deltaTime)
{
    (void)deltaTime;

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

    // 2. 右スティック入力の取得 (アクションモードでのみ有効)
    XMFLOAT2 rightStick = GetRightStick();
    XMFLOAT2 mouseDelta = GetMouseDelta();

    // カメラ回転量の計算 (スティック + マウス)
    // マウスのY移動は、画面上(マイナス)に行くとカメラを上に向けたい -> スティック上(プラス)と同じ挙動に合わせるため反転
    float yawInput = rightStick.x * CAMERA_SENSITIVITY_X + mouseDelta.x * MOUSE_SENSITIVITY;
    float pitchInput = rightStick.y * CAMERA_SENSITIVITY_Y - mouseDelta.y * MOUSE_SENSITIVITY;

    // 2. 【フリーカメラ】デバッグモードが有効な場合
    if (isDebugMode)
    {
        // カメラの現在の位置と注視点を直接操作するためのロジック
        // ※ 既存のm_currentCameraPosとm_currentLookAtを直接更新します。

        // 右スティック＋マウスでYAW/PITCHを更新
        m_currentYaw += yawInput * 2.0f; // デバッグ時は少し早めに
        m_currentPitch -= pitchInput * 2.0f;
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
                m_currentYaw += yawInput;
                m_currentPitch -= pitchInput;

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