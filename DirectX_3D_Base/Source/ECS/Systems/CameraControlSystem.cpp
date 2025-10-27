/*****************************************************************//**
 * @file	CameraControlSystem.cpp
 * @brief	CameraControlSystem�̎����B�J�����̒Ǐ]�ƃr���[�s��̍X�V���s���B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�v���C���[��TransformComponent�Ɋ�Â��A�J�����ʒu���Ԃ��A�r���[�E�v���W�F�N�V�����s���Geometory�V�X�e���ɐݒ肷�郍�W�b�N�������B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#include "ECS/Systems/CameraControlSystem.h"
#include "Systems/Geometory.h" // �J�����ݒ�֐����g�p

using namespace DirectX;

/**
 * @brief 2�̈ʒu����`��Ԃ��� (Lerp)
 * @param start - �J�n�ʒu
 * @param end - �I���ʒu
 * @param t - ��ԌW�� (0.0f�`1.0f)
 * @return XMFLOAT3 - ��Ԃ��ꂽ�ʒu
 */
static XMFLOAT3 Lerp(const XMFLOAT3& start, const XMFLOAT3& end, float t)
{
	t = std::min(1.0f, std::max(0.0f, t)); // t��0�`1�ɃN�����v
	XMFLOAT3 result;
	result.x = start.x + (end.x - start.x) * t;
	result.y = start.y + (end.y - start.y) * t;
	result.z = start.z + (end.z - start.z) * t;
	return result;
}

/**
 * @brief �J�����̈ʒu���v�Z���A�r���[�E�v���W�F�N�V�����s���ݒ肷��
 */
void CameraControlSystem::Update()
{
	// CameraComponent������Entity�͒ʏ�1�Ƒz��
	for (auto const& entity : m_entities)
	{
		CameraComponent& cameraComp = m_coordinator->GetComponent<CameraComponent>(entity);

		ECS::EntityID focusID = cameraComp.FocusEntityID;

		// �Ǐ]�Ώۂ�Entity�����݂��ATransform�������Ă��邩�`�F�b�N
		if (focusID != ECS::INVALID_ENTITY_ID &&
			m_coordinator->m_entityManager->GetSignature(focusID).test(m_coordinator->GetComponentTypeID<TransformComponent>()))
		{
			// �Ǐ]�Ώۂ�TransformComponent���擾
			TransformComponent& focusTrans = m_coordinator->GetComponent<TransformComponent>(focusID);

			// 1. �ڕW�J�����ʒu (Target Camera Position) �̌v�Z
			// �^�[�Q�b�g�̈ʒu + �I�t�Z�b�g
			XMFLOAT3 targetPos;
			targetPos.x = focusTrans.Position.x + cameraComp.Offset.x;
			targetPos.y = focusTrans.Position.y + cameraComp.Offset.y;
			targetPos.z = focusTrans.Position.z + cameraComp.Offset.z;

			// 2. �J�����ʒu�̕�� (Smooth Following)
			// ���݈ʒu����ڕW�ʒu�֌������Ċɂ₩�Ɉړ�
			m_currentCameraPos = Lerp(m_currentCameraPos, targetPos, cameraComp.FollowSpeed);

			// 3. �����_�iLookAt�j�̕��
			// �Ǐ]�Ώۂ̈ʒu���V���������_
			XMFLOAT3 targetLookAt = focusTrans.Position;
			m_currentLookAt = Lerp(m_currentLookAt, targetLookAt, cameraComp.FollowSpeed);
		}

		// 4. �r���[�s��̌v�Z�Ɛݒ� (RenderSystem�̖�����������)

		XMVECTOR camPos = XMLoadFloat3(&m_currentCameraPos);
		XMVECTOR lookAt = XMLoadFloat3(&m_currentLookAt);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // Y-up

		// �r���[�s��
		XMFLOAT4X4 matView;
		XMStoreFloat4x4(&matView, XMMatrixTranspose(
			XMMatrixLookAtLH(camPos, lookAt, up)));
		Geometory::SetView(matView);

		// �v���W�F�N�V�����s��̌v�Z�Ɛݒ� (RenderSystem�̖�����������)
		XMFLOAT4X4 matProj;
		XMStoreFloat4x4(&matProj, XMMatrixTranspose(
			XMMatrixPerspectiveFovLH(
				cameraComp.FOV,
				(float)SCREEN_WIDTH / SCREEN_HEIGHT,
				cameraComp.NearClip,
				cameraComp.FarClip)
		));
		Geometory::SetProjection(matProj);
	}
}