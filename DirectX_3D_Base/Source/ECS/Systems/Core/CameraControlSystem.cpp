/*****************************************************************//**
 * @file	CameraControlSystem.cpp
 * @brief	CameraControlSystemの実装。カメラの追従とビュー行列の更新を行う。
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/ECS.h"
#include "Systems/Geometory.h"
#include "Systems/Input.h"
#include <algorithm>

using namespace DirectX;

float CameraControlSystem::m_mouseSensitivity = 0.005f;

// ===== 定数・マクロ定義 =====
const float CAMERA_SENSITIVITY_X = 0.04f;
const float CAMERA_SENSITIVITY_Y = 0.02f;
const float PITCH_MAX = XM_PIDIV2 - 0.2f;
const float PITCH_MIN = -XM_PIDIV4 + 0.5f; // 下を見すぎないように制限緩和
const float TOP_VIEW_HEIGHT = 100.0f;

// ★調整: 注視点の高さオフセット
const float FOCUS_HEIGHT_OFFSET = 2.0f;

// --- 演出用静的変数 ---
static float s_bobTimer = 0.0f;
static float s_idleSwayTimer = 0.0f;
static float s_currentBank = 0.0f;
static float s_targetFOV = 0.0f;

/**
 * @brief ワールド座標をグリッド座標 (XMINT2) に変換するヘルパー関数
 */
