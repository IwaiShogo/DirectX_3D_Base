/*****************************************************************//**
 * @file	World.h
 * @brief	ECS���[���h�Ǘ��V�X�e���ƃG���e�B�e�B�r���_�[�̒�`
 * 
 * @details	���̃t�@�C���́AECS�A�[�L�e�N�`���̒��j�ƂȂ�World�N���X�ƁA
 *			�G���e�B�e�B���ȒP�ɍ쐬���邽�߂�EntityBuilder�N���X���`���܂��B
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

#ifndef ___WORLD_H___
#define ___WORLD_H___

// ===== �C���N���[�h =====
#include "Entity.h"
#include "Component.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

// ===== �O���錾 =====
class World;

// --------------------------------------------------
// 1. �R���|�[�l���g�X�g�A�̊�� (IStore, Store<T>)
// --------------------------------------------------

/**
 * @struct	IStore
 * @brief	�S�ẴR���|�[�l���g�X�g�A�̒��ۊ��N���X
 * 
 * @details	World�N���X���^���ӎ������Ɂistd::type_index�Łj�����
 *			�R���|�[�l���g�X�g�A�ւ̃|�C���^��ێ��ł���悤��
 *			���邽�߂̃C���^�[�t�F�[�X�ł��B
 */
struct IStore
{
	virtual ~IStore() = default;

	/**
	 * [void - RemoveComponent]
	 * @brief	�G���e�B�e�B���j�����ꂽ�ہA���̃R���|�[�l���g���X�g�A����폜����
	 * 
	 * @param	[in] e	�폜�Ώۂ̃G���e�B�e�B 
	 */
	virtual void RemoveComponent(Entity e) = 0;
};

#endif // !___WORLD_H___