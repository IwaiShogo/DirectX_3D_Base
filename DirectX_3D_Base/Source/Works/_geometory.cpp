#include "../Systems/Geometory.h"

void Geometory::MakeBox()
{
	//--- ���_�̍쐬
	Vertex vtx[] = {
		// -Z��
		{{ -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }},
		{{  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f }},
		{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f }},
		{{  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f }},
	};

	//--- �C���f�b�N�X�̍쐬
	int idx[] = {
		0,1,2, 1,3,2
	};

	// �o�b�t�@�̍쐬
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
	//--- ���_�̍쐬
	// �V�ʁA���

	// ����

	//--- �C���f�b�N�X�̍쐬
	// �V�ʁA���

	// ����


	//--- �o�b�t�@�̍쐬
}

void Geometory::MakeSphere()
{
	//--- ���_�̍쐬

	//--- �C���f�b�N�X�̍쐬

	// �o�b�t�@�̍쐬
}