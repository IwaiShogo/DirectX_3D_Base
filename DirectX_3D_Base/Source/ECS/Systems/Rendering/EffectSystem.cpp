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

	// Renderer / Manager
	m_renderer = EffekseerRendererDX11::Renderer::Create(GetDevice(), GetContext(), 8000);
	m_manager = Effekseer::Manager::Create(8000);

	m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
	m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
	m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
	m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
	m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

	m_manager->SetTextureLoader(m_renderer->CreateTextureLoader());
	m_manager->SetModelLoader(m_renderer->CreateModelLoader());
	m_manager->SetMaterialLoader(m_renderer->CreateMaterialLoader());

	Asset::AssetManager::GetInstance().SetEffekseerManager(m_manager);
}

void EffectSystem::Uninit()
{
	Asset::AssetManager::GetInstance().UnloadEffects();
	Asset::AssetManager::GetInstance().SetEffekseerManager(nullptr);

	m_manager.Reset();
	m_renderer.Reset();

	m_hasOverride = false;
}

void EffectSystem::SetScreenSpaceCamera(float screenW, float screenH)
{
	using namespace DirectX;

	XMMATRIX view = XMMatrixIdentity();

	// 左上(0,0)〜右下(screenW, screenH)、Y下向き
	// ★Z範囲を広げる（UIでもエフェクト側がZを使う場合があるため）
	XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
		0.0f, screenW,
		screenH, 0.0f,
		0.0f, 1000.0f
	);

	XMStoreFloat4x4(&m_overrideView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_overrideProj, XMMatrixTranspose(proj));

	m_hasOverride = true;
}

void EffectSystem::ClearOverrideCamera()
{
	m_hasOverride = false;
}

void EffectSystem::Update(float deltaTime)
{
	// -----------------------------
	// 1) カメラ設定（View/Proj）
	// -----------------------------
	XMFLOAT4X4 viewMat{}, projMat{};
	XMFLOAT3 camPos = { 0,0,0 };
	bool camFound = false;

	// 通常のカメラを探す
	ECS::EntityID camID = ECS::FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
	if (camID != ECS::INVALID_ENTITY_ID)
	{
		auto& cam = m_coordinator->GetComponent<CameraComponent>(camID);
		viewMat = cam.viewMatrix;
		projMat = cam.projectionMatrix;
		camPos = cam.worldPosition;
		camFound = true;
	}
	else
	{
		camID = ECS::FindFirstEntityWithComponent<BasicCameraComponent>(m_coordinator);
		if (camID != ECS::INVALID_ENTITY_ID)
		{
			auto& cam = m_coordinator->GetComponent<BasicCameraComponent>(camID);
			auto& trans = m_coordinator->GetComponent<TransformComponent>(camID);
			viewMat = cam.viewMatrix;
			projMat = cam.projectionMatrix;
			camPos = trans.position;
			camFound = true;
		}
	}

	// ★UI用上書きがあれば優先
	if (m_hasOverride)
	{
		viewMat = m_overrideView;
		projMat = m_overrideProj;
		camPos = { 0,0,0 };
		camFound = true;
	}

	if (camFound)
	{
		// 格納は転置想定なので元に戻す
		XMMATRIX v = XMMatrixTranspose(XMLoadFloat4x4(&viewMat));
		XMMATRIX p = XMMatrixTranspose(XMLoadFloat4x4(&projMat));

		Effekseer::Matrix44 effView, effProj;
		XMStoreFloat4x4((XMFLOAT4X4*)&effView, v);
		XMStoreFloat4x4((XMFLOAT4X4*)&effProj, p);

		m_renderer->SetCameraMatrix(effView);
		m_renderer->SetProjectionMatrix(effProj);
	}

	// -----------------------------
	// 2) エフェクトコンポーネント処理
	// -----------------------------
	for (auto const& entity : m_entities)
	{
		auto& effectComp = m_coordinator->GetComponent<EffectComponent>(entity);
		auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);

		// ループ時の距離制御（UI用途は基本OFF想定）
		// ※スクリーン座標カメラ上書き中(m_hasOverride)は距離判定がUIピクセル距離になって破綻するのでOFF
		if (effectComp.isLooping && !m_hasOverride)
		{
			float dx = transform.position.x - camPos.x;
			float dy = transform.position.y - camPos.y;
			float dz = transform.position.z - camPos.z;
			float distSq = dx * dx + dy * dy + dz * dz;

			const float showDist = 20.0f;
			const float hideDist = 23.0f;

			if (effectComp.handle == -1)
			{
				if (distSq < showDist * showDist) effectComp.requestPlay = true;
			}
			else
			{
				if (distSq > hideDist * hideDist) effectComp.requestStop = true;
			}
		}

		// 再生
		if (effectComp.requestPlay)
		{
			effectComp.requestPlay = false;

			Effekseer::EffectRef effectRef = Asset::AssetManager::GetInstance().LoadEffect(effectComp.assetID);

			if (effectRef != nullptr)
			{
				Effekseer::Handle handle = m_manager->Play(
					effectRef,
					transform.position.x + effectComp.offset.x,
					transform.position.y + effectComp.offset.y,
					transform.position.z + effectComp.offset.z
				);

				effectComp.handle = handle;

				m_manager->SetScale(handle,
					transform.scale.x * effectComp.scale,
					transform.scale.y * effectComp.scale,
					transform.scale.z * effectComp.scale
				);
				m_manager->SetRotation(handle, transform.rotation.x, transform.rotation.y, transform.rotation.z);
			}
			else
			{
				std::cerr << "[EffectSystem] Failed to load effect asset: " << effectComp.assetID << std::endl;
			}
		}

		// 停止
		if (effectComp.requestStop)
		{
			effectComp.requestStop = false;
			if (m_manager->Exists(effectComp.handle))
			{
				m_manager->StopEffect(effectComp.handle);
			}
			effectComp.handle = -1;
		}

		// 位置同期
		if (m_manager->Exists(effectComp.handle))
		{
			m_manager->SetLocation(
				effectComp.handle,
				transform.position.x + effectComp.offset.x,
				transform.position.y + effectComp.offset.y,
				transform.position.z + effectComp.offset.z
			);

			m_manager->SetRotation(effectComp.handle, transform.rotation.x, transform.rotation.y, transform.rotation.z);

			m_manager->SetScale(effectComp.handle,
				transform.scale.x * effectComp.scale,
				transform.scale.y * effectComp.scale,
				transform.scale.z * effectComp.scale
			);
		}
		else
		{
			// 終了したら-1へ
			if (effectComp.isLooping && effectComp.handle != -1)
			{
				effectComp.requestPlay = true;
			}
			else
			{
				effectComp.handle = -1;
			}
		}
	}

	// Effekseer更新（60fps基準）
	m_manager->Update(deltaTime * 60.0f);
}

void EffectSystem::Render()
{
	m_renderer->BeginRendering();
	m_manager->Draw();
	m_renderer->EndRendering();
}
