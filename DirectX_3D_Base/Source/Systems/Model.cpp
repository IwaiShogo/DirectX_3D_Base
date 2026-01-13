#include "Systems/Model.h"
#include "Systems/DirectX/ShaderList.h"
#include "../../DirectXTex/DirectXTex.h"
#include "Systems/AssetManager.h"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifdef _DEBUG
#include "Systems/Geometory.h"
#endif

#if _MSC_VER >= 1930
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "assimp-vc143-mt.lib")
#endif
#elif _MSC_VER >= 1920
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc142-mtd.lib")
#else
#pragma comment(lib, "assimp-vc142-mt.lib")
#endif
#elif _MSC_VER >= 1910
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc141-mtd.lib")
#else
#pragma comment(lib, "assimp-vc141-mt.lib")
#endif
#endif

// staticoϐ`
VertexShader* Model::m_pDefVS = nullptr;
PixelShader* Model::m_pDefPS = nullptr;
unsigned int	Model::m_shaderRef = 0;
#ifdef _DEBUG
std::string		Model::m_errorStr = "";
#endif

/*
* @brief assimp̍sXMMATRIX^ɕϊ
* @param[in] M assimp̍s
* @return ϊ̍s
*/
DirectX::XMMATRIX GetMatrixFromAssimpMatrix(aiMatrix4x4 M)
{
	return DirectX::XMMatrixSet(
		M.a1, M.b1, M.c1, M.d1,
		M.a2, M.b2, M.c2, M.d2,
		M.a3, M.b3, M.c3, M.d3,
		M.a4, M.b4, M.c4, M.d4
	);
}

/*
* @brief ftHg̃VF[_[쐬
* @param[out] vs _VF[_[i[
* @param[out] ps sNZVF[_[i[
*/
void MakeModelDefaultShader(VertexShader** vs, PixelShader** ps)
{
	const char* ModelVS = R"EOT(
struct VS_IN {
	float3 pos : POSITION0;
	float3 normal : NORMAL0;
	float2 uv : TEXCOORD0;
};
struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 normal : NORMAL0;
	float2 uv : TEXCOORD0;
};
VS_OUT main(VS_IN vin) {
	VS_OUT vout;
	vout.pos = float4(vin.pos, 1.0f);
	vout.pos.z += 0.5f;
	vout.pos.y -= 0.8f;
	vout.normal = vin.normal;
	vout.uv = vin.uv;
	return vout;
})EOT";
	const char* ModelPS = R"EOT(
struct PS_IN {
	float4 pos : SV_POSITION;
	float3 normal : NORMAL0;
	float2 uv : TEXCOORD0;
};
Texture2D tex : register(t0);
SamplerState samp : register(s0);
float4 main(PS_IN pin) : SV_TARGET
{
	return tex.Sample(samp, pin.uv);
})EOT";
	*vs = new VertexShader();
	(*vs)->Compile(ModelVS);
	*ps = new PixelShader();
	(*ps)->Compile(ModelPS);
}



/*
* @brief RXgN^
*/
Model::Model()
	: m_loadScale(1.0f)
	, m_loadFlip(None)
	, m_playNo(ANIME_NONE)
	, m_blendNo(ANIME_NONE)
	, m_parametric{ ANIME_NONE, ANIME_NONE }
	, m_blendTime(0.0f)
	, m_blendTotalTime(0.0f)
	, m_parametricBlend(0.0f)
{
	// ftHgVF[_[̓Kp
	if (m_shaderRef == 0)
	{
		MakeModelDefaultShader(&m_pDefVS, &m_pDefPS);
	}
	m_pVS = m_pDefVS;
	m_pPS = m_pDefPS;
	++m_shaderRef;
}

/*
* @brief fXgN^
*/
Model::~Model()
{
	Reset();
	--m_shaderRef;
	if (m_shaderRef <= 0)
	{
		delete m_pDefPS;
		delete m_pDefVS;
	}
}

/*
* @brief f[^폜
*/
void Model::Reset()
{
	auto meshIt = m_meshes.begin();
	while (meshIt != m_meshes.end())
	{
		if (meshIt->pMesh) delete meshIt->pMesh;
		++meshIt;
	}

	auto matIt = m_materials.begin();
	while (matIt != m_materials.end())
	{
		if (matIt->pTexture) delete matIt->pTexture;
		++matIt;
	}
}

/*
* @brief _VF[_[ݒ
*/
void Model::SetVertexShader(VertexShader* vs)
{
	m_pVS = vs;
}

/*
* @brief sNZVF[_[ݒ
*/
void Model::SetPixelShader(PixelShader* ps)
{
	m_pPS = ps;
}

