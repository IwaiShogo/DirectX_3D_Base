/*****************************************************************//**
 * @file	SceneManager.h
 * @brief	�Q�[���V�[���̊Ǘ��A�؂�ւ��A���C�t�T�C�N��������s���ÓI�N���X�B
 * 
 * @details	
 * Title, Game, Result�ȂǁA�S�Ă�Scene�𒊏ۊ��N���X`Scene`�Ƃ��Ĉ����A
 * �J�ڗv�������S�ɏ�������B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F�V�[���̊Ǘ��E�J�ڋ@�\������ `SceneManager` �ÓI�N���X���`�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___SCENE_MANAGER_H___
#define ___SCENE_MANAGER_H___

#include <memory> // std::unique_ptr�p
#include <typeindex> // �V�[���̌^�����ʂ��邽��
#include <functional> // �V�[���̃t�@�N�g���֐��p
#include <unordered_map>

#include "Scene.h"
#include "GameScene.h"

 /**
  * @class SceneManager
  * @brief �V�[���̓o�^�A�؂�ւ��A���s�𓝊�����ÓI�}�l�[�W���N���X
  */
class SceneManager
{
private:
	// ���ݎ��s���̃V�[���̃C���X�^���X
	static std::unique_ptr<Scene> s_currentScene;

	// ���ɐ؂�ւ���V�[���̃t�@�N�g���֐��inullptr�̏ꍇ�͐؂�ւ��Ȃ��j
	static std::function<Scene* ()> s_nextSceneFactory;

	// �V�[���𐶐����邽�߂̃t�@�N�g���֐��̃}�b�v�i�^ID -> �t�@�N�g���֐��j
	static std::unordered_map<std::type_index, std::function<Scene* ()>> s_sceneFactories;

	// @brief ���݂̃V�[����j�����A���̃V�[���𐶐��E�����������������
	static void ProcessSceneChange();

public:
	SceneManager() = delete; // �ÓI�N���X�̂��߃C���X�^���X�����֎~

	/// @brief �V�[���}�l�[�W���̏���������
	static void Init();

	/// @brief �V�[���}�l�[�W���̏I�������i���݂̃V�[����j���j
	static void Uninit();

	/// @brief ���t���[���̍X�V�����B�V�[���؂�ւ��������܂ށB
	static void Update(float deltaTime);

	/// @brief ���t���[���̕`�揈��
	static void Draw();

	/**
	 * @brief �V�����V�[���^��SceneManager�ɓo�^����
	 * @tparam T - Scene���N���X���p�������V�[���N���X
	 */
	template<typename T>
	static void RegisterScene()
	{
		std::type_index type = std::type_index(typeid(T));
		s_sceneFactories[type] = []() -> Scene* { return new T(); };
	}

	/**
	 * @brief �V�[���̐؂�ւ������N�G�X�g����i����Update()�Ŏ��s�����j
	 * @tparam T - �؂�ւ�����Scene�̋�ی^
	 */
	template<typename T>
	static void ChangeScene()
	{
		std::type_index type = std::type_index(typeid(T));
		if (s_sceneFactories.count(type))
		{
			s_nextSceneFactory = s_sceneFactories[type];
		}
		else
		{
			// TODO: �G���[�����i���o�^�̃V�[���^�j
			// ��O�𓊂��邩�A���O�ɏo��
		}
	}
};

#endif // !___SCENE_MANAGER_H___