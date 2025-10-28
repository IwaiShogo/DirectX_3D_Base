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
#include "Main.h"
#include "Systems/DirectX/DirectX.h"
#include "Systems/Geometory.h"
#include "Systems/Input.h"
#include <iostream>

using namespace DirectX;

// ===== 描画リソースの静的定義 =====
namespace RenderResource
{
	// 事前にロードするモデルのリソースID
	static uint32_t PlayerModelID = 0;
	static uint32_t GroundModelID = 0;
	// 事前にロードするテクスチャのリソースID
	static uint32_t NatureTexID = 0;
}

void RenderSystem::Init()
{
	m_coordinator = GameScene::GetCoordinator();

	// --- 3Dモデルとテクスチャのロード ---
	// 提供されたAssetsから、地面とプレイヤーに利用できそうなモデルをロード

	// モデルシステムAPI (Model.h/cppに存在する想定) を利用してモデルとテクスチャをロード
	// Model::Init()はMain.cppのInitDirectX後で呼び出し済みと仮定、ここではリソースをロードする

	// 地面モデルをロード
	//
	//RenderResource::GroundModelID = Model::LoadModel(ASSET("Model/LowPolyNature/Ground_01.fbx"));

	//// プレイヤーモデルを仮にRock_01.fbxで代替
	////
	//RenderResource::PlayerModelID = Model::LoadModel(ASSET("Model/LowPolyNature/Rock_01.fbx"));

	//// 共通テクスチャをロード
	////
	//RenderResource::NatureTexID = Texture::LoadTexture(ASSET("Model/LowPolyNature/Nature_Texture_01.png"));
}

/**
 * @brief カメラ設定とデバッグ描画を行う
 */
void RenderSystem::DrawSetup()
{
	// ***** Main.cppから移植したカメラとグリッドの描画ロジック *****

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
	// Systemが保持するEntityセットをイテレート
	for (auto const& entity : m_entities)
	{
		// Componentを高速に取得
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		// 1. ワールド行列の計算 (TransformComponent -> XMMATRIX -> XMFLOAT4X4)

		// スケール行列
		XMMATRIX S = XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z);

		// 回転行列 (Z -> X -> Y の順で適用)
		XMMATRIX Rx = XMMatrixRotationX(transform.Rotation.x);
		XMMATRIX Ry = XMMatrixRotationY(transform.Rotation.y);
		XMMATRIX Rz = XMMatrixRotationZ(transform.Rotation.z);
		XMMATRIX R = Rz * Rx * Ry;

		// 平行移動行列
		XMMATRIX T = XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);

		// ワールド行列: S * R * T (スケール -> 回転 -> 移動)
		XMMATRIX worldMatrix = S * R * T;

		// DirectXへ渡すために転置して格納
		XMFLOAT4X4 fMat;
		XMStoreFloat4x4(&fMat, XMMatrixTranspose(worldMatrix));

		// 2. 描画処理 (RenderComponent)

		// 行列を設定
		Geometory::SetWorld(fMat);

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
			break;
		case MESH_NONE:
			// 何もしない
			break;
		}
	}
}