/*
* @brief ff[^ǂݍ
* @param[in] file ǂݍރft@Cւ̃pX
* @param[in] scale f̃TCYύX
* @param[in] flip ]ݒ
* @return ǂݍ݌
*/
bool Model::Load(const char* file, float scale, Flip flip)
{
#ifdef _DEBUG
	m_errorStr = "";
#endif
	Reset();

	// assimp̐ݒ
	Assimp::Importer importer;
	int flag = 0;
	flag |= aiProcess_Triangulate;
	flag |= aiProcess_FlipUVs;
	//flag |= aiProcess_MakeLeftHanded;

	// assimpœǂݍ
	const aiScene* pScene = importer.ReadFile(file, flag);
	if (!pScene) {
#ifdef _DEBUG
		m_errorStr = importer.GetErrorString();
#endif
		return false;
	}

	// ǂݍݎ̐ݒۑ
	m_loadScale = scale;
	m_loadFlip = flip;

	// fBNg̓ǂݎ
	std::string directory = file;
	auto strIt = directory.begin();
	while (strIt != directory.end()) {
		if (*strIt == '/')
			*strIt = '\\';
		++strIt;
	}
	directory = directory.substr(0, directory.find_last_of('\\') + 1);

	// m[h̍쐬
	MakeBoneNodes(pScene);
	// bV쐬
	MakeMesh(pScene, scale, flip);
	// }eA̍쐬
	MakeMaterial(pScene, directory);

	return true;
}

/*
* @brief `
* @param[in] meshNo `悷郁bVA-1͑S\
* @param[in] func bV`R[obN
*/
void Model::Draw(int meshNo)
{
	// VF[_[ݒ
	m_pVS->Bind();
	m_pPS->Bind();

	// eNX`ݒtO
	bool isAutoTexture = (meshNo == -1);

	// `͈͐ݒ
	size_t drawNum = m_meshes.size();
	UINT startMesh = 0;
	if (meshNo != -1) {
		startMesh = meshNo;
		drawNum = meshNo + 1;
	}

	// bVƂ̕`惋[v
	for (UINT i = startMesh; i < drawNum; ++i)
	{
		// eNX`ݒ
		if (isAutoTexture) {
			m_pPS->SetTexture(0, m_materials[m_meshes[i].materialID].pTexture);
		}

		// --- XLjOs̍쐬Ɠ] ---

		// 1. zmۂAׂĒPʍsŏ (dv)
		//    ȂƁAgpȂvfɃS~AGPUŕsN܂
		DirectX::XMFLOAT4X4 boneMatrices[MAX_BONE];

		const UINT boneCount = static_cast<UINT>(std::min(m_meshes[i].bones.size(), static_cast<size_t>(MAX_BONE)));
		for (UINT b = 0; b < boneCount; ++b)
		{
			const Bone& bone = m_meshes[i].bones[b];

			DirectX::XMMATRIX m = DirectX::XMMatrixIdentity();
			if (bone.index != INDEX_NONE)
			{
				// ŏIXLjOs
				m = bone.invOffset * m_nodes[bone.index].mat;
			}

			DirectX::XMStoreFloat4x4(&boneMatrices[b], DirectX::XMMatrixTranspose(m));
		}

		for (UINT b = boneCount; b < MAX_BONE; ++b)
		{
			DirectX::XMStoreFloat4x4(&boneMatrices[b], DirectX::XMMatrixIdentity());
		}

		// 3. 萔obt@֏
		//    ShaderListoRāA݃oChĂVF[_[(VS_ANIME)̒萔obt@XV
		ShaderList::SetBones(boneMatrices);
		// ----------------------------------------

		// `
		if (m_meshes[i].pMesh)
		{
			m_meshes[i].pMesh->Draw();
		}
	}
}

/*
* @brief bV擾
* @param[in] index bVԍ
* @return YbV
*/
const Model::Mesh* Model::GetMesh(unsigned int index)
{
	if (index < GetMeshNum())
	{
		return &m_meshes[index];
	}
	return nullptr;
}

/*
* @brief bV擾
*/
uint32_t Model::GetMeshNum()
{
	return static_cast<uint32_t>(m_meshes.size());
}

/*
* @brief }eA擾
* @param[in] index }eAԍ
* @return Y}eA
*/
const Model::Material* Model::GetMaterial(unsigned int index)
{
	if (index < GetMaterialNum())
	{
		return &m_materials[index];
	}
	return nullptr;
}

/*
* @brief }eA擾
*/
uint32_t Model::GetMaterialNum()
{
	return static_cast<uint32_t>(m_materials.size());
}

/*
* @brief Aj[V̕ϊs擾
* @param[in] index {[ԍ
* @return Y{[̕ϊs
*/
DirectX::XMMATRIX Model::GetBone(NodeIndex index)
{
	if (index < m_nodes.size())
	{
		return m_nodes[index].mat;
	}
	return DirectX::XMMatrixIdentity();
}

/*
* @brief Aj[V擾
* @param[in] no Ajԍ
* @return YAj[V
*/
const Model::Animation* Model::GetAnimation(AnimeNo no)
{
	if (AnimeNoCheck(no))
	{
		return &m_animes[no];
	}
	return nullptr;
}

// ID݂̂w肵ă[h֐
Model::AnimeNo Model::AddAnimation(const std::string& assetID)
{
	if (m_animeIdMap.count(assetID) > 0)
	{
		return m_animeIdMap[assetID];
	}

	// AssetManagerpX擾
	// AssetOԂ̃VOgɃANZX
	std::string filePath = Asset::AssetManager::GetInstance().GetAnimationPath(assetID);

	if (filePath.empty())
	{
		// pXȂꍇ̃G[
		return ANIME_NONE;
	}

	// pXgă[hsi2IDnă}bvo^Cj
	return AddAnimation(filePath.c_str(), assetID);
}

