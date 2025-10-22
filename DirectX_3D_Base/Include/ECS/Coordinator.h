#ifndef ___COORDINATOR_H___
#define ___COORDINATOR_H___



#include "Types.h"
#include "ComponentManager.h" // ��Ŏ���
#include "EntityManager.h"    // ��Ŏ���
#include "SystemManager.h"    // ��Ŏ���

/**
 * @class Coordinator
 * @brief Entity-Component-System�̑S�Ă𓝊�����R�A�}�l�[�W���[�i�t�@�T�[�h�j
 * * ECS�̗��p�҂́A���̃N���X��ʂ��Ă̂�Entity�̐����AComponent�̒ǉ��A
 * System�̓o�^�Ȃǂ��s�����ƂŁA�����ڍׂ��番������܂��B
 */
class Coordinator
{
private:
    std::unique_ptr<ComponentManager> componentManager_;
    std::unique_ptr<EntityManager> entityManager_;
    std::unique_ptr<SystemManager> systemManager_;

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