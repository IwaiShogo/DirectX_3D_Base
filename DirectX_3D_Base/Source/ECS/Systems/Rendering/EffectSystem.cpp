/*****************************************************************//**
 * @file	EffectSystem.cpp
 * @brief	エフェクト
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/07	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "ECS/Systems/Rendering/EffectSystem.h"
#include "Systems/AssetManager.h"
#include "Systems/DirectX/DirectX.h" 
#include "Main.h"

using namespace DirectX;

void EffectSystem::Init(ECS::Coordinator* coordinator)
{
    m_coordinator = coordinator;

    if (m_manager != nullptr)
    {
        Uninit();
    }

    // 1. レンダラー作成 (最大スプライト数: 8000)
    m_renderer = EffekseerRendererDX11::Renderer::Create(
        GetDevice(), GetContext(), 8000
    );

    // 2. マネージャー作成
    m_manager = Effekseer::Manager::Create(8000);

    // 3. 描画モジュールの設定
    m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
    m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
    m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
    m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
    m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

    // 4. AssetManagerにマネージャーを登録
    Asset::AssetManager::GetInstance().SetEffekseerManager(m_manager);
}

void EffectSystem::Uninit()
{
    Asset::AssetManager::GetInstance().UnloadEffects();

    // AssetManagerの参照を外す
    Asset::AssetManager::GetInstance().SetEffekseerManager(nullptr);
    m_manager.Reset();
    m_renderer.Reset();
}

void EffectSystem::Update(float deltaTime)
{
    // ---------------------------------------------------
    // 1. カメラ情報の更新 (EffekseerにView/Proj行列を渡す)
    // ---------------------------------------------------
    XMFLOAT4X4 viewMat, projMat;
    XMFLOAT3 camPos = { 0,0,0 };
    bool camFound = false;

    // メインカメラを探す (RenderSystemと同様の優先順位)
    ECS::EntityID camID = ECS::FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
    if (camID != ECS::INVALID_ENTITY_ID) {
        auto& cam = m_coordinator->GetComponent<CameraComponent>(camID);
        viewMat = cam.viewMatrix;
        projMat = cam.projectionMatrix;
        camPos = cam.worldPosition;
        camFound = true;
    }
    else {
        camID = ECS::FindFirstEntityWithComponent<BasicCameraComponent>(m_coordinator);
        if (camID != ECS::INVALID_ENTITY_ID) {
            auto& cam = m_coordinator->GetComponent<BasicCameraComponent>(camID);
            // BasicCameraにはworldPositionメンバがない場合があるため、Transformから取得
            auto& trans = m_coordinator->GetComponent<TransformComponent>(camID);
            viewMat = cam.viewMatrix;
            projMat = cam.projectionMatrix;
            camPos = trans.position;
            camFound = true;
        }
    }

    if (camFound) {
        // RenderSystemで転置して保存されている場合、元に戻す
        XMMATRIX v = XMMatrixTranspose(XMLoadFloat4x4(&viewMat));
        XMMATRIX p = XMMatrixTranspose(XMLoadFloat4x4(&projMat));

        Effekseer::Matrix44 effView, effProj;
        XMStoreFloat4x4((XMFLOAT4X4*)&effView, v);
        XMStoreFloat4x4((XMFLOAT4X4*)&effProj, p);

        m_renderer->SetCameraMatrix(effView);
        m_renderer->SetProjectionMatrix(effProj);
    }

    // ---------------------------------------------------
    // 2. コンポーネント更新 (再生・位置同期)
    // ---------------------------------------------------
    for (auto const& entity : m_entities)
    {
        auto& effectComp = m_coordinator->GetComponent<EffectComponent>(entity);
        auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

        // A. 再生リクエスト処理
        if (effectComp.requestPlay)
        {
            effectComp.requestPlay = false;

            // AssetManager経由でロード
            Effekseer::EffectRef effectRef = Asset::AssetManager::GetInstance().LoadEffect(effectComp.assetID);

            if (effectRef != nullptr)
            {
                // Playにはそのまま渡す
                Effekseer::Handle handle = m_manager->Play(
                    effectRef,
                    transform.position.x + effectComp.offset.x,
                    transform.position.y + effectComp.offset.y,
                    transform.position.z + effectComp.offset.z
                );
                effectComp.handle = handle;

                // 初期パラメータ設定
                m_manager->SetScale(handle, transform.scale.x * effectComp.scale, transform.scale.y * effectComp.scale, transform.scale.z * effectComp.scale);
                m_manager->SetRotation(handle, transform.rotation.x, transform.rotation.y, transform.rotation.z);
            }
            else
            {
                // ロード失敗時
                std::cerr << "[EffectSystem] Failed to load effect asset: " << effectComp.assetID << std::endl;
            }
        }

        // B. 停止リクエスト
        if (effectComp.requestStop)
        {
            effectComp.requestStop = false;
            if (m_manager->Exists(effectComp.handle)) {
                m_manager->StopEffect(effectComp.handle);
            }
            effectComp.handle = 0;
        }

        // C. 位置同期 (再生中の場合)
        if (m_manager->Exists(effectComp.handle))
        {
            m_manager->SetLocation(
                effectComp.handle,
                transform.position.x + effectComp.offset.x,
                transform.position.y + effectComp.offset.y,
                transform.position.z + effectComp.offset.z
            );
            m_manager->SetRotation(effectComp.handle, transform.rotation.x, transform.rotation.y, transform.rotation.z);
            m_manager->SetScale(effectComp.handle, transform.scale.x * effectComp.scale, transform.scale.y * effectComp.scale, transform.scale.z * effectComp.scale);
        }
        else
        {
            // エフェクト終了（かつループ設定なら再再生）
            if (effectComp.isLooping && effectComp.handle != 0) {
                effectComp.requestPlay = true;
            }
            else {
                effectComp.handle = 0;
            }
        }
    }

    // Effekseer自体の更新 (60fps基準)
    m_manager->Update(deltaTime * 60.0f);
}

void EffectSystem::Render()
{
    m_renderer->BeginRendering();
    m_manager->Draw();
    m_renderer->EndRendering();
}