/*
* @brief Aj[Vǂݍ
* @param[in] file ǂݍރAj[Vt@Cւ̃pX
* @return Ŋ蓖ĂꂽAj[Vԍ
*/
Model::AnimeNo Model::AddAnimation(const char* file, const std::string& aliasID)
{
#ifdef _DEBUG
	m_errorStr = "";
#endif

	// assimp̐ݒ
	Assimp::Importer importer;
	int flag = 0;
	flag |= aiProcess_Triangulate;
	flag |= aiProcess_FlipUVs;
	if (m_loadFlip == Flip::XFlip)  flag |= aiProcess_MakeLeftHanded;

	// assimpœǂݍ
	const aiScene* pScene = importer.ReadFile(file, flag);
	if (!pScene)
	{
#ifdef _DEBUG
		m_errorStr += importer.GetErrorString();
#endif
		return ANIME_NONE;
	}

	// Aj[V`FbN
	if (!pScene->HasAnimations())
	{
#ifdef _DEBUG
		m_errorStr += "no animation.";
#endif
		return ANIME_NONE;
	}

	// Aj[Vf[^m
	aiAnimation* assimpAnime = pScene->mAnimations[0];
	m_animes.push_back(Animation());
	Animation& anime = m_animes.back();

	// Aj[Vݒ
	float animeFrame = static_cast<float>(assimpAnime->mTicksPerSecond);
	anime.totalTime = static_cast<float>(assimpAnime->mDuration) / animeFrame;
	anime.channels.resize(assimpAnime->mNumChannels);
	Channels::iterator channelIt = anime.channels.begin();
	while (channelIt != anime.channels.end())
	{
		// Ή`l({[)T
		uint32_t channelIdx = static_cast<uint32_t>(channelIt - anime.channels.begin());
		aiNodeAnim* assimpChannel = assimpAnime->mChannels[channelIdx];
		Model::Nodes::iterator nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(),
			[assimpChannel](Node& node) {
				return node.name == assimpChannel->mNodeName.data;
			});
		if (nodeIt == m_nodes.end())
		{
			channelIt->index = INDEX_NONE;
			++channelIt;
			continue;
		}

		// eL[̒lݒ
		channelIt->index = static_cast<NodeIndex>(nodeIt - m_nodes.begin());
		Timeline& timeline = channelIt->timeline;

		// xXMVECTOR^Ŋi[
		using XMVectorKey = std::pair<float, DirectX::XMVECTOR>;
		using XMVectorKeys = std::map<float, DirectX::XMVECTOR>;
		XMVectorKeys keys[3];
		// ʒu
		for (UINT i = 0; i < assimpChannel->mNumPositionKeys; ++i)
		{
			aiVectorKey& key = assimpChannel->mPositionKeys[i];
			keys[0].insert(XMVectorKey(static_cast<float>(key.mTime) / animeFrame,
				DirectX::XMVectorSet(key.mValue.x, key.mValue.y, key.mValue.z, 0.0f)
			));
		}
		// ]
		for (UINT i = 0; i < assimpChannel->mNumRotationKeys; ++i)
		{
			aiQuatKey& key = assimpChannel->mRotationKeys[i];
			keys[1].insert(XMVectorKey(static_cast<float>(key.mTime) / animeFrame,
				DirectX::XMVectorSet(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w)));
		}
		// gk
		for (UINT i = 0; i < assimpChannel->mNumScalingKeys; ++i)
		{
			aiVectorKey& key = assimpChannel->mScalingKeys[i];
			keys[2].insert(XMVectorKey(static_cast<float>(key.mTime) / animeFrame,
				DirectX::XMVectorSet(key.mValue.x, key.mValue.y, key.mValue.z, 0.0f)));
		}

		// e^CC̐擪̎QƂݒ
		XMVectorKeys::iterator it[] = { keys[0].begin(), keys[1].begin(), keys[2].begin() };
		for (int i = 0; i < 3; ++i)
		{
			// L[Ȃꍇ́AQƏI
			if (keys[i].size() == 1)
				++it[i];
		}

		// evfƂ̃^CCł͂ȂAׂĂ̕ϊ܂߂^CC̍쐬
		while (it[0] != keys[0].end() || it[1] != keys[1].end() || it[2] != keys[2].end())
		{
			// ŏԂ̎ZoWbN
			// ̎QƈʒuňԏԂ擾
			float time = anime.totalTime; // [vɏKv͏\傫Ȓl
			bool isEnd = true; // Ip

			for (int i = 0; i < 3; ++i)
			{
				// ύXFCe[^I[łȂꍇ̂ݎԂr
				if (it[i] != keys[i].end())
				{
					time = std::min(it[i]->first, time);
					isEnd = false;
				}
			}

			// ׂẴCe[^I[ȂI
			if (isEnd) break;

			// ԂɊÂĕԒlvZ
			DirectX::XMVECTOR result[3];
			for (int i = 0; i < 3; ++i)
			{
				// 擪̃L[菬Ԃł΁A擪̒lݒ
				if (time < keys[i].begin()->first)
				{
					result[i] = keys[i].begin()->second;
				}
				// ŏIL[傫Ԃł΁AŏI̒lݒ
				else if (keys[i].rbegin()->first <= time)
				{
					result[i] = keys[i].rbegin()->second;
					it[i] = keys[i].end();
				}
				// L[mɋ܂ꂽԂł΁AԒlvZ
				else
				{
					// QƂĂ鎞ԂƓł΁A̎QƂփL[i߂
					if (it[i]->first <= time)
					{
						++it[i];
					}

					// ԒľvZ
					XMVectorKeys::iterator prev = it[i];
					--prev;
					float rate = (time - prev->first) / (it[i]->first - prev->first);
					result[i] = DirectX::XMVectorLerp(prev->second, it[i]->second, rate);
				}
			}

			// w莞ԂɊÂL[ǉ
			Transform transform;
			DirectX::XMStoreFloat3(&transform.translate, result[0]);
			DirectX::XMStoreFloat4(&transform.quaternion, result[1]);
			DirectX::XMStoreFloat3(&transform.scale, result[2]);
			timeline.insert(Key(time, transform));
		}

		++channelIt;
	}

	// Ajԍ
	AnimeNo newIndex = static_cast<AnimeNo>(m_animes.size() - 1);

	// IDw肳Ă΃}bvɓo^
	if (!aliasID.empty())
	{
		m_animeIdMap[aliasID] = newIndex;
	}

	return newIndex;
}

