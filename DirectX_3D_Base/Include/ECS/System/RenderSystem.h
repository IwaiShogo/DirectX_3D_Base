/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	ECS���[���h���̕`��\�ȃG���e�B�e�B��`�悷��V�X�e��
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

#ifndef ___RENDERSYSTEM_H___
#define ___RENDERSYSTEM_H___

// ===== �C���N���[�h =====
// --------------------------------------------------
// ECS �R�A
// --------------------------------------------------
#include "ECS/Types.h"
#include "ECS/Coordinator.h" // Coordinator�̎Q�Ƃ�K�v�Ƃ��邽�߁A�����ŃC���N���[�h

// �`��ɕK�v��Component
#include "ECS/Component/Transform.h"
#include "ECS/Component/MeshRenderer.h"

// --------------------------------------------------
// DirectX �ˑ��V�X�e�� (RenderSystem�̎��s�ɕK�v�ȊO���T�[�r�X)
// --------------------------------------------------
#include "Systems/DirectX/DirectX.h"    
#include "Systems/DirectX/ShaderList.h" 
#include "Systems/DirectX/MeshBuffer.h"  
#include "Systems/DirectX/Texture.h"     

#include <DirectXMath.h>
#include <wrl/client.h>
#include <map>

// MeshBuffer�̊Ǘ��N���X�͂܂���`����Ă��܂��񂪁ADraw���\�b�h�̈����Ƃ��ĕK�v�ł��B
class MeshManager; // ���f���E���b�V�����Ǘ�����N���X (��)
class Camera;      // �J��������񋟂���N���X (��)

// Coordinator�̃O���[�o���Q�� (GameScene���Ǘ����邱�Ƃ�z�肵�A�O���A�g�p�Ƃ��Ē�`)
extern Coordinator *g_pCoordinator;

/**
 * @class RenderSystem
 * @brief ECS���[���h���̕`��\�ȃG���e�B�e�B��`�悷��V�X�e��
 * @note System���N���X���p�����AUpdate(�܂���Draw)���\�b�h�Ń��W�b�N�����s���܂��B
 */
class RenderSystem : public System
{
public:
    // �`��ɕK�v�ȃ��\�[�X�̏��
    ShaderList::VSKind vsKind_ = ShaderList::VS_WORLD;
    ShaderList::PSKind psKind_ = ShaderList::PS_LAMBERT;

    // ECS��System�N���X�́A���ۊ��N���X��Update���\�b�h����������`��������܂����A
    // RenderSystem�͕`��p�C�v���C���̍Ō�ɌĂ΂�邱�Ƃ��������߁ADraw���\�b�h�ő�ւ��܂��B
    // ���̂��߁AUpdate���\�b�h�͂����ł͋�Œ�`���܂��B�i�������z�֐����������邽�߁j
    void Update(float deltaTime) override {}

    /**
     * @brief �`����s�̃��C���G���g���[�|�C���g (DirectX��DrawCall�����s)
     * @param[in] cam ���݂̃V�[���J����
     * @param[in] meshMgr MeshBuffer�Ǘ��N���X
     */
    template<typename MeshManager>
    void Draw(Camera& cam, MeshManager& meshMgr)
    {
        // 1. �V�F�[�_�ƕ`��R���e�L�X�g�̐ݒ� (System�̏������܂��͊O�������x�����ݒ�)
        ShaderList::GetVS(vsKind_)->Set();
        ShaderList::GetPS(psKind_)->Set();

        // 2. �J�����s��̎擾
        DirectX::XMMATRIX view = cam.GetViewMatrix();
        DirectX::XMMATRIX proj = cam.GetProjectionMatrix();
        DirectX::XMMATRIX viewProj = view * proj;

        // 3. �S�Ă̕`��\�ȃG���e�B�e�B�ɑ΂��ă��[�v
        // this->entities�ɂ́ARenderSystem��Signature�Ɉ�v����Entity ID�݂̂��܂܂�Ă��܂��B
        for (const Entity entity : *entities)
        {
            // Coordinator��ʂ���Component�f�[�^���擾 (���W�b�N�ƃf�[�^�𕪗�)
            // RenderSystem��Signature�� Transform �� MeshRenderer ��v�����邽�߁A
            // �����Ŏ擾�����s���邱�Ƃ͂Ȃ��͂��ł����A�ی��Ƃ��� Try-Catch ��A�T�[�g���g�p��������]�܂����ł��B

            Transform& t = g_pCoordinator->GetComponent<Transform>(entity);
            MeshRenderer& mr = g_pCoordinator->GetComponent<MeshRenderer>(entity);

            // a. World�s��̌v�Z��Transform�ւ̃L���b�V��
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(t.rotation.x),
                DirectX::XMConvertToRadians(t.rotation.y),
                DirectX::XMConvertToRadians(t.rotation.z)
            );
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
            DirectX::XMMATRIX W = S * R * T;
            DirectX::XMStoreFloat4x4(&t.worldMatrix, W); // �v�Z���ʂ�Component�ɏ����߂�

            DirectX::XMMATRIX WVP = W * viewProj;

            // b. VS�萔�o�b�t�@�̍X�V (WVP)
            DirectX::XMFLOAT4X4 WVP_T;
            DirectX::XMStoreFloat4x4(&WVP_T, DirectX::XMMatrixTranspose(WVP));
            ShaderList::SetWVP(&WVP_T);

            // c. PS�萔�o�b�t�@�̍X�V (Material/Color)
            // Material�萔�o�b�t�@��WVP�ȊO�̏��(���[���h�s��Ȃ�)���K�v�ȏꍇ�A�����ŃZ�b�g
            // ShaderList::SetMaterial(mr.color, (mr.textureId != 0)); // �[���R�[�h

            // d. �e�N�X�`���̐ݒ�
            // Texture* tex = TextureManager::GetTexture(mr.textureId); // TextureManager�N���X���ʓr�K�v
            // if (tex) GetContext()->PSSetShaderResources(0, 1, tex->GetResource()); // �[���R�[�h

            // e. �`����s (MeshBuffer::Draw���g�p)
            // MeshBuffer* meshBuf = meshMgr.GetMeshBuffer(mr.meshId); 
            // if (meshBuf && mr.isLoaded) meshBuf->Draw(); // �[���R�[�h
        }
    }
};

#endif // !___RENDERSYSTEM_H___