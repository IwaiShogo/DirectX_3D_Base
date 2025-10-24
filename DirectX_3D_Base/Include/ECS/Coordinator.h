#ifndef ___COORDINATOR_H___
#define ___COORDINATOR_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

/* Component */
#include "ECS/Component/MeshRenderer.h"

// ===== �O���錾 =====
// �O�����\�[�X�}�l�[�W���[
class ModelManager;
class TextureManager;

/**
 * @class Coordinator
 * @brief Entity-Component-System�̑S�Ă𓝊�����R�A�}�l�[�W���[�i�t�@�T�[�h�j
 * * ECS�̗��p�҂́A���̃N���X��ʂ��Ă̂�Entity�̐����AComponent�̒ǉ��A
 * System�̓o�^�Ȃǂ��s�����ƂŁA�����ڍׂ��番������܂��B
 */
class Coordinator
{
private:
    // ECS�Ǘ�
    std::unique_ptr<ComponentManager>   componentManager_;
    std::unique_ptr<EntityManager>      entityManager_;
    std::unique_ptr<SystemManager>      systemManager_;

    // ���\�[�X�Ǘ�
    ModelManager*   modelManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;

public:
    void Init()
    {
        // Manager�̏�����
        entityManager_ = std::make_unique<EntityManager>();
        componentManager_ = std::make_unique<ComponentManager>();
        systemManager_ = std::make_unique<SystemManager>();
    }

    // -----------------------------------------
    // Entity�֐�
    // -----------------------------------------
    Entity CreateEntity()
    {
        return entityManager_->CreateEntity();
    }

    void DestroyEntity(Entity entity)
    {
        entityManager_->DestroyEntity(entity);
        componentManager_->EntityDestroyed(entity);
        systemManager_->EntityDestroyed(entity);
    }

    // -----------------------------------------
    // Component�֐� (�ǐ������߂邽�߂Ƀe���v���[�g���g�p)
    // -----------------------------------------

    /// @brief Component��o�^���AComponentType ID�����蓖�Ă�
    template<typename T>
    void RegisterComponent()
    {
        componentManager_->RegisterComponent<T>();
    }

    /// @brief Entity��Component��ǉ����AEntity��Signature���X�V����
    template<typename T>
    void AddComponent(Entity entity, T component)
    {
        componentManager_->AddComponent<T>(entity, component);

        auto signature = entityManager_->GetSignature(entity);
        signature.set(componentManager_->GetComponentType<T>(), true);
        entityManager_->SetSignature(entity, signature);

        systemManager_->EntitySignatureChanged(entity, signature);
    }

    /// @brief Entity����Component���폜���AEntity��Signature���X�V����
    template<typename T>
    void RemoveComponent(Entity entity)
    {
        componentManager_->RemoveComponent<T>(entity);

        auto signature = entityManager_->GetSignature(entity);
        signature.set(componentManager_->GetComponentType<T>(), false);
        entityManager_->SetSignature(entity, signature);

        systemManager_->EntitySignatureChanged(entity, signature);
    }

    /// @brief Entity����Component���擾����
    template<typename T>
    T& GetComponent(Entity entity)
    {
        return componentManager_->GetComponent<T>(entity);
    }

    /// @brief Component�̌^ID���擾����
    template<typename T>
    ComponentType GetComponentType()
    {
        return componentManager_->GetComponentType<T>();
    }

    // -----------------------------------------
    // System�֐�
    // -----------------------------------------

    /// @brief System��o�^���A����System�I�u�W�F�N�g���擾����
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        return systemManager_->RegisterSystem<T>();
    }

    /// @brief System�������ΏۂƂ���Component��Signature��ݒ肷��
    template<typename T>
    void SetSystemSignature(Signature signature)
    {
        systemManager_->SetSignature<T>(signature);
    }

    // -----------------------------------------
    // ���\�[�X�}�l�[�W���[�̓o�^ (GameScene::Initialize�ŌĂяo��)
    // -----------------------------------------
    void RegisterModelManager(ModelManager* manager)
    {
        modelManager_ = manager;
    }

    void RegisterTextureManager(TextureManager* manager)
    {
        textureManager_ = manager;
    }

    // -----------------------------------------
    // ���\�[�X���[�h��Entity�ւ�Component�t�^�̃w���p�[�֐�
    // -----------------------------------------

    /**
     * @brief ���f�������[�h���AMeshRenderer Component��Entity�ɕt�^����w���p�[
     * @param[in] entity �Ώۂ�Entity ID
     * @param[in] path ���f���t�@�C���̃p�X (��: "Assets/Model/cube.fbx")
     * @param[in] texturePath �e�N�X�`���t�@�C���̃p�X (ModelManager���e�N�X�`��������Ń��[�h����ꍇ�ɗ��p)
     * @return ���������ꍇ��Model ID�A���s�����ꍇ��0
     */
    std::uint32_t LoadModelAndAttachRenderer(Entity entity, const std::string& path, const std::string& texturePath = "")
    {
        if (!modelManager_ || !textureManager_)
        {
            // �G���[����
            return 0;
        }

        // 1. ���f���̃��[�h�܂��͎擾
        std::uint32_t modelId = 0; // modelManager_->LoadModel(path); // �[���R�[�h

        // 2. �e�N�X�`���̃��[�h�܂��͎擾
        std::uint32_t textureId = 0; // textureManager_->LoadTexture(texturePath); // �[���R�[�h

        if (modelId != 0)
        {
            // 3. MeshRenderer Component���쐬���AEntity�ɕt�^
            MeshRenderer mr;
            mr.meshId = modelId;
            mr.textureId = textureId;
            mr.isLoaded = true;

            AddComponent(entity, mr);
        }

        return modelId;
    }

    // -----------------------------------------
    // System�����\�[�X�ɃA�N�Z�X���邽�߂�Getter
    // -----------------------------------------

    ModelManager* GetModelManager() const { return modelManager_; }
    TextureManager* GetTextureManager() const { return textureManager_; }
};

// --------------------------------------------------
// �O���[�o���ϐ�: Coordinator�C���X�^���X (�V���O���R�ACoordinator��z��)
// --------------------------------------------------

// �O���[�o����Coordinator�C���X�^���X���쐬���A�ǂ�����ł��A�N�Z�X�ł���悤�ɂ���
// Game/Scene�N���X����Coordinator���������E�Ǘ����A�eSystem/�N���X�ɎQ�Ƃ�n���������]�܂������A
// �����R�[�h�Ƃ̐�������D�悵�A�����ł̓V���O���g���A�N�Z�X�\�ȃw���p�[���`����B

// �� ���ۂ̃v���W�F�N�g�ł͈ˑ�������(DI)�܂��̓V�[���Ǘ��N���X�ɂ��Ǘ��𐄏��B
//    ECS���S�Ҍ����̂��߁A�����ł̓V���v���ȐÓI�A�N�Z�X�Ƃ��Ă��܂��B

// Coordinator* g_Coordinator = nullptr; // ���C�����[�v����new���Ďg�p����

#endif // !___COORDINATOR_H___