/*
* @brief Aj[V̍XV
* @param[in] tick Aj[Voߎ
*/
void Model::Step(float tick)
{
	// Aj[V̍ĐmF
	if (m_playNo == ANIME_NONE) { return; }

	//--- Aj[Vs̍XV
	// pgbN
	if (m_playNo == PARAMETRIC_ANIME || m_blendNo == PARAMETRIC_ANIME)
	{
		CalcAnime(PARAMETRIC0, m_parametric[0]);
		CalcAnime(PARAMETRIC1, m_parametric[1]);
	}
	// CAj
	if (m_playNo != ANIME_NONE && m_playNo != PARAMETRIC_ANIME)
	{
		CalcAnime(MAIN, m_playNo);
	}
	// uhAj
	if (m_blendNo != ANIME_NONE && m_blendNo != PARAMETRIC_ANIME)
	{
		CalcAnime(BLEND, m_blendNo);
	}

	std::function<void(NodeIndex, DirectX::XMMATRIX)> funcCalcBones =
		[&](NodeIndex index, DirectX::XMMATRIX parent)
		{
			Transform transform;
			// pgbN
			if (m_playNo == PARAMETRIC_ANIME || m_blendNo == PARAMETRIC_ANIME)
			{
				LerpTransform(&transform, m_nodeTransform[PARAMETRIC0][index], m_nodeTransform[PARAMETRIC1][index], m_parametricBlend);
				if (m_playNo == PARAMETRIC_ANIME) m_nodeTransform[MAIN][index] = transform;
				if (m_blendNo == PARAMETRIC_ANIME) m_nodeTransform[BLEND][index] = transform;
			}
			// uhAj
			if (m_blendNo != ANIME_NONE)
			{
				LerpTransform(&transform, m_nodeTransform[MAIN][index], m_nodeTransform[BLEND][index], m_blendTime / m_blendTotalTime);
			}
			else
			{
				transform = m_nodeTransform[MAIN][index];
			}

			// Ym[h̎psvZ
			Node& node = m_nodes[index];
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&transform.translate));
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&transform.quaternion));
			DirectX::XMMATRIX S = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&transform.scale));
			node.mat = (S * R * T) * parent;

			// qvf̎pXV
			for (auto child : node.children)
			{
				funcCalcBones(child, node.mat);
			}
		};

	funcCalcBones(0, DirectX::XMMatrixScaling(m_loadScale, m_loadScale, m_loadScale));


	//--- Aj[V̎ԍXV
	// CAj
	if (m_playNo != ANIME_NONE && m_playNo != PARAMETRIC_ANIME) {
		UpdateAnime(m_playNo, tick);
	}
	// uhAj
	if (m_blendNo != ANIME_NONE)
	{
		if (m_blendNo != PARAMETRIC_ANIME) {
			UpdateAnime(m_blendNo, tick);
		}
		m_blendTime += tick;
		if (m_blendTime >= m_blendTotalTime) // oOC: <= ł͂Ȃ >=
		{
			// uhAj̎I
			m_blendTime = 0.0f;
			m_blendTotalTime = 0.0f;
			m_playNo = m_blendNo;
			m_blendNo = ANIME_NONE;
		}
	}
	// pgbN
	if (m_playNo == PARAMETRIC_ANIME || m_blendNo == PARAMETRIC_ANIME)
	{
		UpdateAnime(m_parametric[0], tick);
		UpdateAnime(m_parametric[1], tick);
	}
}

/*
* @brief Aj[VĐ
* @param[in] no ĐAj[Vԍ
* @param[in] loop [vĐtO
* @param[in] speed Đx
*/
void Model::Play(AnimeNo no, bool loop, float speed)
{
	// 再生チェック
	if (!AnimeNoCheck(no)) { return; }

	// IMPORTANT: allow restarting even if the same animation is requested
	// This avoids "resume from last time" when Play is called again for UI effects.
	// Also clear blend state so previous transitions do not leak.
	m_blendTime = 0.0f;
	m_blendTotalTime = 0.0f;
	m_blendNo = ANIME_NONE;

	// 合成アニメーションかチェック
	if (no != PARAMETRIC_ANIME)
	{
		// 通常の初期化（nowTimeを0に戻す）
		InitAnime(no);
		m_animes[no].isLoop = loop;
		m_animes[no].speed = speed;
	}
	else
	{
		// 合成アニメーションの元になっているアニメーションを初期化
		InitAnime(m_parametric[0]);
		InitAnime(m_parametric[1]);
		m_animes[m_parametric[0]].isLoop = loop;
		m_animes[m_parametric[1]].isLoop = loop;
		SetParametricBlend(0.0f);
	}

	// 再生アニメーションの設定（同一noでも上のInitで必ずリスタートする）
	m_playNo = no;
}


