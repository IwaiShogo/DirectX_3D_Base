/*****************************************************************//**
 * @file	RenderComponent.h
 * @brief	Entity�̕`����@�Ɋւ�������`����Component�B
 * 
 * @details	
 * �`�悷��`��̎�ށi�{�b�N�X�A�X�t�B�A�Ȃǁj��A�F���A�e�N�X�`��ID�Ȃǂ�ێ�����B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�`��ɕK�v�Ȍ`��ƐF��ێ����� `RenderComponent` ���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___RENDER_COMPONENT_H___
#define ___RENDER_COMPONENT_H___

#include <DirectXMath.h>
#include <cstdint>

 /**
  * @enum MeshType
  * @brief �`�悷�郁�b�V���̌`������ʂ��邽�߂̗񋓌^�B
  */
enum MeshType : uint8_t
{
	MESH_BOX,			///< ���i�����́j
	MESH_SPHERE,		///< ��
	MESH_MODEL,			///< �O���t�@�C������̃��[�h���f�� (������)
	MESH_NONE,			///< �`����s��Ȃ��i�f�o�b�O�p�Ȃǁj
};

/**
 * @struct RenderComponent
 * @brief Entity�̊O�ρi�`��A�F�A�e�N�X�`���j�Ɋւ���f�[�^
 */
struct RenderComponent
{
	MeshType Type;				///< �`�悷�郁�b�V���̌`��
	DirectX::XMFLOAT4 Color;	///< �`�掞�̐F (R, G, B, A)

	/**
	 * @brief �R���X�g���N�^
	 * @param type - ���b�V���`��
	 * @param color - �`��F
	 */
	RenderComponent(
		MeshType type = MESH_BOX,
		DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
	)
		: Type(type)
		, Color(color)
	{}
};

#endif // !___RENDER_COMPONENT_H___