/*****************************************************************//**
 * @file	Scene.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/10/21	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___SCENE_H___
#define ___SCENE_H___

/**
 * @class   Scene
 * @brief   �V�[���Ǘ��̂��߂̒��ۊ��N���X
 */
class Scene
{
public:
    virtual ~Scene() = default;

    /**
     * @brief �V�[���̏���������
     * @note ECS�R���|�[�l���g�ƃV�X�e���A����уV�[���ŗL�̃��\�[�X�̓ǂݍ��݂��s��
     */
    virtual void Initialize() = 0;

    /**
     * @brief �V�[���̍X�V����
     * @param[in] deltaTime �O�t���[������̌o�ߎ��ԁi�b�j
     * @note ECS�V�X�e���iMovementSystem�Ȃǁj��Update���\�b�h���Ăяo��
     */
    virtual void Update(float deltaTime) = 0;

    /**
     * @brief �V�[���̕`�揈��
     * @note ECS�V�X�e���iRenderSystem�Ȃǁj��Draw���\�b�h���Ăяo��
     */
    virtual void Draw() = 0;

    /**
     * @brief �V�[���̏I������
     */
    virtual void Finalize() = 0;
};

#endif // !___SCENE_H___