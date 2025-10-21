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

// ===== �C���N���[�h =====
#include <string>
#include <memory>
#include "ECS/World.h"
#include "ECS/System/RenderSystem.h" // RenderSystem���e�V�[���ŕێ��E���s���邽��

// ===== �O���錾 =====
class SceneManager;

/**
 * @interface   Scene
 * @brief       �e�V�[���̒��ۊ��N���X
 */
class Scene
{
public:
    virtual ~Scene() = default;

    /**
     * @brief   �V�[���J�n���̏���������
     * @details World�̏������AEntity�̐����ARenderSystem�Ȃǂ̃Z�b�g�A�b�v���s��
     */
    virtual void Initialize(SceneManager& manager) = 0;

    /**
     * @brief ���t���[���̍X�V����
     * @param[in] dt �f���^�^�C��
     */
    virtual void Update(float dt) = 0;

    /**
     * @brief ���t���[���̕`�揈��
     */
    virtual void Draw() = 0;

    /**
     * @brief �V�[���I�����̌㏈��
     */
    virtual void Finalize() = 0;

protected:
    // �e�V�[�����Ɨ�����World��RenderSystem�̃C���X�^���X�����L
    std::unique_ptr<World> world_;
    std::unique_ptr<RenderSystem> renderer_;
};

/**
 * @class SceneManager
 * @brief �V�[���J�ڂƌ��݂̃V�[���̍X�V�E�`����Ǘ�����N���X
 */
class SceneManager
{
public:
    void RegisterScene(const std::string& name, std::unique_ptr<Scene> scene);
    void ChangeScene(const std::string& name);
    void Update(float deltaTime);
    void Draw();

private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> mScenes;
    Scene* mCurrentScene = nullptr;
    std::string mNextSceneName;
};

#endif // !___SCENE_H___