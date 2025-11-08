#include "Systems/Geometory.h"

void Geometory::MakeBox()
{
	float hs = 1.0f * 0.5f;
	//--- 頂点の作成
	Vertex vtx[] = {
		// 1. -Z面 (手前)
		{{ -hs,  hs, -hs }, { 0.0f, 0.0f }}, // 0: 左上
		{{  hs,  hs, -hs }, { 1.0f, 0.0f }}, // 1: 右上
		{{ -hs, -hs, -hs }, { 0.0f, 1.0f }}, // 2: 左下
		{{  hs, -hs, -hs }, { 1.0f, 1.0f }}, // 3: 右下

		// 2. +Z面 (奥)
		{{  hs,  hs,  hs }, { 0.0f, 0.0f }}, // 4: 右上
		{{ -hs,  hs,  hs }, { 1.0f, 0.0f }}, // 5: 左上
		{{  hs, -hs,  hs }, { 0.0f, 1.0f }}, // 6: 右下
		{{ -hs, -hs,  hs }, { 1.0f, 1.0f }}, // 7: 左下

		// 3. -X面 (左)
		{{ -hs,  hs,  hs }, { 0.0f, 0.0f }}, // 8: 左上 (Z+)
		{{ -hs,  hs, -hs }, { 1.0f, 0.0f }}, // 9: 右上 (Z-)
		{{ -hs, -hs,  hs }, { 0.0f, 1.0f }}, // 10: 左下 (Z+)
		{{ -hs, -hs, -hs }, { 1.0f, 1.0f }}, // 11: 右下 (Z-)

		// 4. +X面 (右)
		{{  hs,  hs, -hs }, { 0.0f, 0.0f }}, // 12: 左上 (Z-)
		{{  hs,  hs,  hs }, { 1.0f, 0.0f }}, // 13: 右上 (Z+)
		{{  hs, -hs, -hs }, { 0.0f, 1.0f }}, // 14: 左下 (Z-)
		{{  hs, -hs,  hs }, { 1.0f, 1.0f }}, // 15: 右下 (Z+)

		// 5. +Y面 (上)
		{{ -hs,  hs,  hs }, { 0.0f, 0.0f }}, // 16: 左奥 (X-)
		{{  hs,  hs,  hs }, { 1.0f, 0.0f }}, // 17: 右奥 (X+)
		{{ -hs,  hs, -hs }, { 0.0f, 1.0f }}, // 18: 手前左 (X-)
		{{  hs,  hs, -hs }, { 1.0f, 1.0f }}, // 19: 手前右 (X+)

		// 6. -Y面 (下)
		{{ -hs, -hs, -hs }, { 0.0f, 0.0f }}, // 20: 手前左 (X-)
		{{  hs, -hs, -hs }, { 1.0f, 0.0f }}, // 21: 手前右 (X+)
		{{ -hs, -hs,  hs }, { 0.0f, 1.0f }}, // 22: 奥左 (X-)
		{{  hs, -hs,  hs }, { 1.0f, 1.0f }}, // 23: 奥右 (X+)
	};

	//--- インデックスの作成
	int idx[] = {
		// 1. -Z面 (頂点 0-3)
		 0,  1,  2,    1,  3,  2,
		// 2. +Z面 (頂点 4-7)
		 4,  5,  6,    5,  7,  6,
		// 3. -X面 (頂点 8-11)
		 8,  9, 10,    9, 11, 10,
		// 4. +X面 (頂点 12-15)
		12, 13, 14,   13, 15, 14,
		// 5. +Y面 (頂点 16-19)
		16, 17, 18,   17, 19, 18,
		// 6. -Y面 (頂点 20-23)
		20, 21, 22,   21, 23, 22,
	};

	// バッファの作成
	MeshBuffer::Description desc = {};
	desc.pVtx		= vtx;
	desc.vtxCount	= 24;
	desc.vtxSize	= 20;	// sizeof(Vertex)
	desc.pIdx		= idx;
	desc.idxCount	= 36;
	desc.idxSize	= 4;	// sizeof(int)
	desc.topology	= D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_pBox			= new MeshBuffer();
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