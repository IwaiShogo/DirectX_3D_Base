/*****************************************************************//**
 * @file	Entity.h
 * @brief	ECS�A�[�L�e�N�`���̃G���e�B�e�B��`
 * 
 * @details	Entity Component System (ECS) �A�[�L�e�N�`���ɂ�����
 *			�G���e�B�e�B�̊�{��`��񋟁B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/17	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ENTITY_H___
#define ___ENTITY_H___

// ===== �C���N���[�h =====
#include <cstdint>

/**
 * @struct	Entity
 * @brief	�Q�[�����E�ɑ��݂���I�u�W�F�N�g��\����ӂȎ��ʎq
 * 
 * @details	Entity��uint32_t�^��ID�ԍ��݂̂�ێ�����A���Ɍy�ʂȍ\���̂ł��B
 *			World�ɂ���ă��j�[�N��ID�����蓖�Ă��A����ID��ʂ��ăR���|�[�l���g�ɃA�N�Z�X���܂��B
 * 
 * @par		�g�p��F
 * @code
 *	// World w��World�N���X�̃C���X�^���X�Ƃ��܂�
 *	Entity player = w.Create()
 *		.With<Transform>()
 *		.Build();
 *	if (player.IsValid()) {
 *	// �G���e�B�e�B���L���ȏꍇ�̏���
 *  }
 * @endcode
 * 
 * @note	�G���e�B�e�B��ID�͎����I�Ɋ��蓖�Ă��邽�߁A���ڑ��삷��K�v�͂���܂���B
 * @warning	�G���e�B�e�B���폜����ۂ́A�K��World::DestroyEntity()���g�p���AID�̍ė��p�������s���Ă��������B
 */
struct Entity
{
	// @brief	�G���e�B�e�B����ӂɎ��ʂ���ID
	using ID = std::uint32_t;
	/**
	 * @var		id
	 * @brief	�G���e�B�e�B�̈�ӎ��ʔԍ�
	 * 
	 * @details	World�N���X�ɂ���Ď����I�Ɋ��蓖�Ă����ӂ�ID�B
	 *			����ID���g���āAWorld����R���|�[�l���g���擾�E�ǉ��E�폜����B
	 *			0�͖�����ID�������܂��B
	 * 
	 * @warning	ID�𒼐ڕύX���Ȃ����ƁB
	 */
	ID id = 0;

	/**
	 * [bool - IsValid]
	 * @brief	�G���e�B�e�B���L����ID�������Ă��邩�𔻒�
	 * 
	 * @return	true.�L�� false.����
	 */
	bool IsValid() const { return id != 0; }

	// --- ��r���Z�q�̃I�[�o�[���[�h ---
	bool operator ==(const Entity& other) const { return id == other.id; }
	bool operator !=(const Entity& other) const { return id != other.id; }
};

#endif // !___ENTITY_H___