void Model::Play(const std::string& assetID, bool loop, float speed)
{
	auto it = m_animeIdMap.find(assetID);
	if (it != m_animeIdMap.end())
	{
		// CfbNXgĊPlayĂ
		Play(it->second, loop, speed);
	}
	else
	{
		std::cerr << "Warning: Animation ID '" << assetID << "' not found in model." << std::endl;
	}
}

/*
* @brief uhĐ
* @param[in] no Aj[Vԍ
* @param[in] blendTime uhɊ|鎞
* @param[in] loop [vtO
* @param[in] speed Đx
*/
void Model::PlayBlend(AnimeNo no, float blendTime, bool loop, float speed)
{
	// Đ`FbN
	if (!AnimeNoCheck(no)) { return; }

	// Aj[V`FbN
	if (no != PARAMETRIC_ANIME)
	{
		InitAnime(no);
		m_animes[no].isLoop = loop;
		m_animes[no].speed = speed;
	}
	else
	{
		// Aj[V̌ɂȂĂAj[V
		InitAnime(m_parametric[0]);
		InitAnime(m_parametric[1]);
		m_animes[m_parametric[0]].isLoop = loop;
		m_animes[m_parametric[1]].isLoop = loop;
		SetParametricBlend(0.0f);
	}

	// uh̐ݒ
	m_blendTime = 0.0f;
	m_blendTotalTime = blendTime;
	m_blendNo = no;
}

void Model::PlayBlend(const std::string& animeID, float blendTime, bool loop, float speed)
{
	// IDCfbNX
	auto it = m_animeIdMap.find(animeID);
	if (it != m_animeIdMap.end())
	{
		// CfbNX(AnimeNo)gāAPlayBlendĂ
		PlayBlend(it->second, blendTime, loop, speed);
	}
	else
	{
		// IDȂꍇ̓G[OoĉȂ
		std::cerr << "Warning: PlayBlend failed. Animation ID '" << animeID << "' not found." << std::endl;
	}
}

/*
* @brief Aj[V̐ݒ
* @param[in] no1 Aj1
* @param[in] no2 Aj2
*/
void Model::SetParametric(AnimeNo no1, AnimeNo no2)
{
	// Aj[V`FbN
	if (!AnimeNoCheck(no1)) { return; }
	if (!AnimeNoCheck(no2)) { return; }

	// ݒ
	m_parametric[0] = no1;
	m_parametric[1] = no2;
	SetParametricBlend(0.0f);
}

/*
* @brief Aj[V̍ݒ
* @param[in] blendRate
*/
void Model::SetParametricBlend(float blendRate)
{
	// Ajݒ肳Ă邩mF
	if (m_parametric[0] == ANIME_NONE || m_parametric[1] == ANIME_NONE) return;

	// ݒ
	m_parametricBlend = blendRate;

	// ɊÂăAj[V̍Đxݒ
	Animation& anime1 = m_animes[m_parametric[0]];
	Animation& anime2 = m_animes[m_parametric[1]];
	float blendTotalTime = anime1.totalTime * (1.0f - m_parametricBlend) + anime2.totalTime * m_parametricBlend;
	anime1.speed = anime1.totalTime / blendTotalTime;
	anime2.speed = anime2.totalTime / blendTotalTime;
}

/*
* @brief Aj[V̍ĐԂύX
* @param[in] no ύXAj
* @param[in] time VĐ
*/
void Model::SetAnimationTime(AnimeNo no, float time)
{
	// Aj[V`FbN
	if (!AnimeNoCheck(no)) { return; }

	// ĐԕύX
	Animation& anime = m_animes[no];
	anime.nowTime = time;
	while (anime.nowTime >= anime.totalTime)
	{
		anime.nowTime -= anime.totalTime;
	}
}

/*
* @brief ĐtO̎擾
* @param[in] no ׂAjԍ
* @return ݍĐȂtrue
*/
bool Model::IsPlay(AnimeNo no)
{
	// Aj[V`FbN
	if (!AnimeNoCheck(no)) { return false; }

	// pgbÑ͍Ajɔf
	if (no == PARAMETRIC_ANIME) { no = m_parametric[0]; }

	// ĐԂ̔
	if (m_animes[no].totalTime < m_animes[no].nowTime) { return false; }

	// ꂼ̍Đԍɐݒ肳Ă邩mF
	if (m_playNo == no) { return true; }
	if (m_blendNo == no) { return true; }
	if (m_playNo == PARAMETRIC_ANIME || m_blendNo == PARAMETRIC_ANIME)
	{
		if (m_parametric[0] == no) { return true; }
		if (m_parametric[1] == no) { return true; }
	}

	// ĐłȂ
	return false;
}

/*
* @brief Đ̔ԍ̎擾
* @return Ajԍ
*/
Model::AnimeNo Model::GetPlayNo()
{
	return m_playNo;
}

