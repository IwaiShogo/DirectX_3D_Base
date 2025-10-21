#include "../Systems/Geometory.h"

void Geometory::MakeBox()
{
	float hs = 1.0f * 0.5f;
	//--- ���_�̍쐬
	Vertex vtx[] = {
		// 1. -Z�� (��O)
		{{ -hs,  hs, -hs }, { 0.0f, 0.0f }}, // 0: ����
		{{  hs,  hs, -hs }, { 1.0f, 0.0f }}, // 1: �E��
		{{ -hs, -hs, -hs }, { 0.0f, 1.0f }}, // 2: ����
		{{  hs, -hs, -hs }, { 1.0f, 1.0f }}, // 3: �E��

		// 2. +Z�� (��)
		{{  hs,  hs,  hs }, { 0.0f, 0.0f }}, // 4: �E��
		{{ -hs,  hs,  hs }, { 1.0f, 0.0f }}, // 5: ����
		{{  hs, -hs,  hs }, { 0.0f, 1.0f }}, // 6: �E��
		{{ -hs, -hs,  hs }, { 1.0f, 1.0f }}, // 7: ����

		// 3. -X�� (��)
		{{ -hs,  hs,  hs }, { 0.0f, 0.0f }}, // 8: ���� (Z+)
		{{ -hs,  hs, -hs }, { 1.0f, 0.0f }}, // 9: �E�� (Z-)
		{{ -hs, -hs,  hs }, { 0.0f, 1.0f }}, // 10: ���� (Z+)
		{{ -hs, -hs, -hs }, { 1.0f, 1.0f }}, // 11: �E�� (Z-)

		// 4. +X�� (�E)
		{{  hs,  hs, -hs }, { 0.0f, 0.0f }}, // 12: ���� (Z-)
		{{  hs,  hs,  hs }, { 1.0f, 0.0f }}, // 13: �E�� (Z+)
		{{  hs, -hs, -hs }, { 0.0f, 1.0f }}, // 14: ���� (Z-)
		{{  hs, -hs,  hs }, { 1.0f, 1.0f }}, // 15: �E�� (Z+)

		// 5. +Y�� (��)
		{{ -hs,  hs,  hs }, { 0.0f, 0.0f }}, // 16: ���� (X-)
		{{  hs,  hs,  hs }, { 1.0f, 0.0f }}, // 17: �E�� (X+)
		{{ -hs,  hs, -hs }, { 0.0f, 1.0f }}, // 18: ��O�� (X-)
		{{  hs,  hs, -hs }, { 1.0f, 1.0f }}, // 19: ��O�E (X+)

		// 6. -Y�� (��)
		{{ -hs, -hs, -hs }, { 0.0f, 0.0f }}, // 20: ��O�� (X-)
		{{  hs, -hs, -hs }, { 1.0f, 0.0f }}, // 21: ��O�E (X+)
		{{ -hs, -hs,  hs }, { 0.0f, 1.0f }}, // 22: ���� (X-)
		{{  hs, -hs,  hs }, { 1.0f, 1.0f }}, // 23: ���E (X+)
	};

	//--- �C���f�b�N�X�̍쐬
	int idx[] = {
		// 1. -Z�� (���_ 0-3)
		 0,  1,  2,    1,  3,  2,
		// 2. +Z�� (���_ 4-7)
		 4,  5,  6,    5,  7,  6,
		// 3. -X�� (���_ 8-11)
		 8,  9, 10,    9, 11, 10,
		// 4. +X�� (���_ 12-15)
		12, 13, 14,   13, 15, 14,
		// 5. +Y�� (���_ 16-19)
		16, 17, 18,   17, 19, 18,
		// 6. -Y�� (���_ 20-23)
		20, 21, 22,   21, 23, 22,
	};

	// �o�b�t�@�̍쐬
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