static XMINT2 GetGridPosition(const XMFLOAT3& worldPos, const MapComponent& mapComp)
{
    const float MAP_CENTER_OFFSET_X = (mapComp.gridSizeX / 2.0f) * mapComp.tileSize;
    const float MAP_CENTER_OFFSET_Y = (mapComp.gridSizeY / 2.0f) * mapComp.tileSize;
    const float X_ADJUSTMENT = 0.5f * mapComp.tileSize;
    const float Z_ADJUSTMENT = 1.0f * mapComp.tileSize;

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
 */
static bool RaycastToWall(
    const XMFLOAT3& start,
    const XMFLOAT3& end,
    const MapComponent& mapComp,
    XMFLOAT3& hitPoint
)
{
    const float CAMERA_SAFETY_OFFSET = mapComp.tileSize * 0.25f;
    const float MAP_CENTER_OFFSET_X = (mapComp.gridSizeX / 2.0f) * mapComp.tileSize;
    const float MAP_CENTER_OFFSET_Y = (mapComp.gridSizeY / 2.0f) * mapComp.tileSize;
    const float X_ADJUSTMENT = 0.5f * mapComp.tileSize;
    const float Z_ADJUSTMENT = 1.0f * mapComp.tileSize;

    XMVECTOR startV = XMLoadFloat3(&start);
    XMVECTOR endV = XMLoadFloat3(&end);
    XMVECTOR directionV = endV - startV;
    float distance = XMVectorGetX(XMVector3Length(directionV));

    if (distance < CAMERA_SAFETY_OFFSET) {
        hitPoint = end; return false;
    }

    XMVECTOR dirNormalized = XMVector3Normalize(directionV);
    float dx = XMVectorGetX(dirNormalized);
    float dy = XMVectorGetZ(dirNormalized);
    float t = 0.0f;

    XMINT2 currentGrid = GetGridPosition(start, mapComp);

    while (t < distance)
    {
        float t_x = std::numeric_limits<float>::max();
        float t_y = std::numeric_limits<float>::max();
        float nextGridBoundaryX = 0.0f;
        float nextGridBoundaryY = 0.0f;

        if (std::abs(dx) > 1e-6) {
            if (dx > 0) nextGridBoundaryX = ((float)currentGrid.x + 1.0f) * mapComp.tileSize - MAP_CENTER_OFFSET_X + X_ADJUSTMENT;
            else nextGridBoundaryX = (float)currentGrid.x * mapComp.tileSize - MAP_CENTER_OFFSET_X + X_ADJUSTMENT;
            t_x = (nextGridBoundaryX - start.x) / dx;
        }
        if (std::abs(dy) > 1e-6) {
            if (dy > 0) nextGridBoundaryY = ((float)currentGrid.y + 1.0f) * mapComp.tileSize - MAP_CENTER_OFFSET_Y + Z_ADJUSTMENT;
            else nextGridBoundaryY = (float)currentGrid.y * mapComp.tileSize - MAP_CENTER_OFFSET_Y + Z_ADJUSTMENT;
            t_y = (nextGridBoundaryY - start.z) / dy;
        }

        float t_next = std::min(t_x, t_y);
        if (t_next >= distance) {
            hitPoint = end; return false;
        }

        if (t_x < t_y) {
            if (dx > 0) currentGrid.x++; else currentGrid.x--;
        }
        else {
            if (dy > 0) currentGrid.y++; else currentGrid.y--;
        }
        t = t_next;

        if (currentGrid.x >= 0 && currentGrid.x < mapComp.gridSizeX &&
            currentGrid.y >= 0 && currentGrid.y < mapComp.gridSizeY)
        {
            if (mapComp.grid[currentGrid.y][currentGrid.x].type == CellType::Wall) {
                float t_adjust = t - CAMERA_SAFETY_OFFSET;
                float t_min = mapComp.tileSize * 0.5f;
                t_adjust = std::max(t_adjust, t_min);
                XMVECTOR collisionPointV = startV + dirNormalized * t_adjust;
                XMStoreFloat3(&hitPoint, collisionPointV);
                return true;
            }
        }
    }
    hitPoint = end;
    return false;
}

void CameraControlSystem::Update(float deltaTime)
{
    // ポーズチェック
    if (m_coordinator) {
        ECS::EntityID stateID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
        if (stateID != ECS::INVALID_ENTITY_ID) {
            if (m_coordinator->GetComponent<GameStateComponent>(stateID).isPaused) return;
        }
    }

    ECS::EntityID controllerID = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator);
    GameMode currentMode = GameMode::ACTION_MODE;
    bool isDebugMode = false;

    if (controllerID != ECS::INVALID_ENTITY_ID) {
        currentMode = m_coordinator->GetComponent<GameStateComponent>(controllerID).currentMode;
        if (m_coordinator->m_entityManager->GetSignature(controllerID).test(m_coordinator->GetComponentTypeID<DebugComponent>())) {
            isDebugMode = m_coordinator->GetComponent<DebugComponent>(controllerID).isDebugModeActive;
        }
    }
    bool isModeChanged = (currentMode != m_lastGameMode);
    m_lastGameMode = currentMode;

    ECS::EntityID cameraID = ECS::FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    if (cameraID == ECS::INVALID_ENTITY_ID) return;

    auto& cameraComp = m_coordinator->GetComponent<CameraComponent>(cameraID);
    auto& cameraTrans = m_coordinator->GetComponent<TransformComponent>(cameraID);

    // 固定カメラ
    if (m_isFixedMode) {
        XMVECTOR currentPos = XMLoadFloat3(&cameraTrans.position);
        XMVECTOR targetPos = XMLoadFloat3(&m_fixedPos);
        XMVECTOR newPos = XMVectorLerp(currentPos, targetPos, 5.0f * deltaTime);
        XMStoreFloat3(&cameraTrans.position, newPos);
        XMStoreFloat3(&m_currentCameraPos, newPos);

        XMVECTOR eye = newPos;
        XMVECTOR focus = XMLoadFloat3(&m_fixedLookAt);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
        XMStoreFloat3(&m_currentLookAt, focus);
        XMStoreFloat4x4(&cameraComp.viewMatrix, XMMatrixTranspose(view));

        XMMATRIX proj = XMMatrixPerspectiveFovLH(cameraComp.FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, cameraComp.nearClip, cameraComp.farClip);
        XMStoreFloat4x4(&cameraComp.projectionMatrix, XMMatrixTranspose(proj));
        cameraComp.worldPosition = m_currentCameraPos;
        return;
    }

    // 入力
    XMFLOAT2 rightStick = GetRightStick();
    XMFLOAT2 mouseDelta = GetMouseDelta();
    float yawInput = rightStick.x * (m_mouseSensitivity * 10.0f)+mouseDelta.x * m_mouseSensitivity;
    float pitchInput = rightStick.y * (m_mouseSensitivity * 5.0f) - mouseDelta.y * m_mouseSensitivity;

    // デバッグカメラ
    if (isDebugMode) {
        m_currentYaw += yawInput * 2.0f;
        m_currentPitch -= pitchInput * 2.0f;
        m_currentPitch = std::max(PITCH_MIN, std::min(PITCH_MAX, m_currentPitch));

        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_currentPitch, m_currentYaw, 0.0f);
        XMVECTOR forwardV = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
        XMVECTOR rightV = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix);
        XMVECTOR upV = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMVECTOR currentPosV = XMLoadFloat3(&m_currentCameraPos);
        float moveSpeed = 0.5f;
        if (IsKeyPress(VK_UP)) currentPosV += forwardV * moveSpeed;
        if (IsKeyPress(VK_DOWN)) currentPosV -= forwardV * moveSpeed;
        if (IsKeyPress(VK_LEFT)) currentPosV -= rightV * moveSpeed;
        if (IsKeyPress(VK_RIGHT)) currentPosV += rightV * moveSpeed;
        if (IsKeyPress(VK_SHIFT)) currentPosV += upV * moveSpeed;
        if (IsKeyPress(VK_CONTROL)) currentPosV -= upV * moveSpeed;

        XMStoreFloat3(&m_currentCameraPos, currentPosV);
        XMVECTOR focusPos = currentPosV + forwardV;
        XMMATRIX view = XMMatrixLookAtLH(currentPosV, focusPos, upV);
        XMStoreFloat4x4(&cameraComp.viewMatrix, XMMatrixTranspose(view));
        XMMATRIX proj = XMMatrixPerspectiveFovLH(
            cameraComp.FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, cameraComp.nearClip, cameraComp.farClip
        );
        XMStoreFloat4x4(&cameraComp.projectionMatrix, XMMatrixTranspose(proj));
        return;
    }

    // 通常カメラ
    for (auto const& entity : m_entities)
    {
        ECS::EntityID focusID = cameraComp.focusEntityID;
        if (focusID == ECS::INVALID_ENTITY_ID) continue;
        TransformComponent& focusTrans = m_coordinator->GetComponent<TransformComponent>(focusID);

        XMVECTOR targetCamPosV;
        XMVECTOR targetLookAtV;

        if (currentMode == GameMode::SCOUTING_MODE) {
            // TopView
            m_currentYaw = 0.0f; m_currentPitch = -XM_PIDIV2;
            const XMFLOAT3 VOID_POS = { 0, TOP_VIEW_HEIGHT, 0 };
            const XMFLOAT3 VOID_LOOK = { 0, 0, 0 };
            targetCamPosV = XMLoadFloat3(&VOID_POS);
            targetLookAtV = XMLoadFloat3(&VOID_LOOK);
            cameraComp.followSpeed = 1.0f;
            s_currentBank = 0.0f;
        }
        else {
            // ActionView
            m_currentYaw += yawInput;
            m_currentPitch -= pitchInput;
            m_currentPitch = std::max(PITCH_MIN, std::min(PITCH_MAX, m_currentPitch));

            XMVECTOR defaultOffset = XMLoadFloat3(&cameraComp.offset);
            float offsetLength = XMVectorGetX(XMVector3Length(defaultOffset));
            XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(m_currentPitch, m_currentYaw, 0.0f);
            XMVECTOR rotatedOffset = XMVector3Transform(XMVectorSet(0, 0, -offsetLength, 0), rotMat);
            XMVECTOR focusPos = XMLoadFloat3(&focusTrans.position);

            // ★視点オフセット (頭上を見る)
            XMVECTOR focusHeight = XMVectorSet(0.0f, FOCUS_HEIGHT_OFFSET, 0.0f, 0.0f);
            XMVECTOR focusCenter = focusPos + focusHeight;

            XMVECTOR rawTarget = focusCenter + rotatedOffset;
            targetLookAtV = focusCenter;

            // 壁判定 (RaycastToWall呼び出し)
            XMFLOAT3 rawTargetCamPosF; XMStoreFloat3(&rawTargetCamPosF, rawTarget);
            XMFLOAT3 adjustedCamPosF;
            ECS::EntityID mapEntity = ECS::FindFirstEntityWithComponent<MapComponent>(m_coordinator);

            if (mapEntity != ECS::INVALID_ENTITY_ID) {
                const MapComponent& mapComp = m_coordinator->GetComponent<MapComponent>(mapEntity);
                XMFLOAT3 focusPosF; XMStoreFloat3(&focusPosF, focusCenter); // Raycastの始点も高くする
                if (RaycastToWall(focusPosF, rawTargetCamPosF, mapComp, adjustedCamPosF)) {
                    targetCamPosV = XMLoadFloat3(&adjustedCamPosF);
                }
                else {
                    targetCamPosV = rawTarget;
                }
            }
            else {
                targetCamPosV = rawTarget;
            }
            cameraComp.followSpeed = 0.1f;
        }

        // 補間
        XMVECTOR currentCamPosV = XMLoadFloat3(&m_currentCameraPos);
        XMVECTOR currentLookAtV = XMLoadFloat3(&m_currentLookAt);
        float speed = isModeChanged ? 1.0f : cameraComp.followSpeed;
        XMVECTOR newCamPos = XMVectorLerp(currentCamPosV, targetCamPosV, speed);
        XMVECTOR newLookAt = XMVectorLerp(currentLookAtV, targetLookAtV, speed);
        XMStoreFloat3(&m_currentCameraPos, newCamPos);
        XMStoreFloat3(&m_currentLookAt, newLookAt);

        // ★★★ 演出適用 (JUICE: 調整版) ★★★
        XMVECTOR finalEye = newCamPos;
        XMVECTOR finalLook = newLookAt;
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);

        if (currentMode == GameMode::ACTION_MODE) {
            // 移動入力検知
            XMFLOAT2 stick = GetLeftStick();
            if (IsKeyPress('W')) stick.y += 1.0f;
            if (IsKeyPress('S')) stick.y -= 1.0f;
            if (IsKeyPress('D')) stick.x += 1.0f;
            if (IsKeyPress('A')) stick.x -= 1.0f;
            float mag = sqrt(stick.x * stick.x + stick.y * stick.y);
            if (mag > 1.0f) mag = 1.0f;

            // 1. ヘッドボビング (手ブレ)
            if (mag > 0.1f) {
                s_bobTimer += deltaTime * 14.0f;

                // 縦揺れ(bobY): 0.2f
                // 横揺れ(bobX): 0.05f
                float bobY = sinf(s_bobTimer) * 0.2f;
                float bobX = cosf(s_bobTimer * 0.5f) * 0.05f;

                finalEye += XMVectorSet(bobX, bobY, 0, 0);
                finalLook += XMVectorSet(bobX * 0.5f, bobY * 0.5f, 0, 0);
            }

            // 2. カメラバンク (傾き)
            float targetBank = -stick.x * 0.5f - yawInput * 5.0f;
            targetBank = std::max(-XMConvertToRadians(2.0f), std::min(XMConvertToRadians(2.0f), targetBank));

            s_currentBank += (targetBank - s_currentBank) * 5.0f * deltaTime;

            XMMATRIX bankRot = XMMatrixRotationZ(s_currentBank);
            up = XMVector3TransformNormal(up, bankRot);

            // 3. ダイナミックFOV (ここはそのまま)
            float baseFOV = 45.0f;
            float targetFOV = (mag > 0.5f) ? baseFOV + 3.0f : baseFOV;
            s_targetFOV += (targetFOV - s_targetFOV) * 5.0f * deltaTime;
        }
        else {
            up = XMVectorSet(0, 0, 1, 0);
            s_targetFOV = cameraComp.FOV;
        }

        // ★★★ 修正箇所: ここで行列を定義 ★★★
        XMMATRIX viewMat = XMMatrixLookAtLH(finalEye, finalLook, up);
        XMMATRIX projMat;

        if (currentMode == GameMode::SCOUTING_MODE) {
            float h = cameraComp.FOV * TOP_VIEW_HEIGHT;
            float w = h * ((float)SCREEN_WIDTH / SCREEN_HEIGHT);
            projMat = XMMatrixOrthographicLH(w, h, cameraComp.nearClip, cameraComp.farClip);
        }
        else {
            float fov = XMConvertToRadians(s_targetFOV > 0 ? s_targetFOV : 45.0f);
            projMat = XMMatrixPerspectiveFovLH(fov, (float)SCREEN_WIDTH / SCREEN_HEIGHT, cameraComp.nearClip, cameraComp.farClip);
        }

        XMFLOAT4X4 fView, fProj;
        XMStoreFloat4x4(&fView, XMMatrixTranspose(viewMat));
        XMStoreFloat4x4(&fProj, XMMatrixTranspose(projMat));
        cameraComp.viewMatrix = fView;
        cameraComp.projectionMatrix = fProj;
        cameraComp.worldPosition = m_currentCameraPos;
    }
}

void CameraControlSystem::SetFixedCamera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& lookAt)
{
    m_isFixedMode = true;
    m_fixedPos = position;
    m_fixedLookAt = lookAt;
    m_currentCameraPos = position;
    m_currentLookAt = lookAt;
}

void CameraControlSystem::ReleaseFixedCamera()
{
    m_isFixedMode = false;
}

void CameraControlSystem::ResetCameraAngle(float yaw, float pitch)
{
    m_currentYaw = yaw;
    m_currentPitch = pitch;
}