/*
* @brief Đ̃uhAj̎擾
* @return Ajԍ
*/
Model::AnimeNo Model::GetBlendNo()
{
	return m_blendNo;
}


#ifdef _DEBUG

/*
* @brief G[bZ[W擾
* @returnn G[bZ[W
*/
std::string Model::GetError()
{
	return m_errorStr;
}

/*
* @brief {[fobO`
*/
void Model::DrawBone()
{
	// ċA
	std::function<void(int, DirectX::XMFLOAT3)> FuncDrawBone =
		[&FuncDrawBone, this](int idx, DirectX::XMFLOAT3 parent)
		{
			// em[h猻݈ʒu܂ŕ`
			DirectX::XMFLOAT3 pos;
			DirectX::XMStoreFloat3(&pos, DirectX::XMVector3TransformCoord(DirectX::XMVectorZero(), m_nodes[idx].mat));
			Geometory::AddLine(parent, pos, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));

			// qm[h̕`
			auto it = m_nodes[idx].children.begin();
			while (it != m_nodes[idx].children.end())
			{
				FuncDrawBone(*it, pos);
				++it;
			}
		};

	// `s
	FuncDrawBone(0, DirectX::XMFLOAT3());
	Geometory::DrawLines();
}

#endif


void Model::MakeBoneNodes(const void* ptr)
{
	// ċAAssimp̃m[hǂݎ
	std::function<NodeIndex(aiNode*, NodeIndex, DirectX::XMMATRIX mat)> FuncAssimpNodeConvert =
		[&FuncAssimpNodeConvert, this](aiNode* assimpNode, NodeIndex parent, DirectX::XMMATRIX mat)
		{
			DirectX::XMMATRIX transform = GetMatrixFromAssimpMatrix(assimpNode->mTransformation);
			std::string name = assimpNode->mName.data;
			if (name.find("$AssimpFbx") != std::string::npos)
			{
				mat = transform * mat;
				return FuncAssimpNodeConvert(assimpNode->mChildren[0], parent, mat);
			}
			else
			{
				// Assimp̃m[hfNX֊i[
				Node node;
				node.name = assimpNode->mName.data;
				node.parent = parent;
				node.children.resize(assimpNode->mNumChildren);
				node.mat = mat;

				// m[hXgɒǉ
				m_nodes.push_back(node);
				NodeIndex nodeIndex = static_cast<NodeIndex>(m_nodes.size() - 1);

				// qvflɕϊ
				for (UINT i = 0; i < assimpNode->mNumChildren; ++i)
				{
					m_nodes[nodeIndex].children[i] = FuncAssimpNodeConvert(
						assimpNode->mChildren[i], nodeIndex, DirectX::XMMatrixIdentity());
				}
				return nodeIndex;
			}
		};

	// m[h쐬
	m_nodes.clear();
	FuncAssimpNodeConvert(reinterpret_cast<const aiScene*>(ptr)->mRootNode, INDEX_NONE, DirectX::XMMatrixIdentity());

	// Aj[VvZ̈ɁAm[h̏f[^쐬
	Transform init = {
		DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)
	};
	for (int i = 0; i < MAX_TRANSFORM; ++i)
	{
		m_nodeTransform[i].resize(m_nodes.size(), init);
	}
}

