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

	// カメラの値
	//static bool camAutoSwitch = false;
	//static bool camUpDownSwitch = true;
	//static float camAutoRotate = 1.0f;
	//if (IsKeyTrigger(VK_RETURN)) {
	//	camAutoSwitch ^= true;
	//}
	//if (IsKeyTrigger(VK_SPACE)) {
	//	camUpDownSwitch ^= true;
	//}

	//XMVECTOR camPos;
	//if (camAutoSwitch) {
	//	camAutoRotate += 0.01f;
	//}
	//camPos = XMVectorSet(
	//	cosf(camAutoRotate) * 5.0f,
	//	3.5f * (camUpDownSwitch ? 1.0f : -1.0f),
	//	sinf(camAutoRotate) * 5.0f,
	//	0.0f);

	//// ジオメトリ用カメラ初期化
	//XMFLOAT4X4 mat[2];
	//XMStoreFloat4x4(&mat[0], XMMatrixTranspose(
	//	XMMatrixLookAtLH(
	//		camPos,
	//		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
	//		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	//	)));
	//XMStoreFloat4x4(&mat[1], XMMatrixTranspose(
	//	XMMatrixPerspectiveFovLH(
	//		XMConvertToRadians(60.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 1000.0f)
	//));
	//Geometory::SetView(mat[0]);
	//Geometory::SetProjection(mat[1]);
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
	ECS::EntityID cameraID = ECS::FindFirstEntityWithComponent<CameraComponent>(m_coordinator);

    if (cameraID == ECS::INVALID_ENTITY_ID)  
    {  
		return;
    }
	auto& cameraComp = m_coordinator->GetComponent<CameraComponent>(cameraID);

	RenderTarget* pRTV = GetDefaultRTV();    // デフォルトのRenderTargetViewを取得
	DepthStencil* pDSV = GetDefaultDSV();    // デフォルトのDepthStencilViewを取得
	SetRenderTargets(1, &pRTV, pDSV);    // 第3引数がnullの場合、2D表示となる

	SetDepthTest(true); // 【確認・維持】デプス・テストが有効化されていることを確認

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
		wvp[1] = cameraComp.viewMatrix;
		wvp[2] = cameraComp.projectionMatrix;

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

				// 描画実行
				for (uint32_t i = 0; i < model.pModel->GetMeshNum(); ++i) {
					// モデルのメッシュを取得
					Model::Mesh mesh = *model.pModel->GetMesh(i);
					// メッシュに割り当てられているマテリアルを取得
					Model::Material	material = *model.pModel->GetMaterial(mesh.materialID);
					// シェーダーへマテリアルを設定
					ShaderList::SetMaterial(material);
					// モデルの描画
					model.pModel->Draw(i);
					model.pModel->DrawBone();
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