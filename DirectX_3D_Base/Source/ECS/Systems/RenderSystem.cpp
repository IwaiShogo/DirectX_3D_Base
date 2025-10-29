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

#include "ECS/Systems/RenderSystem.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "Main.h"
#include "Systems/DirectX/DirectX.h"
#include "Systems/Geometory.h"
#include "Systems/Input.h"
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"
#include "Systems/DirectX/ShaderList.h"
#include <iostream>

using namespace DirectX;

void RenderSystem::Init()
{
	m_coordinator = GameScene::GetCoordinator();

}

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

	Geometory::DrawLines();

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
}

/**
 * @brief TransformComponentとRenderComponentを持つ全てのEntityを描画する
 */
void RenderSystem::DrawEntities()
{
	// 1. CameraComponentを持つEntityを検索し、カメラ座標と行列を取得
	ECS::EntityID cameraID = ECS::INVALID_ENTITY_ID;

	// Coordinatorの全Entityを走査 (非効率だが確実)
	for (auto const& entity : m_entities)
	{
		if (m_coordinator->m_entityManager->GetSignature(entity).test(m_coordinator->GetComponentTypeID<CameraComponent>()))
		{
			cameraID = entity;
			break;
		}
	}

    CameraComponent* cameraComp = nullptr;  
    if (cameraID != ECS::INVALID_ENTITY_ID)  
    {  
        cameraComp = &m_coordinator->GetComponent<CameraComponent>(cameraID);  

        // ★★★ 2. Geometoryに行列とカメラ位置を設定 ★★★  
        Geometory::SetView(cameraComp->ViewMatrix);  
        Geometory::SetProjection(cameraComp->ProjectionMatrix);  
    }
	if (cameraID != ECS::INVALID_ENTITY_ID)
	{
		*cameraComp = m_coordinator->GetComponent<CameraComponent>(cameraID);

		// ★★★ 2. Geometoryに行列とカメラ位置を設定 ★★★
		Geometory::SetView(cameraComp->ViewMatrix);
		Geometory::SetProjection(cameraComp->ProjectionMatrix);
	}

	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// Componentを高速に取得
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		DirectX::XMFLOAT4X4 wvp[3];
		DirectX::XMMATRIX world, view, proj;

		// 1. ワールド行列の計算 (TransformComponent -> XMMATRIX -> XMFLOAT4X4)
		// ... (スケール、回転、平行移動の計算ロジックは維持) ...
		XMMATRIX S = XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z);
		XMMATRIX Rx = XMMatrixRotationX(transform.Rotation.x);
		XMMATRIX Ry = XMMatrixRotationY(transform.Rotation.y);
		XMMATRIX Rz = XMMatrixRotationZ(transform.Rotation.z);
		XMMATRIX R = Rz * Rx * Ry;
		XMMATRIX T = XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);
		world = S * R * T;

		// ビュー行列

		XMStoreFloat4x4(&wvp[0], XMMatrixTranspose(world));
		wvp[1] = cameraComp->ViewMatrix;
		wvp[2] = cameraComp->ProjectionMatrix;

		// シェーダーへの変換行列を設定
		Geometory::SetWorld(wvp[0]);
		Geometory::SetView(wvp[1]);
		Geometory::SetProjection(wvp[2]);

		ShaderList::SetWVP(wvp);
		// 2. 描画処理 (RenderComponent)

		// 形状に応じて描画
		switch (render.Type)
		{
		case MESH_BOX:
			Geometory::DrawBox();
			break;
		case MESH_SPHERE:
			Geometory::DrawBox(); // 代替としてBoxを描画
			break;
		case MESH_MODEL:
			// ModelSystemに移行後、実装
		{
			// ModelComponentを取得し、Model::Draw()を呼び出す
			// RenderSystemのシグネチャにModelComponentが含まれている前提
			ModelComponent& model = m_coordinator->GetComponent<ModelComponent>(entity);
			if (model.pModel)
			{
				model.pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
				model.pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

				for (int i = 0; i < model.pModel->GetMeshNum(); ++i) {
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