void Model::MakeWeight(const void* ptr, int meshIdx)
{
	const aiScene* pScene = reinterpret_cast<const aiScene*>(ptr);

	// bVɑΉ{[邩
	aiMesh* assimpMesh = pScene->mMeshes[meshIdx];
	Mesh& mesh = m_meshes[meshIdx];
	if (assimpMesh->HasBones())
	{
		// bV̒_̈쐬
		struct WeightPair
		{
			unsigned int idx;
			float weight;
		};
		std::vector<std::vector<WeightPair>> weights;
		weights.resize(mesh.vertices.size());


		// bVɊ蓖ĂĂ{[̈m
		mesh.bones.resize(assimpMesh->mNumBones);
		for (auto boneIt = mesh.bones.begin(); boneIt != mesh.bones.end(); ++boneIt)
		{
			UINT boneIdx = static_cast<UINT>(boneIt - mesh.bones.begin());
			aiBone* assimpBone = assimpMesh->mBones[boneIdx];
			// \zς݂̃{[m[hYm[h擾
			std::string boneName = assimpBone->mName.data;
			auto nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(),
				[boneName](const Node& val) {
					return val.name == boneName;
				});
			// bVɊ蓖ĂĂ{[Am[hɑ݂Ȃ
			if (nodeIt == m_nodes.end())
			{
				boneIt->index = INDEX_NONE;
				continue;
			}

			// bṼ{[ƃm[h̕RÂ
			boneIt->index = static_cast<NodeIndex>(nodeIt - m_nodes.begin());
			boneIt->invOffset = GetMatrixFromAssimpMatrix(assimpBone->mOffsetMatrix);
			boneIt->invOffset.r[3].m128_f32[0] *= m_loadScale;
			boneIt->invOffset.r[3].m128_f32[1] *= m_loadScale;
			boneIt->invOffset.r[3].m128_f32[2] *= m_loadScale;
			boneIt->invOffset =
				DirectX::XMMatrixScaling(m_loadFlip == ZFlipUseAnime ? -1.0f : 1.0f, 1.0f, 1.0f) *
				boneIt->invOffset *
				DirectX::XMMatrixScaling(1.f / m_loadScale, 1.f / m_loadScale, 1.f / m_loadScale);

			// EFCg̐ݒ
			UINT weightNum = assimpBone->mNumWeights;
			for (UINT i = 0; i < weightNum; ++i)
			{
				aiVertexWeight weight = assimpBone->mWeights[i];
				weights[weight.mVertexId].push_back({ boneIdx, weight.mWeight });
			}
		}

		// 擾Ă_EFCgݒ
		for (int i = 0; i < weights.size(); ++i)
		{
			if (weights[i].size() >= 4)
			{
				std::sort(weights[i].begin(), weights[i].end(), [](WeightPair& a, WeightPair& b) {
					return a.weight > b.weight;
					});
				// EFCg4ɍ킹ĐK
				float total = 0.0f;
				for (int j = 0; j < 4; ++j)
					total += weights[i][j].weight;
				for (int j = 0; j < 4; ++j)
					weights[i][j].weight /= total;
			}
			for (int j = 0; j < weights[i].size() && j < 4; ++j)
			{
				mesh.vertices[i].index[j] = weights[i][j].idx;
				mesh.vertices[i].weight[j] = weights[i][j].weight;
			}
		}
	}
	else
	{
		// bV̐em[hgXtH[ƂČvZ
		std::string nodeName = assimpMesh->mName.data;
		auto nodeIt = std::find_if(m_nodes.begin(), m_nodes.end(),
			[nodeName](const Node& val) {
				return val.name == nodeName;
			});
		if (nodeIt == m_nodes.end())
		{
			return;	// {[f[^Ȃ
		}

		// bVłȂem[hċAT
		std::function<int(int)> FuncFindNode =
			[&FuncFindNode, this, pScene](NodeIndex parent)
			{
				std::string name = m_nodes[parent].name;
				for (UINT i = 0; i < pScene->mNumMeshes; ++i)
				{
					if (name == pScene->mMeshes[i]->mName.data)
					{
						return FuncFindNode(m_nodes[parent].parent);
					}
				}
				return parent;
			};

		Bone bone;
		bone.index = FuncFindNode(nodeIt->parent);
		bone.invOffset = DirectX::XMMatrixInverse(nullptr, m_nodes[bone.index].mat);
		for (auto vtxIt = mesh.vertices.begin(); vtxIt != mesh.vertices.end(); ++vtxIt)
		{
			vtxIt->weight[0] = 1.0f;
		}

		mesh.bones.resize(1);
		mesh.bones[0] = bone;
	}
}



bool Model::AnimeNoCheck(AnimeNo no)
{
	// pgbNAj[VmF
	if (no == PARAMETRIC_ANIME)
	{
		// pgbÑAj[Vݒ肳Ă邩
		return
			m_parametric[0] != ANIME_NONE &&
			m_parametric[1] != ANIME_NONE;
	}
	else
	{
		// ȂAj[Vԍǂ
		return 0 <= no && no < m_animes.size();
	}
}
void Model::InitAnime(AnimeNo no)
{
	// Aj̐ݒȂApgbNŐݒ肳ĂȂ珉Ȃ
	if (no == ANIME_NONE || no == PARAMETRIC_ANIME) { return; }

	Animation& anime = m_animes[no];
	anime.nowTime = 0.0f;
	anime.speed = 1.0f;
	anime.isLoop = false;
}
void Model::CalcAnime(AnimeTransform kind, AnimeNo no)
{
	Animation& anime = m_animes[no];
	Channels::iterator channelIt = anime.channels.begin();
	while (channelIt != anime.channels.end())
	{
		// v{[Ȃ΃XLbv
		Timeline& timeline = channelIt->timeline;
		if (channelIt->index == INDEX_NONE || timeline.empty())
		{
			++channelIt;
			continue;
		}

		//--- Ym[h̎pAj[VōXV
		Transform& transform = m_nodeTransform[kind][channelIt->index];
		if (timeline.size() <= 1)
		{
			// L[Ȃ̂Œl̂܂܎gp
			transform = channelIt->timeline.begin()->second;
		}
		else
		{
			Timeline::iterator startIt = timeline.begin();
			if (anime.nowTime <= startIt->first)
			{
				// 擪L[O̎ԂȂA擪̒lgp
				transform = startIt->second;
			}
			else if (timeline.rbegin()->first <= anime.nowTime)
			{
				// ŏIL[̎ԂȂAŌ̒lgp
				transform = timeline.rbegin()->second;
			}
			else
			{
				// w肳ꂽԂ2̃L[AԂꂽlvZ
				Timeline::iterator nextIt = timeline.upper_bound(anime.nowTime);
				startIt = nextIt;
				--startIt;
				float rate = (anime.nowTime - startIt->first) / (nextIt->first - startIt->first);
				LerpTransform(&transform, startIt->second, nextIt->second, rate);
			}
		}

		++channelIt;
	}
}
void Model::UpdateAnime(AnimeNo no, float tick)
{
	if (no == PARAMETRIC_ANIME) { return; }

	Animation& anime = m_animes[no];
	anime.nowTime += anime.speed * tick;
	if (anime.isLoop)
	{
		while (anime.nowTime >= anime.totalTime)
		{
			anime.nowTime -= anime.totalTime;
		}
	}
}
void Model::CalcBones(NodeIndex index, const DirectX::XMMATRIX parent)
{
	//--- Aj[VƂ̃p[^
	Transform transform;
	// pgbN
	if (m_playNo == PARAMETRIC_ANIME || m_blendNo == PARAMETRIC_ANIME)
	{
		LerpTransform(&transform, m_nodeTransform[PARAMETRIC0][index], m_nodeTransform[PARAMETRIC1][index], m_parametricBlend);
		if (m_playNo == PARAMETRIC_ANIME)
		{
			m_nodeTransform[MAIN][index] = transform;
		}
		if (m_blendNo == PARAMETRIC_ANIME)
		{
			m_nodeTransform[BLEND][index] = transform;
		}
	}
	// uhAj
	if (m_blendNo != ANIME_NONE)
	{
		LerpTransform(&transform, m_nodeTransform[MAIN][index], m_nodeTransform[BLEND][index], m_blendTime / m_blendTotalTime);
	}
	else
	{
		// CAĵ
		transform = m_nodeTransform[MAIN][index];
	}

	// Ym[h̎psvZ
	Node& node = m_nodes[index];
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&transform.translate));
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&transform.quaternion));
	DirectX::XMMATRIX S = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&transform.scale));
	node.mat = (S * R * T) * parent;

	// qvf̎pXV
	Children::iterator it = node.children.begin();
	while (it != node.children.end())
	{
		CalcBones(*it, node.mat);
		++it;
	}
}

