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
	// --- 軸線・グリッドの表示設定 ---
	// グリッド（地面の網目）
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

	// ★ 修正箇所 ★
	// 以下の「軸（XYZ）」の描画が画面中央の線の正体です。
	// 不要な場合はコメントアウトすることで、画面上の邪魔な線を消せます。

	/* // X軸（赤）
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(size, 0, 0), XMFLOAT4(1, 0, 0, 1));
	// Y軸（緑） <- これが中央の横線の正体！
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, size, 0), XMFLOAT4(0, 1, 0, 1));
	// Z軸（青）
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, size), XMFLOAT4(0, 0, 1, 1));
	// 反対方向の軸（黒）
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(-size, 0, 0), XMFLOAT4(0, 0, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, -size), XMFLOAT4(0, 0, 0, 1));
	*/

#endif

	// マップ生成システムのデバッグ行（これも必要に応じてMapGenerationSystem内で緑色の線を引いている可能性があります）
	auto mapGenSystem = ECS::ECSInitializer::GetSystem<MapGenerationSystem>();
	if (mapGenSystem)
	{
		mapGenSystem->DrawDebugLines();
	}

	// 登録されたすべてのラインを描画
	//Geometory::DrawLines();

	
}

/**
 * @brief 1つのエンティティを描画する内部関数
 */
void RenderSystem::DrawEntityInternal(ECS::EntityID entity, const DirectX::XMFLOAT4X4& viewMat, const DirectX::XMFLOAT4X4& projMat, bool isTransparentPass)
{
	TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
	RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

	DirectX::XMFLOAT4X4 wvp[3];
	DirectX::XMMATRIX world;

	// 1. ワールド行列の計算
	XMMATRIX S = XMMatrixScaling(transform.scale.x, transform.scale.y, transform.scale.z);
	XMMATRIX Rx = XMMatrixRotationX(transform.rotation.x);
	XMMATRIX Ry = XMMatrixRotationY(transform.rotation.y);
	XMMATRIX Rz = XMMatrixRotationZ(transform.rotation.z);
	XMMATRIX R = Rz * Rx * Ry;
	XMMATRIX T = XMMatrixTranslation(transform.position.x, transform.position.y, transform.position.z);
	world = S * R * T;

	XMStoreFloat4x4(&wvp[0], XMMatrixTranspose(world));
	wvp[1] = viewMat;
	wvp[2] = projMat;

	// シェーダーへの変換行列を設定
	Geometory::SetWorld(wvp[0]);
	Geometory::SetView(wvp[1]);
	Geometory::SetProjection(wvp[2]);
	ShaderList::SetWVP(wvp);

	// 色設定
	Geometory::SetColor(render.color);

	switch (render.type)
	{
	case MESH_BOX:
	case MESH_SPHERE:
		// プリミティブはマテリアルがないので、エンティティ全体の透明度で判断
		// 透明パスなのに不透明(1.0)ならスキップ、不透明パスなのに透明(<1.0)ならスキップ
		if (isTransparentPass != (render.color.w < 1.0f)) return;

		Geometory::DrawBox();
		break;

	case MESH_MODEL:
	{

		ModelComponent& model = m_coordinator->GetComponent<ModelComponent>(entity);
		if (model.pModel)
		{
			// シェーダー設定 (変更なし)
			if (m_coordinator->HasComponent<AnimationComponent>(entity))
				model.pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_ANIME));
			else
				model.pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));

			model.pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

			// ★修正: メッシュごとのループ内で選別を行う
			for (uint32_t i = 0; i < model.pModel->GetMeshNum(); ++i)
			{
				const Model::Mesh* pMesh = model.pModel->GetMesh(i);
				Model::Material material = *model.pModel->GetMaterial(pMesh->materialID);

				// 最終的なアルファ値 = マテリアルのAlpha * RenderComponentのAlpha
				float finalAlpha = material.diffuse.w * render.color.w;

				// このメッシュは透明か？ (0.99f以下なら透明とみなす)
				bool isMeshTransparent = (finalAlpha < 0.99f);

				// ★判定: 今の描画パスと、メッシュの性質が合致しなければスキップ
				if (isTransparentPass)
				{
					// 透明パス中: 「不透明なメッシュ」は描かない
					if (!isMeshTransparent) continue;
				}
				else
				{
					// 不透明パス中: 「透明なメッシュ」は描かない
					if (isMeshTransparent) continue;
				}

				// 描画採用！
				// 計算したアルファ値を適用して描画
				material.diffuse.w = finalAlpha;
				ShaderList::SetMaterial(material);
				model.pModel->Draw(i);
			}
		}
	}
	break;
	}
}

