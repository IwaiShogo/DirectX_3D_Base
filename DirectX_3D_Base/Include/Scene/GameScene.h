/*****************************************************************//**
 * @file	GameScene.h
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

#ifndef ___GAMESCENE_H___
#define ___GAMESCENE_H___

#include "Scene.h"           // ���N���X Scene
#include "ECS/Coordinator.h" // ECS�R�A�Ǘ��N���X

// �V�[�����ŗ��p����S�Ă�System��O���錾 (Coordinator�ɓo�^���邽��)
class MovementSystem;
class RenderSystem;
class InputSystem;
class CollisionSystem;

/**
 * @class GameScene
 * @brief ���ۂ̃Q�[�����W�b�N��ECS�̃��C�t�T�C�N�����Ǘ�����V�[��
 */
class GameScene
    : public Scene
{
private:
    // --------------------------------------------------
    // ECS�Ǘ��R�A
    // --------------------------------------------------
    Coordinator coordinator_;

    // --------------------------------------------------
    // System�̃C���X�^���X (Coordinator����擾���A���t���[�����s�̂��߂ɕێ�)
    // --------------------------------------------------
    std::shared_ptr<MovementSystem> movementSystem_;
    std::shared_ptr<RenderSystem> renderSystem_;
    std::shared_ptr<InputSystem> inputSystem_;
    std::shared_ptr<CollisionSystem> collisionSystem_;

public:
    GameScene() = default;
    ~GameScene() override = default;

    // Scene�C���^�[�t�F�[�X�̎���
    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Finalize() override;

private:
    /**
     * @brief ECS�R�A�AComponent�ASystem�̏������Ɠo�^���s��
     */
    void SetupECS();

    /**
     * @brief ECS Entity�̏����z�u�i�e�X�g�pEntity�̐����Ȃǁj���s��
     */
    void SetupEntities();
};

#endif // !___GAMESCENE_H___