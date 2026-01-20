/* UIWallRenderSystem.cpp */

// =======================================================
// UIWallRenderSystem.cpp (最終決定版 - Geometory::DrawWallUIを呼び出す)
// =======================================================

// ===== 必須インクルード =====
#include "ECS/Systems/UI/UIWallRenderSystem.h" 
#include "ECS/ComponentRegistry.h"          
#include "ECS/Coordinator.h"                
#include "DirectXMath.h"                    

// 描画対象のコンポーネント定義
#include "ECS/Components/UI/UIWallComponent.h"
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/Core/CameraComponent.h"    

// 描画処理に必要なGeometoryクラス
#include "Systems/Geometory.h" 

// 標準ライブラリ
#include <vector>
#include <algorithm>
#include <iostream>

using namespace DirectX;
using namespace ECS;

// =======================================================
// 1. ソート用データ構造とコンポーネントの自動登録
// =======================================================

// ★★★ UIWallRenderData の定義 (Draw関数より前) ★★★
struct UIWallRenderData
{
    EntityID entityID;
    float depth;

    // Zソート: depthが小さい（カメラに近い）ものから先に描画する
    bool operator<(const UIWallRenderData& other) const
    {
        return depth < other.depth;
    }
};

REGISTER_COMPONENT_TYPE(UIWallComponent)


// =======================================================
// 2. システムの実装
// =======================================================

void UIWallRenderSystem::Draw(float deltaTime)
{
    if (m_coordinator == nullptr || m_entities.empty())
    {
        return;
    }

    // A. カメラ情報の取得とView/Projection行列の計算
    EntityID cameraEntity = FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    if (cameraEntity == INVALID_ENTITY_ID) { return; }

    CameraComponent& camera = m_coordinator->GetComponent<CameraComponent>(cameraEntity);
    XMMATRIX viewMatrix = XMLoadFloat4x4(&camera.viewMatrix);
    XMMATRIX projMatrix = XMLoadFloat4x4(&camera.projectionMatrix);

    // 【描画準備フェーズ】: 共通行列の設定 (Geometoryの既存機能を使用)
    XMFLOAT4X4 viewFloat, projFloat;
    XMStoreFloat4x4(&viewFloat, viewMatrix);
    XMStoreFloat4x4(&projFloat, projMatrix);
    Geometory::SetView(viewFloat);
    Geometory::SetProjection(projFloat);

    // ※ Geometory::SetWallUIShaderState() のような、描画ステート設定関数が必要になる場合があります。


    // B. ソートリストの作成とソート (Zソート)
    std::vector<UIWallRenderData> renderList;
    renderList.reserve(m_entities.size());
    for (const auto& entity : m_entities)
    {
        const auto& transformComp = m_coordinator->GetComponent<TransformComponent>(entity);

        // E0304エラーの修正: UIWallRenderDataを明示的に構築
        renderList.push_back(UIWallRenderData{ entity, transformComp.position.z });
    }
    std::sort(renderList.begin(), renderList.end());


    // C. ソートされた順に描画処理を実行
    for (const auto& data : renderList)
    {
        EntityID entity = data.entityID;
        TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
        UIWallComponent& uiWall = m_coordinator->GetComponent<UIWallComponent>(entity);

        // 1. World行列の計算
        XMMATRIX worldMatrix = XMMatrixScaling(transform.scale.x, transform.scale.y, transform.scale.z);
        worldMatrix *= XMMatrixRotationRollPitchYaw(
            transform.rotation.x, transform.rotation.y, transform.rotation.z
        );
        worldMatrix *= XMMatrixTranslation(transform.position.x, transform.position.y, transform.position.z);

        // ----------------------------------------------------
        // 2. Geometory ヘルパー関数を呼び出し
        // ----------------------------------------------------

        // a. World行列をGeometoryに設定
        XMFLOAT4X4 worldFloat;
        XMStoreFloat4x4(&worldFloat, worldMatrix);
        Geometory::SetWorld(worldFloat);

        Geometory::DrawBox(); // ← これにより LNK2019 エラーが解消されます
        {}
    }

    // 【描画後処理フェーズ】
    // Geometory::RestoreStates(); // 必要であれば後処理関数を呼び出す
}