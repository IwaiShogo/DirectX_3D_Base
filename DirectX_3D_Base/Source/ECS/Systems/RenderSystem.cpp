/*****************************************************************//**
 * @file	RenderSystem.cpp
 * @brief	TransformComponent��RenderComponent������Entity��`�悷��System�̎����B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FRenderSystem�̕`�惍�W�b�N�������BMain.cpp����f���`��ƃJ�����ݒ���ڊǁB
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#include "ECS/Systems/RenderSystem.h"
#include "Main.h"
#include "Systems/DirectX/DirectX.h"
#include "Systems/Geometory.h"
#include "Systems/Input.h"
#include <iostream>

using namespace DirectX;

// ===== �`�惊�\�[�X�̐ÓI��` =====
namespace RenderResource
{
	// ���O�Ƀ��[�h���郂�f���̃��\�[�XID
	static uint32_t PlayerModelID = 0;
	static uint32_t GroundModelID = 0;
	// ���O�Ƀ��[�h����e�N�X�`���̃��\�[�XID
	static uint32_t NatureTexID = 0;
}

void RenderSystem::Init()
{
	m_coordinator = GameScene::GetCoordinator();

	// --- 3D���f���ƃe�N�X�`���̃��[�h ---
	// �񋟂��ꂽAssets����A�n�ʂƃv���C���[�ɗ��p�ł������ȃ��f�������[�h

	// ���f���V�X�e��API (Model.h/cpp�ɑ��݂���z��) �𗘗p���ă��f���ƃe�N�X�`�������[�h
	// Model::Init()��Main.cpp��InitDirectX��ŌĂяo���ς݂Ɖ���A�����ł̓��\�[�X�����[�h����

	// �n�ʃ��f�������[�h
	//
	//RenderResource::GroundModelID = Model::LoadModel(ASSET("Model/LowPolyNature/Ground_01.fbx"));

	//// �v���C���[���f��������Rock_01.fbx�ő��
	////
	//RenderResource::PlayerModelID = Model::LoadModel(ASSET("Model/LowPolyNature/Rock_01.fbx"));

	//// ���ʃe�N�X�`�������[�h
	////
	//RenderResource::NatureTexID = Texture::LoadTexture(ASSET("Model/LowPolyNature/Nature_Texture_01.png"));
}

/**
 * @brief �J�����ݒ�ƃf�o�b�O�`����s��
 */
void RenderSystem::DrawSetup()
{
	// ***** Main.cpp����ڐA�����J�����ƃO���b�h�̕`�惍�W�b�N *****

#ifdef _DEBUG
	// �����̕\��
	// �O���b�h
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
	// ��
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(size, 0, 0), XMFLOAT4(1, 0, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, size, 0), XMFLOAT4(0, 1, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, size), XMFLOAT4(0, 0, 1, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(-size, 0, 0), XMFLOAT4(0, 0, 0, 1));
	Geometory::AddLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, -size), XMFLOAT4(0, 0, 0, 1));

	Geometory::DrawLines();

	// �J�����̒l
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

	//// �W�I���g���p�J����������
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
 * @brief TransformComponent��RenderComponent�����S�Ă�Entity��`�悷��
 */
void RenderSystem::DrawEntities()
{
	// System���ێ�����Entity�Z�b�g���C�e���[�g
	for (auto const& entity : m_entities)
	{
		// Component�������Ɏ擾
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		// 1. ���[���h�s��̌v�Z (TransformComponent -> XMMATRIX -> XMFLOAT4X4)

		// �X�P�[���s��
		XMMATRIX S = XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z);

		// ��]�s�� (Z -> X -> Y �̏��œK�p)
		XMMATRIX Rx = XMMatrixRotationX(transform.Rotation.x);
		XMMATRIX Ry = XMMatrixRotationY(transform.Rotation.y);
		XMMATRIX Rz = XMMatrixRotationZ(transform.Rotation.z);
		XMMATRIX R = Rz * Rx * Ry;

		// ���s�ړ��s��
		XMMATRIX T = XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);

		// ���[���h�s��: S * R * T (�X�P�[�� -> ��] -> �ړ�)
		XMMATRIX worldMatrix = S * R * T;

		// DirectX�֓n�����߂ɓ]�u���Ċi�[
		XMFLOAT4X4 fMat;
		XMStoreFloat4x4(&fMat, XMMatrixTranspose(worldMatrix));

		// 2. �`�揈�� (RenderComponent)

		// �s���ݒ�
		Geometory::SetWorld(fMat);

		// �`��ɉ����ĕ`��
		switch (render.Type)
		{
		case MESH_BOX:
			Geometory::DrawBox();
			break;
		case MESH_SPHERE:
			Geometory::DrawBox(); // ��ւƂ���Box��`��
			break;
		case MESH_MODEL:
			// ModelSystem�Ɉڍs��A����
			break;
		case MESH_NONE:
			// �������Ȃ�
			break;
		}
	}
}