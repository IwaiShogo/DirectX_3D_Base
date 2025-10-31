/*****************************************************************//**
 * @file	ECSInitializer.h
 * @brief	ECS�V�X�e���S�̂̏��������W�񂵁A�V�[����Init()����Ӗ��𕪗����邽�߂̃w���p�[�N���X�B
 * 
 * @details	
 * ECS�V�X�e���S�̂̏��������i�R���|�[�l���g / �V�X�e���o�^�A�V�O�l�`���ݒ�j
 * ���̃N���X�͐ÓI���\�b�h�݂̂������A�C���X�^���X������܂���B
 * �ǂ̃V�[���ł���т���ECS�\�����\�z���邽�߂ɗ��p����܂��B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS�̏��������W�b�N�𕪗����邽�߂̃N���X���쐬�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___ECS_INITIALIZER_H___
#define ___ECS_INITIALIZER_H___

// ===== �C���N���[�h =====
#include "ECS/Coordinator.h"

#include <memory>

namespace ECS
{
	/**
	 * @class ECSInitializer
	 * @brief Coordinator���󂯎��AECS�̑S�̍\����ݒ肷��ÓI�w���p�[
	 */
	class ECSInitializer final
	{
	public:
		// �O������Coordinator�|�C���^���󂯎��A�����������s
		static void InitECS(std::shared_ptr<Coordinator>& coordinator);

		// �O������Coordinator�|�C���^���󂯎��A�j�����������s (Scene::Uninit()����Ă΂��)
		static void UninitECS();

		/**
		 * @brief �o�^�ς�System�C���X�^���X�ւ�SharedPtr���擾����B
		 * @tparam T - �擾������System�̌^
		 * @return std::shared_ptr<T> - System�C���X�^���X��SharedPtr
		 */
		template<typename T>
		static std::shared_ptr<T> GetSystem()
		{
			// �^��System�̃T�u�N���X�ł��邱�Ƃ�ۏ؂���A�T�[�V�����Ȃǂ�����Ɨǂ�
			auto it = s_systems.find(std::type_index(typeid(T)));
			if (it != s_systems.end())
			{
				// �}�b�v����擾����System���N���X��SharedPtr���A�ړI�̌^�Ƀ_�E���L���X�g���ĕԂ�
				return std::static_pointer_cast<T>(it->second);
			}
			return nullptr;
		}

	private:
		// �R���|�[�l���g�̓o�^�݂̂��s��
		static void RegisterComponents(Coordinator* coordinator);

		// �V�X�e���o�^���ASharedPtr��ÓI�}�b�v�Ɋi�[����悤�ɕύX
		static void RegisterSystemsAndSetSignatures(Coordinator* coordinator);

	private:
		// �o�^���ꂽ�S�V�X�e���C���X�^���X��ێ�����ÓI�}�b�v
		// System�̌^���L�[�Ƃ��ASharedPtr��l�Ƃ���
		static std::unordered_map<std::type_index, std::shared_ptr<System>> s_systems;
	};
}

#endif // !___ECS_INITIALIZER_H___