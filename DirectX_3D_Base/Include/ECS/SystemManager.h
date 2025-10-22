#ifndef ___SYSTEMMANAGER_H___
#define ___SYSTEMMANAGER_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include <map>
#include <stdexcept>

/**
 * @class SystemManager
 * @brief System�̃C���X�^���X�ASignature�A����я����Ώ�Entity�̏W�����Ǘ�����
 */
class SystemManager
{
private:
    // System�̌^����System�C���X�^���X�̃}�b�v
    std::map<const char*, Signature> signatures_;

    // System�̌^����System�C���X�^���X�̃}�b�v
    std::map<const char*, std::shared_ptr<System>> systems_;

public:
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        const char* typeName = typeid(T).name();

        if (systems_.count(typeName))
        {
            throw std::runtime_error("Error: System type already registered.");
        }

        // System�C���X�^���X���쐬���A�}�b�v�Ɋi�[
        auto system = std::make_shared<T>();
        systems_.insert({ typeName, system });
        return system;
    }

    template<typename T>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(T).name();

        if (!systems_.count(typeName))
        {
            throw std::runtime_error("Error: System not registered. Call RegisterSystem<T>() first.");
        }

        // System���������ׂ�Component��Signature��o�^
        signatures_.insert({ typeName, signature });
    }

    /// @brief Entity��Signature���ύX���ꂽ�Ƃ��ɌĂяo�����
    void EntitySignatureChanged(Entity entity, Signature entitySignature)
    {
        // �S�Ă�System�ɑ΂��ă��[�v���AEntity�������Ώۂɒǉ�/�폜���邩���f����
        for (auto const& pair : systems_)
        {
            const char* typeName = pair.first;
            auto& system = pair.second;
            const Signature& systemSignature = signatures_[typeName];

            // Entity��Signature��System��Signature���܂��邩�H
            // (entitySignature & systemSignature) == systemSignature
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // ��v���� -> System�̏����Ώ�Entity���X�g�ɒǉ�
                system->entities->insert(entity);
            }
            else
            {
                // ��v���Ȃ� -> System�̏����Ώ�Entity���X�g����폜
                system->entities->erase(entity);
            }
        }
    }

    /// @brief Entity���j�󂳂ꂽ�Ƃ��ɌĂяo�����
    void EntityDestroyed(Entity entity)
    {
        // �S�Ă�System�̏����Ώ�Entity���X�g����폜����
        for (auto const& pair : systems_)
        {
            pair.second->entities->erase(entity);
        }
    }
};

#endif // !___SYSTEMMANAGER_H___