void Model::LerpTransform(Transform* pOut, const Transform& a, const Transform& b, float rate)
{
	DirectX::XMVECTOR vec[][2] = {
		{ DirectX::XMLoadFloat3(&a.translate),	DirectX::XMLoadFloat3(&b.translate) },
		{ DirectX::XMLoadFloat4(&a.quaternion),	DirectX::XMLoadFloat4(&b.quaternion) },
		{ DirectX::XMLoadFloat3(&a.scale),		DirectX::XMLoadFloat3(&b.scale) },
	};
	for (int i = 0; i < 3; ++i)
	{
		vec[i][0] = DirectX::XMVectorLerp(vec[i][0], vec[i][1], rate);
	}
	DirectX::XMStoreFloat3(&pOut->translate, vec[0][0]);
	DirectX::XMStoreFloat4(&pOut->quaternion, vec[1][0]);
	DirectX::XMStoreFloat3(&pOut->scale, vec[2][0]);
}

bool Model::GetAnimatedTransform(DirectX::XMFLOAT3& outPos, DirectX::XMFLOAT3& outRot, DirectX::XMFLOAT3& outScale)
{
	// 1. XLjOfi{[j`FbN
	// {[ꍇ̓VF[_[(VS_ANIME)œ߁AłfalseԂďȂ
	for (const auto& mesh : m_meshes)
	{
		if (!mesh.bones.empty()) return false;
	}

	// 2. Aj[VĐĂ邩`FbN
	if (m_playNo == ANIME_NONE && m_blendNo == ANIME_NONE)
	{
		return false;
	}

	// 3. [gm[h(Index 0)̍s擾
	// Step()֐ m_nodes[0].mat XVĂOł
	if (m_nodes.empty()) return false;

	DirectX::XMMATRIX rootMat = m_nodes[0].mat;

	// 4. s𕪉 (Scale, Rotation, Translation)
	DirectX::XMVECTOR s, r, t;
	if (!DirectX::XMMatrixDecompose(&s, &r, &t, rootMat))
	{
		return false;
	}

	// 5. l̊i[
	DirectX::XMStoreFloat3(&outScale, s);
	DirectX::XMStoreFloat3(&outPos, t);

	// 6. NH[^jI -> IC[p(x) ϊ
	// TransformComponent́ux@vŊpxĂ̂ŕϊKvł
	DirectX::XMFLOAT4 q;
	DirectX::XMStoreFloat4(&q, r);

	// ʓIȕϊ (Singularity΍􍞂)
	float sqw = q.w * q.w;
	float sqx = q.x * q.x;
	float sqy = q.y * q.y;
	float sqz = q.z * q.z;
	float unit = sqx + sqy + sqz + sqw;
	float test = q.x * q.w - q.y * q.z;

	if (test > 0.4995f * unit) { // k (ٓ_)
		outRot.y = 2.0f * atan2f(q.y, q.x);
		outRot.x = DirectX::XM_PIDIV2;
		outRot.z = 0;
	}
	else if (test < -0.4995f * unit) { //  (ٓ_)
		outRot.y = -2.0f * atan2f(q.y, q.x);
		outRot.x = -DirectX::XM_PIDIV2;
		outRot.z = 0;
	}
	else {
		outRot.y = atan2f(2.0f * q.w * q.y + 2.0f * q.z * q.x, 1 - 2.0f * (sqx + sqy));
		outRot.x = asinf(2.0f * (q.w * q.x - q.y * q.z));
		outRot.z = atan2f(2.0f * q.w * q.z + 2.0f * q.x * q.y, 1 - 2.0f * (sqz + sqx));
	}

	// WA -> x֕ϊ
	outRot.x = DirectX::XMConvertToDegrees(outRot.x);
	outRot.y = DirectX::XMConvertToDegrees(outRot.y);
	outRot.z = DirectX::XMConvertToDegrees(outRot.z);

	return true;
}
