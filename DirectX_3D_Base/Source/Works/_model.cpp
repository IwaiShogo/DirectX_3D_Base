#include "Systems/Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


void Model::MakeMesh(const void* ptr, float scale, Flip flip)
{
	const aiScene* pScene = reinterpret_cast<const aiScene*>(ptr);

	// メッシュ配列の確保
	m_meshes.resize(pScene->mNumMeshes);

	for (UINT i = 0; i < pScene->mNumMeshes; ++i)
	{
		aiMesh* pAssimpMesh = pScene->mMeshes[i];
		Mesh& mesh = m_meshes[i];

		// --- 1. 頂点情報の抽出 ---
		mesh.vertices.resize(pAssimpMesh->mNumVertices);
		for (UINT v = 0; v < pAssimpMesh->mNumVertices; ++v)
		{
			Vertex& vertex = mesh.vertices[v];
			aiVector3D pos = pAssimpMesh->mVertices[v];
			aiVector3D normal = pAssimpMesh->mNormals[v];

			// 座標・法線
			vertex.pos = DirectX::XMFLOAT3(pos.x * scale, pos.y * scale, pos.z * scale);
			vertex.normal = DirectX::XMFLOAT3(normal.x, normal.y, normal.z);

			// UV
			if (pAssimpMesh->HasTextureCoords(0))
			{
				vertex.uv = DirectX::XMFLOAT2(pAssimpMesh->mTextureCoords[0][v].x, pAssimpMesh->mTextureCoords[0][v].y);
			}
			else
			{
				vertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f);
			}

			// カラー
			if (pAssimpMesh->HasVertexColors(0))
			{
				vertex.color = DirectX::XMFLOAT4(
					pAssimpMesh->mColors[0][v].r, pAssimpMesh->mColors[0][v].g, pAssimpMesh->mColors[0][v].b, pAssimpMesh->mColors[0][v].a
				);
			}
			else
			{
				vertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			// 反転処理
			if (flip == XFlip)
			{
				vertex.pos.x *= -1.0f;
				vertex.normal.x *= -1.0f;
			}
			else if (flip == ZFlip || flip == ZFlipUseAnime)
			{
				vertex.pos.z *= -1.0f;
				vertex.normal.z *= -1.0f;
			}

			// ウェイトとインデックスの初期化
			for (int j = 0; j < 4; ++j) {
				vertex.weight[j] = 0.0f;
				vertex.index[j] = 0;
			}
		}

		// --- 2. インデックス情報の抽出 ---
		mesh.indices.resize(pAssimpMesh->mNumFaces * 3);
		for (UINT f = 0; f < pAssimpMesh->mNumFaces; ++f)
		{
			aiFace& face = pAssimpMesh->mFaces[f];
			mesh.indices[f * 3 + 0] = face.mIndices[0];
			mesh.indices[f * 3 + 1] = face.mIndices[1];
			mesh.indices[f * 3 + 2] = face.mIndices[2];
			if (flip != None) std::swap(mesh.indices[f * 3 + 1], mesh.indices[f * 3 + 2]);
		}

		// --- 3. マテリアルID ---
		mesh.materialID = pAssimpMesh->mMaterialIndex;

		// --- 4. ウェイト計算（★ここが重要） ---
		MakeWeight(ptr, i);

		// --- 5. GPUバッファの作成 ---
		MeshBuffer::Description desc = {};
		desc.pVtx = mesh.vertices.data();
		desc.vtxSize = sizeof(Vertex);
		desc.vtxCount = static_cast<UINT>(mesh.vertices.size());
		desc.pIdx = mesh.indices.data();
		desc.idxSize = sizeof(unsigned long);
		desc.idxCount = static_cast<UINT>(mesh.indices.size());
		desc.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		desc.isWrite = false;

		mesh.pMesh = new MeshBuffer();
		mesh.pMesh->Create(desc);
	}
}

void Model::MakeMaterial(const void* ptr, std::string directory)
{
	// 事前準備
	aiColor3D color(0.0f, 0.0f, 0.0f);
	float shininess = 0.0f;
	const aiScene* pScene = reinterpret_cast<const aiScene*>(ptr);

	// マテリアルの作成
	m_materials.resize(pScene->mNumMaterials);
	for (unsigned int i = 0; i < m_materials.size(); ++i)
	{
		//--- 各種マテリアルパラメーターの読み取り
		// ☆拡散光の読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
			m_materials[i].diffuse = DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f);
		else
			m_materials[i].diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		// ☆環境光の読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
			m_materials[i].ambient = DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f);
		else
			m_materials[i].ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
		// ☆反射光の読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
			m_materials[i].specular = DirectX::XMFLOAT4(color.r, color.g, color.b, 0.0f);
		else
			m_materials[i].specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		// ☆反射光の強さを読み取り
		if (pScene->mMaterials[i]->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
			m_materials[i].specular.w = shininess;

		// テクスチャ読み込み処理
		HRESULT hr;
		aiString path;

		// テクスチャのパス情報を読み込み
		m_materials[i].pTexture = nullptr;
		if (pScene->mMaterials[i]->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) != AI_SUCCESS) {
			continue;
		}

		// テクスチャ領域確保
		m_materials[i].pTexture = new Texture;

		// そのまま読み込み
		hr = m_materials[i].pTexture->Create(path.C_Str());
		if (SUCCEEDED(hr)) { continue; }

		// ディレクトリと連結して探索
		hr = m_materials[i].pTexture->Create((directory + path.C_Str()).c_str());
		if (SUCCEEDED(hr)) { continue; }

		// モデルと同じ階層を探索
		// パスからファイル名のみ取得
		std::string fullPath = path.C_Str();
		std::string::iterator strIt = fullPath.begin();
		while (strIt != fullPath.end()) {
			if (*strIt == '/')
				*strIt = '\\';
			++strIt;
		}
		size_t find = fullPath.find_last_of("\\");
		std::string fileName = fullPath;
		if (find != std::string::npos)
			fileName = fileName.substr(find + 1);
		// テクスチャの読込
		hr = m_materials[i].pTexture->Create((directory + fileName).c_str());
		if (SUCCEEDED(hr)) { continue; }

		// テクスチャが見つからなかった
		delete m_materials[i].pTexture;
		m_materials[i].pTexture = nullptr;
#ifdef _DEBUG
		m_errorStr += path.C_Str();
#endif
	}
}