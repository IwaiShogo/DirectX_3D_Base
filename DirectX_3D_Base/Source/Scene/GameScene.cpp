/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
#include "Scene/GameScene.h"

// Component

// System


/* ���\�[�X�}�l�[�W���[ */
#include "Systems/Model.h"
#include "Systems/DirectX/Texture.h"



// --------------------------------------------------
// �����֐�: ECS�̏����ݒ�
// --------------------------------------------------

/**
 * @brief ECS�R�A�AComponent�ASystem�̏������Ɠo�^���s��
 */
void GameScene::SetupECS()
{
    
}

/**
 * @brief ECS Entity�̏����z�u�i�e�X�g�pEntity�̐����Ȃǁj���s��
 */
void GameScene::SetupEntities()
{
    
}

// --------------------------------------------------
// Scene�C���^�[�t�F�[�X�̎���
// --------------------------------------------------

void GameScene::Initialize()
{
    // ECS�̃Z�b�g�A�b�v
    SetupECS();

    // Entity�̏����z�u
    SetupEntities();

    // ... �J������DirectX���\�[�X�̏������Ȃ�
}

void GameScene::Update(float deltaTime)
{
    
}

void GameScene::Draw()
{
    

}

void GameScene::Finalize()
{
    // ... �V�[���ŗL�̃��\�[�X����Ȃ�
}