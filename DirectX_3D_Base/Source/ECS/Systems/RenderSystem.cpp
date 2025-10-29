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
 * @brief �J�����ݒ�ƃf�o�b�O�`����s��
 */
void RenderSystem::DrawSetup()
{
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
	// 1. CameraComponent������Entity���������A�J�������W�ƍs����擾
	ECS::EntityID cameraID = ECS::INVALID_ENTITY_ID;

	// Coordinator�̑SEntity�𑖍� (����������m��)
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

        // ������ 2. Geometory�ɍs��ƃJ�����ʒu��ݒ� ������  
        Geometory::SetView(cameraComp->ViewMatrix);  
        Geometory::SetProjection(cameraComp->ProjectionMatrix);  
    }
	if (cameraID != ECS::INVALID_ENTITY_ID)
	{
		*cameraComp = m_coordinator->GetComponent<CameraComponent>(cameraID);

		// ������ 2. Geometory�ɍs��ƃJ�����ʒu��ݒ� ������
		Geometory::SetView(cameraComp->ViewMatrix);
		Geometory::SetProjection(cameraComp->ProjectionMatrix);
	}

	// System���ێ�����Entity�Z�b�g���C�e���[�g
	for (auto const& entity : m_entities)
	{
		// Component�������Ɏ擾
		TransformComponent& transform = m_coordinator->GetComponent<TransformComponent>(entity);
		RenderComponent& render = m_coordinator->GetComponent<RenderComponent>(entity);

		DirectX::XMFLOAT4X4 wvp[3];
		DirectX::XMMATRIX world, view, proj;

		// 1. ���[���h�s��̌v�Z (TransformComponent -> XMMATRIX -> XMFLOAT4X4)
		// ... (�X�P�[���A��]�A���s�ړ��̌v�Z���W�b�N�͈ێ�) ...
		XMMATRIX S = XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z);
		XMMATRIX Rx = XMMatrixRotationX(transform.Rotation.x);
		XMMATRIX Ry = XMMatrixRotationY(transform.Rotation.y);
		XMMATRIX Rz = XMMatrixRotationZ(transform.Rotation.z);
		XMMATRIX R = Rz * Rx * Ry;
		XMMATRIX T = XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);
		world = S * R * T;

		// �r���[�s��

		XMStoreFloat4x4(&wvp[0], XMMatrixTranspose(world));
		wvp[1] = cameraComp->ViewMatrix;
		wvp[2] = cameraComp->ProjectionMatrix;

		// �V�F�[�_�[�ւ̕ϊ��s���ݒ�
		Geometory::SetWorld(wvp[0]);
		Geometory::SetView(wvp[1]);
		Geometory::SetProjection(wvp[2]);

		ShaderList::SetWVP(wvp);
		// 2. �`�揈�� (RenderComponent)

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
		{
			// ModelComponent���擾���AModel::Draw()���Ăяo��
			// RenderSystem�̃V�O�l�`����ModelComponent���܂܂�Ă���O��
			ModelComponent& model = m_coordinator->GetComponent<ModelComponent>(entity);
			if (model.pModel)
			{
				model.pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
				model.pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

				for (int i = 0; i < model.pModel->GetMeshNum(); ++i) {
					// ���f���̃��b�V�����擾
					Model::Mesh mesh = *model.pModel->GetMesh(i);
					// ���b�V���Ɋ��蓖�Ă��Ă���}�e���A�����擾
					Model::Material	material = *model.pModel->GetMaterial(mesh.materialID);
					// �V�F�[�_�[�փ}�e���A����ݒ�
					ShaderList::SetMaterial(material);
					// ���f���̕`��
					model.pModel->Draw(i);
				}

			}
		}
			break;
		case MESH_NONE:
			// �������Ȃ�
			break;
		}
	}
}