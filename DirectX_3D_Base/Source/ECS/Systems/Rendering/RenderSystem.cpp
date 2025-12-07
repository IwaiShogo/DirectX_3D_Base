/*****************************************************************//**
 * @file	RenderSystem.cpp
 * @brief	TransformComponentとRenderComponentを持つEntityを描画するSystemの実装。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：RenderSystemの描画ロジックを実装。Main.cppからデモ描画とカメラ設定を移管。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "Main.h"
#include "Systems/DirectX/DirectX.h"
#include "Systems/Geometory.h"
#include "Systems/Input.h"
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"
#include "Systems/DirectX/ShaderList.h"
#include <iostream>

using namespace DirectX;

/**
 * @brief カメラ設定とデバッグ描画を行う
 */
void RenderSystem::DrawSetup()
{
#ifdef _DEBUG
	// 軸線の表示
	// グリッド
	XMFLOAT4 lineColor(0.5f, 0.5f, 0.5f, 1.0f);
	float size = DEBUG_GRID_NUM * DEBUG_GRID_MARGIN;
	for (int i = 1; i <= DEBUG_GRID_NUM; ++i)
	{
		float grid = i * DEBUG_GRID_MARGIN;
		XMFLOAT3 pos[2] = {
			XMFLOAT3(grid, 0.0f, size),
			XMFLOAT3(grid, 0.0f,-size),
		};
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].x = pos[1].x = -grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].x = size;
		pos[1].x = -size;
		pos[0].z = pos[1].z = grid;
		pos[1].z = grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].z = pos[1].z = -grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
	}
	// 軸
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(size, 0, 0), XMFLOAT4(1, 0, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, size, 0), XMFLOAT4(0, 1, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, size), XMFLOAT4(0, 0, 1, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(-size, 0, 0), XMFLOAT4(0, 0, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, -size), XMFLOAT4(0, 0, 0, 1));

#endif
	auto mapGenSystem = ECS::ECSInitializer::GetSystem<MapGenerationSystem>();
	if (mapGenSystem)
	{
		mapGenSystem->DrawDebugLines();
	}

	Geometory::DrawLines();
}

/**
 * @brief TransformComponentとRenderComponentを持つ全てのEntityを描画する
 */
void RenderSystem::DrawEntities()
{
	// 1. CameraComponentを持つEntityを検索し、カメラ座標と行列を取得
	// 1. まず通常のCameraComponentを探す
	DirectX::XMFLOAT4X4 viewMat;
	DirectX::XMFLOAT4X4 projMat;
	bool cameraFound = false;

	ECS::EntityID cameraID = ECS::FindFirstEntityWithComponent<CameraComponent>(m_coordinator);
	if (cameraID != ECS::INVALID_ENTITY_ID)
	{
		auto& cam = m_coordinator->GetComponent<CameraComponent>(cameraID);
		viewMat = cam.viewMatrix;
		projMat = cam.projectionMatrix;
		cameraFound = true;
	}
	else
	{
		// 2. なければBasicCameraComponentを探す
		cameraID = ECS::FindFirstEntityWithComponent<BasicCameraComponent>(m_coordinator);
		if (cameraID != ECS::INVALID_ENTITY_ID)
		{
			auto& cam = m_coordinator->GetComponent<BasicCameraComponent>(cameraID);
			viewMat = cam.viewMatrix;
			projMat = cam.projectionMatrix;
			cameraFound = true;
		}
	}

	if (!cameraFound) return;

	{
		// View行列をロード
		XMMATRIX matView = XMLoadFloat4x4(&viewMat);

		// View行列の逆行列を計算（= カメラのワールド行列）
		XMMATRIX matInvView = XMMatrixInverse(nullptr, matView);

		// 逆行列のZ軸（3行目）がカメラの前方ベクトル（視線方向）
		XMFLOAT3 cameraForward;
		XMStoreFloat3(&cameraForward, matInvView.r[2]);

		// 【重要】ベクトルの正規化（長さを1.0にする）
		// これをしないと、計算誤差で光の強さが不安定になることがあります
		XMVECTOR vLight = XMLoadFloat3(&cameraForward);
		vLight = XMVector3Normalize(vLight);
		XMStoreFloat3(&cameraForward, vLight);

		// ライトを設定 (色は白)
		// これにより、カメラがどこを向いても、画面中央の物体は常に正面から照らされます
		ShaderList::SetLight(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), cameraForward);
	}

	RenderTarget* pRTV = GetDefaultRTV();   // デフォルトのRenderTargetViewを取得
	DepthStencil* pDSV = GetDefaultDSV();	// デフォルトのDepthStencilViewを取得
	SetRenderTargets(1, &pRTV, pDSV);		// 第3引数がnullの場合、2D表示となる

	SetDepthTest(true); // デプス・テストが有効化されていることを確認

	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// Componentを高速に取得
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		DirectX::XMFLOAT4X4 wvp[3];
		DirectX::XMMATRIX world;

		// 1. ワールド行列の計算 (TransformComponent -> XMMATRIX -> XMFLOAT4X4)
		XMMATRIX S = XMMatrixScaling(transform.scale.x, transform.scale.y, transform.scale.z);
		XMMATRIX Rx = XMMatrixRotationX(transform.rotation.x);
		XMMATRIX Ry = XMMatrixRotationY(transform.rotation.y);
		XMMATRIX Rz = XMMatrixRotationZ(transform.rotation.z);
		XMMATRIX R = Rz * Rx * Ry;
		XMMATRIX T = XMMatrixTranslation(transform.position.x, transform.position.y, transform.position.z);
		world = S * R * T;

		// ビュー行列

		XMStoreFloat4x4(&wvp[0], XMMatrixTranspose(world));
		wvp[1] = viewMat;
		wvp[2] = projMat;

		// シェーダーへの変換行列を設定
		Geometory::SetWorld(wvp[0]);
		Geometory::SetView(wvp[1]);
		Geometory::SetProjection(wvp[2]);

		ShaderList::SetWVP(wvp);
		// 2. 描画処理 (RenderComponent)
		
		Geometory::SetColor(render.color);

		// 形状に応じて描画
		switch (render.type)
		{
		case MESH_BOX:
			Geometory::DrawBox();
			break;
		case MESH_SPHERE:
			Geometory::DrawBox(); // 代替としてBoxを描画
			break;
		case MESH_MODEL:
		{
			// ModelComponentを取得し、Model::Draw()を呼び出す
			// RenderSystemのシグネチャにModelComponentが含まれている前提
			ModelComponent& model = m_coordinator->GetComponent<ModelComponent>(entity);
			if (model.pModel)
			{
				bool isAnimated = false;
				if (m_coordinator->HasComponent<AnimationComponent>(entity))
				{
					// アニメーション用のシェーダーを設定 (VS_SKIN がある前提)
					// ShaderListに VS_SKIN が定義されていない場合は、追加するか既存のボーン対応シェーダーを指定してください
					model.pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_ANIME));
					isAnimated = true;
				}
				else
				{
					// 通常のシェーダー
					model.pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
				}

				model.pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

				// 描画処理
				for (uint32_t i = 0; i < model.pModel->GetMeshNum(); ++i) {
					// モデルのメッシュを取得
					Model::Mesh mesh = *model.pModel->GetMesh(i);
					// メッシュに割り当てられているマテリアルを取得
					Model::Material	material = *model.pModel->GetMaterial(mesh.materialID);
					// シェーダーへマテリアルを設定
					ShaderList::SetMaterial(material);
					// モデルの描画
					model.pModel->Draw(i);
				}

			}
		}
			break;
		case MESH_NONE:
			// 何もしない
			break;
		}
	}
}