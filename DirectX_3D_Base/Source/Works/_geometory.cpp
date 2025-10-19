#include "../Systems/Geometory.h"

void Geometory::MakeBox()
{
	//--- 頂点の作成
	Vertex vtx[] = {
		// -Z面
		{{ -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }},
		{{  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f }},
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f }},
	};

	//--- インデックスの作成
	int idx[] = {
		0,1,2, 1,3,2
	};

	// バッファの作成
	MeshBuffer::Description desc = {};
	desc.pVtx = vtx;
	desc.vtxCount = 4;
	desc.vtxSize	= 20;	// sizeof(Vertex)
	desc.pIdx = idx;
	desc.idxCount = 6;
	desc.idxSize = 4;	// sizeof(int)
	desc.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_pBox = new MeshBuffer();
	m_pBox->Create(desc);

}

void Geometory::MakeCylinder()
{
	//--- 頂点の作成
	// 天面、底面

	// 側面

	//--- インデックスの作成
	// 天面、底面

	// 側面


	//--- バッファの作成
}

void Geometory::MakeSphere()
{
	//--- 頂点の作成

	//--- インデックスの作成

	// バッファの作成
}