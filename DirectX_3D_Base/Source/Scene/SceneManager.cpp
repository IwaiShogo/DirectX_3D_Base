/*****************************************************************//**
 * @file	SceneManager.cpp
 * @brief	�Q�[���V�[���̊Ǘ��A�؂�ւ��A���C�t�T�C�N��������s���ÓI�N���X�̎����B
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F`SceneManager`�̐ÓI�����o�[���`���A�V�[���؂�ւ����W�b�N�������B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "Scene/SceneManager.h"
#include <iostream>

// ===== �ÓI�����o�[�ϐ��̒�` =====
std::unique_ptr<Scene> SceneManager::s_currentScene = nullptr;
std::function<Scene* ()> SceneManager::s_nextSceneFactory = nullptr;
std::unordered_map<std::type_index, std::function<Scene* ()>> SceneManager::s_sceneFactories;


/**
 * @brief ���݂̃V�[����j�����A���̃V�[���𐶐��E�����������������
 */
void SceneManager::ProcessSceneChange()
{
	// ���̃V�[���̃t�@�N�g���֐����ݒ肳��Ă��Ȃ���Ή������Ȃ�
	if (!s_nextSceneFactory)
	{
		return;
	}

	// 1. ���݂̃V�[���̏I�������Ɣj��
	if (s_currentScene)
	{
		s_currentScene->Uninit();
		// unique_ptr::reset()�ɂ��A�f�X�g���N�^���Ă΂�A����������������
		s_currentScene.reset();
		std::cout << "SceneManager: Previous scene uninitialized and destroyed." << std::endl;
	}

	// 2. �V�����V�[���̐����Ə�����
	// �t�@�N�g���֐�����V����Scene*���擾���Aunique_ptr�ŏ��L�����Ǘ�
	Scene* newScene = s_nextSceneFactory();
	s_currentScene = std::unique_ptr<Scene>(newScene);
	s_currentScene->Init();

	// 3. �؂�ւ��v�����N���A
	s_nextSceneFactory = nullptr;
	std::cout << "SceneManager: New scene initialized and started." << std::endl;
}

void SceneManager::Init()
{
	std::cout << "SceneManager: Initialized." << std::endl;
	// �����őS�ẴV�[���iGameScene, TitleScene�Ȃǁj��RegisterScene<T>()�œo�^���邱�Ƃ����������
}

void SceneManager::Uninit()
{
	// ���݂̃V�[���̏I�������Ɣj��
	if (s_currentScene)
	{
		s_currentScene->Uninit();
		s_currentScene.reset();
		std::cout << "SceneManager: All scenes uninitialized and destroyed." << std::endl;
	}

	// �t�@�N�g���̃N���A
	s_sceneFactories.clear();
}

void SceneManager::Update(float deltaTime)
{
	// ���t���[���̍ŏ��ɃV�[���؂�ւ����`�F�b�N�E���s����
	ProcessSceneChange();

	// ���݂̃V�[���̍X�V����
	if (s_currentScene)
	{
		s_currentScene->Update(deltaTime);
	}
}

void SceneManager::Draw()
{
	// ���݂̃V�[���̕`�揈��
	if (s_currentScene)
	{
		s_currentScene->Draw();
	}
}