/**
 * @brief 
 
 
 
 
 
 
 
 
 
 
 
 
 とRenderComponentを持つ全てのEntityを描画する
 */
void RenderSystem::DrawEntities()
{
	// 1. カメラ取得
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

		if (m_coordinator->HasComponent<TransformComponent>(cameraID))
		{
			auto& trans = m_coordinator->GetComponent<TransformComponent>(cameraID);
			ShaderList::SetCameraPos(cam.worldPosition);
		}
	}
	else
	{
		cameraID = ECS::FindFirstEntityWithComponent<BasicCameraComponent>(m_coordinator);
		if (cameraID != ECS::INVALID_ENTITY_ID)
		{
			auto& cam = m_coordinator->GetComponent<BasicCameraComponent>(cameraID);
			viewMat = cam.viewMatrix;
			projMat = cam.projectionMatrix;
			cameraFound = true;
		}

		if (m_coordinator->HasComponent<TransformComponent>(cameraID))
		{
			auto& trans = m_coordinator->GetComponent<TransformComponent>(cameraID);
			ShaderList::SetCameraPos(trans.position);
		}
	}

	if (!cameraFound) return;

	// 2. ライト設定 (カメラ方向からの平行光源)
	//{
	//	DirectX::XMFLOAT3 lightDir = { -0.5f, -1.0f, 0.5f };

	//	// 正規化（長さを1にする）
	//	XMVECTOR vLight = XMLoadFloat3(&lightDir);
	//	vLight = XMVector3Normalize(vLight);
	//	XMStoreFloat3(&lightDir, vLight);

	//	// セット (色は真っ白: 1,1,1,1)
	//	ShaderList::SetLight(XMFLOAT4(0.1f, 0.1f, 0.3f, 1.0f), lightDir);
	//}
	// ライトリストの作成
	PointLightData lights[MAX_LIGHTS];
	int lightCount = 0;

	// 1. 【プレイヤーのライト】を0番目に登録 (最優先)
	if (m_coordinator->HasComponent<CameraComponent>(cameraID))
	{
		auto& cam = m_coordinator->GetComponent<CameraComponent>(cameraID);

		lights[lightCount].position = { cam.worldPosition.x, cam.worldPosition.y, cam.worldPosition.z, 30.0f }; // w=範囲30m

		// 時間計測用の変数 (staticにすることで値を保持し続ける)
		static float timeCounter = 0.0f;
		timeCounter += 0.05f; // 数値を変えると明滅の速さが変わります

		// 1. ベースの揺らぎ (サイン波でゆっくり明滅)
		// sinの結果(-1.0~1.0)を 0.8~1.1 くらいの範囲に調整
		float wave = sin(timeCounter);
		float intensity = 0.9f + (wave * 0.15f);

		// 2. ノイズ (たまにチカッと暗くする)
		// 乱数(0~99)が 5未満(5%の確率) なら、明るさをガクッと下げる
		if (rand() % 100 < 5)
		{
			intensity *= 0.3f; // 一瞬だけ30%の明るさに
		}

		// 計算した強度(intensity)を色に掛け合わせる
		// 元の色: (1.0, 0.9, 0.7)
		lights[lightCount].color = {
			1.0f * intensity,
			0.9f * intensity,
			0.7f * intensity,
			1.0f
		};

		lightCount++;
	}

	// 2. 【シーン上の点光源】を集める
	// PointLightComponent を持つエンティティを探す
	for (auto const& entity : m_coordinator->GetActiveEntities()) // または全エンティティ走査
	{
		if (lightCount >= MAX_LIGHTS) break; // 上限チェック

		// PointLightComponentを持っていれば...
		if (m_coordinator->HasComponent<PointLightComponent>(entity) && m_coordinator->HasComponent<TransformComponent>(entity))
		{
			auto& pl = m_coordinator->GetComponent<PointLightComponent>(entity);
			auto& trans = m_coordinator->GetComponent<TransformComponent>(entity);

			if (!pl.isActive) continue;

			// 1. エンティティのワールド座標
			XMVECTOR posV = XMLoadFloat3(&trans.position);

			// 2. オフセットベクトル
			XMVECTOR offsetV = XMLoadFloat3(&pl.offset);

			// 3. エンティティの回転行列を作成
			XMMATRIX rotM = XMMatrixRotationRollPitchYaw(
				trans.rotation.x,
				trans.rotation.y,
				trans.rotation.z
			);

			// 4. オフセットを回転させる (体の向きに合わせる)
			// これにより、キャラが回転してもライトの位置関係が保たれます
			XMVECTOR rotatedOffset = XMVector3Transform(offsetV, rotM);

			// 5. 足し合わせる
			XMVECTOR finalPos = posV + rotatedOffset;

			// 格納
			XMFLOAT3 finalPosF;
			XMStoreFloat3(&finalPosF, finalPos);

			lights[lightCount].position = { finalPosF.x, finalPosF.y, finalPosF.z, pl.range };

			// FlickerComponent(点滅)があれば、その結果を反映させる
			if (m_coordinator->HasComponent<FlickerComponent>(entity) && m_coordinator->HasComponent<RenderComponent>(entity))
			{
				// RenderSystemで計算済みのRenderComponentの色を使うと楽です
				auto& render = m_coordinator->GetComponent<RenderComponent>(entity);
				lights[lightCount].color = render.color;
			}
			else
			{
				lights[lightCount].color = pl.color;
			}

			lightCount++;
		}
	}

	// 3. まとめてシェーダーに送信
	// 環境光は少し暗めの青 (0.1, 0.1, 0.3)
	ShaderList::SetPointLights(lights, lightCount, DirectX::XMFLOAT4(0.1f, 0.1f, 0.3f, 1.0f));

	// 3. 描画ターゲット設定
	RenderTarget* pRTV = GetDefaultRTV();
	DepthStencil* pDSV = GetDefaultDSV();
	SetRenderTargets(1, &pRTV, pDSV);

	SetDepthTest(true);

	// ========================================================================
	// パス1: 不透明 (Opaque) オブジェクトの描画
	// ========================================================================
	SetBlendMode(BLEND_NONE);

	for (auto const& entity : m_entities)
	{
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		// エンティティ全体が半透明設定(0.5など)なら、不透明パスでは描かない
		// ただし、モデルの場合は「部分的に不透明」な場合もあるが、
		// 通常RenderComponentのAlphaは「全体フェード」に使うため、ここではスキップでOKとします。
		// もし「半透明な幽霊の中に不透明な骨がある」表現をしたい場合は条件を緩める必要がありますが、
		// 今回のケース（ガラス）ならこのままで大丈夫です。
		if (render.type != MESH_MODEL && render.color.w < 1.0f) continue;

		// モデルの場合、RenderComponentが透明(0.5)ならここでスキップさせても良いが、
		// 安全のため「モデルならとりあえず通す」か、上記の通り「色が透明ならスキップ」のままで進めます。
		if (render.color.w < 1.0f) continue;

		// モデルの場合は「一部だけ不透明」かもしれないので、ここを通す
		// 第4引数: false (不透明パス)
		DrawEntityInternal(entity, viewMat, projMat, false);
	}

	// ========================================================================
	// パス2: 半透明 (Transparent) オブジェクトの描画
	// ========================================================================
	SetBlendMode(BLEND_ALPHA);
	
	// ★ガラスなどが綺麗に見えるよう、デプス書き込みをOFFにする場合もありますが、
	//  複雑なモデルの場合はONのままでも構いません（お好みで調整してください）
	// SetDepthWrite(false); 

	for (auto const& entity : m_entities)
	{
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		bool isModel = (render.type == MESH_MODEL);

		if (!isModel && render.color.w >= 1.0f) continue;

		// モデルの場合は「一部だけ透明」かもしれないので、ここを通す
		// 第4引数: true (透明パス)
		DrawEntityInternal(entity, viewMat, projMat, true);
	}

	// SetDepthWrite(true);
	SetBlendMode(BLEND_NONE);
}