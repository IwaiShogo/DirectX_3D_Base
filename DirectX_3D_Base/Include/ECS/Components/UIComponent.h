/*****************************************************************//**
 * @file	UIComponent.h
 * @brief	HUD�⃁�j���[�ȂǁA2D UI�̕`��ɕK�v�ȏ����`����Component�B
 * 
 * @details	
 * ��ʏ�̈ʒu�A�T�C�Y�A�`�悷��e�N�X�`��ID�Ȃǂ�ێ�����
 *
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/28	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FUI�v�f�̈ʒu�A�T�C�Y�A�e�N�X�`��ID��ێ����� `UIComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___UI_COMPONENT_H___
#define ___UI_COMPONENT_H___

#include <DirectXMath.h>
#include <cstdint>

 /**
  * @struct UIComponent
  * @brief 2D UI�v�f�̕`����
  */
struct UIComponent
{
	uint32_t TextureID;					///< �`�悷��e�N�X�`���̃��\�[�XID
	DirectX::XMFLOAT2 Position;			///< ��ʏ�̒��S���W (�s�N�Z���P�ʁA��: 0, 0������)
	DirectX::XMFLOAT2 Size;				///< �`��T�C�Y (�s�N�Z���P��)
	DirectX::XMFLOAT4 Color;			///< �`�掞�̐F/��Z�J���[ (RGBA)
	float Depth;						///< �`��[�x (0.0f�`1.0f)

	/**
	 * @brief �R���X�g���N�^
	 */
	UIComponent(
		uint32_t textureId = 0,
		DirectX::XMFLOAT2 position = DirectX::XMFLOAT2(0.0f, 0.0f),
		DirectX::XMFLOAT2 size = DirectX::XMFLOAT2(100.0f, 100.0f),
		DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		float depth = 0.5f
	) : TextureID(textureId), Position(position), Size(size), Color(color), Depth(depth)
	{
	}
};

#endif // !___UI_COMPONENT_H___