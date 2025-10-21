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

// ===== �C���N���[�h =====
#include "Scene/Scene.h"
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"
#include "ECS/Component/Rotator.h"
//#include "ECS/Component/InputState.h"
//#include "ECS/Component/PlayerMovement.h"
#include "Systems/DirectX/DirectX.h" // DirectX�N���X�iGfxDevice�j
#include "Systems/DirectX/MeshBuffer.h" // MeshBuffer�iMeshManager�j
#include "Systems/DirectX/ShaderList.h" // ShaderList

class GameScene : public Scene
{
public:
    // ���������ASceneManager����Ă΂��
    void Initialize(SceneManager& manager) override;

    // ���t���[���Ă΂��X�V
    void Update(float dt) override;

    // ���t���[���Ă΂��`��
    void Draw() override;

    // �I������
    void Finalize() override;

private:
    // RenderSystem�������ɕK�v�Ȉˑ��I�u�W�F�N�g��ێ� (���̈ˑ�������)
    DirectX* gfx_ = nullptr;
    Camera* cam_ = nullptr; // �����R�[�h��Camera�N���X��z��
    MeshBuffer* meshMgr_ = nullptr;
    ShaderList* shaderList_ = nullptr;
};

#endif // !___GAMESCENE_H___