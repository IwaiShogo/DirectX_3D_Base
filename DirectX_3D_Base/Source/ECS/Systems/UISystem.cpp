/*****************************************************************//**
 * @file	UISystem.cpp
 * @brief	UISystem�̎����BUIComponent������Entity��2D�`����s���B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FUIComponent�Ɋ�Â��ASprite::Draw���g�p����UI��`�悷�郍�W�b�N�������B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#include "ECS/Systems/UISystem.h"
#include "Systems/DirectX/DirectX.h" // SetDepthTest�֐����g�p

using namespace DirectX;

/**
 * @brief UI�v�f��`�悷��
 */
void UISystem::Draw()
{
	// 1. UI�`��̂��߂̏����i�[�x�e�X�g�̖������j
	SetDepthTest(false);

	// System���ێ�����Entity�Z�b�g���C�e���[�g
	// UI�`��̌������̂��߁ADepth�l�Ń\�[�g���Ă���`�悷�邱�Ƃ���������邪�A�����ł͎����̒P�����̂��ߏȗ��B
	for (auto const& entity : m_entities)
	{
		UIComponent& uiComp = m_coordinator->GetComponent<UIComponent>(entity);

		// Sprite API���g�p���ĕ`��
		// Sprite::Draw(�e�N�X�`��ID, ��ʈʒu, �T�C�Y, �`��J���[, �[�x)
		/*Sprite::Draw(
			uiComp.TextureID,
			uiComp.Position.x,
			uiComp.Position.y,
			uiComp.Size.x,
			uiComp.Size.y,
			uiComp.Color.x,
			uiComp.Color.y,
			uiComp.Color.z,
			uiComp.Color.w);*/

		// �� Sprite::Draw��API�V�O�l�`���̓v���W�F�N�g��Sprite.h�Ɉˑ����܂��B
		// �����ł́A��ʓI�Ȉ����iTextureID, x, y, width, height, r, g, b, a�j��z�肵�Ă��܂��B
	}

	// 2. UI�`��̏I�������i�[�x�e�X�g�̍ėL�����j
	SetDepthTest(true);
}