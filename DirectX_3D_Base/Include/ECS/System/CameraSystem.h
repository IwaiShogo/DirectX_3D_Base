/*****************************************************************//**
 * @file	CameraSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___CAMERASYSTEM_H___
#define ___CAMERASYSTEM_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include "ECS/Coordinator.h"
// Componentの依存
#include "ECS/Component/Transform.h"
#include "ECS/Component/Camera.h"
// 外部システムの依存
#include "Systems/Camera.h" // Cameraヘルパークラス

#include <DirectXMath.h>

// Coordinatorのグローバル参照
extern Coordinator* g_Coordinator;

/**
 * @class CameraSystem
 * @brief TransformとCameraSetting Componentを基に、カメラのView/Projection行列を計算するシステム
 * @note このシステムは、Entityの空間情報とカメラ設定を分離し、データ駆動でカメラを更新します。
 */
class CameraSystem : public System
{
private:
    // カメラヘルパーのインスタンス (行列計算に利用)
    // 描画処理で最新の結果が必要なため、Systemのメンバーとして保持します
    Camera cameraHelper_;

public:
    // 計算されたViewProj行列をRenderSystemに提供するためのGetter
    DirectX::XMMATRIX GetViewProjectionMatrix() const
    {
        return cameraHelper_.GetViewProjectionMatrix();
    }

    void Update(float deltaTime) override
    {
        if (entities->empty()) return;

        // --------------------------------------------------
        // 1. 全てのカメラEntityに対してループ
        // --------------------------------------------------
        for (const Entity entity : *entities)
        {
            // Coordinatorを通じてComponentデータを取得
            Transform& t = g_Coordinator->GetComponent<Transform>(entity);
            CameraSetting& cs = g_Coordinator->GetComponent<CameraSetting>(entity);

            // --------------------------------------------------
            // 2. View行列の計算
            // --------------------------------------------------

            // a. 視点 (Eye) の計算: Transform.positionをそのまま利用
            DirectX::XMFLOAT3 eye = t.position;

            // b. 注視点 (Target) の計算: 
            // 簡易的に、Transformの回転に基づいた前方ベクトルを計算し、LookAtOffsetを適用します。
            DirectX::XMVECTOR forward = DirectX::XMVector3Transform(
                DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // ローカル前方ベクトル
                DirectX::XMMatrixRotationRollPitchYaw( // Transformの回転を適用
                    DirectX::XMConvertToRadians(t.rotation.x),
                    DirectX::XMConvertToRadians(t.rotation.y),
                    DirectX::XMConvertToRadians(t.rotation.z)
                )
            );

            DirectX::XMVECTOR targetVec = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&eye), forward);
            DirectX::XMFLOAT3 target;
            DirectX::XMStoreFloat3(&target, targetVec);

            // c. View行列の更新
            cameraHelper_.UpdateView(eye, target, cs.upVector);

            // --------------------------------------------------
            // 3. Projection行列の計算 (画面サイズ変更時に一度行うだけで十分)
            // --------------------------------------------------
            cameraHelper_.UpdateProjection(
                cs.fieldOfViewY,
                cs.aspectRatio,
                cs.nearClip,
                cs.farClip
            );

            // --------------------------------------------------
            // 4. 結果をComponentに書き戻す
            // --------------------------------------------------
            DirectX::XMStoreFloat4x4(&cs.viewMatrix, cameraHelper_.GetViewMatrix());
            DirectX::XMStoreFloat4x4(&cs.projectionMatrix, cameraHelper_.GetProjectionMatrix());
            DirectX::XMStoreFloat4x4(&cs.viewProjectionMatrix, cameraHelper_.GetViewProjectionMatrix());
        }
    }
};

#endif // !___CAMERASYSTEM_H___