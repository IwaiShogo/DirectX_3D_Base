/*****************************************************************//**
 * @file	System.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/15	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___SYSTEM_H___
#define ___SYSTEM_H___

// ===== �C���N���[�h =====
#include "ECS.h" // �R�A��`���Q��
#include <typeinfo> // typeid().name() �̂��߂ɕK�v

 // --------------------------------------------------
 // 1. System�̒��ۊ��N���X
 // --------------------------------------------------
class System
{
public:
    // ����System�������ΏۂƂ���Entity��Component�\��
    Signature componentSignature;

    // ����System����������Entity��ID���X�g
    std::vector<EntityID> entities;

    // ���t���[�����s�����System�̃��W�b�N (�������z�֐�)
    virtual void Update(float deltaTime) = 0;

    virtual ~System() = default;
};


// --------------------------------------------------
// 2. System Manager �̐錾
// --------------------------------------------------
class SystemManager
{
public:
    // �e���v���[�g�֐�: System�̓o�^�ƃC���X�^���X�擾
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
    {
        const char* typeName = typeid(T).name();

        if (mSystems.count(typeName)) {
            return std::static_pointer_cast<T>(mSystems[typeName]);
        }

        std::shared_ptr<T> system = std::make_shared<T>();
        mSystems.insert({ typeName, system });
        return system;
    }

    // �e���v���[�g�֐�: System�̏����ݒ�
    template<typename T>
    void SetSignature(Signature signature)
    {
        const char* typeName = typeid(T).name();

        if (mSystems.find(typeName) == mSystems.end()) {
            throw std::runtime_error("SystemManager::SetSignature(): System not registered before setting Signature.");
        }

        mSignatures[typeName] = signature;
        mSystems[typeName]->componentSignature = signature;
    }

    // ��e���v���[�g�֐� (System.cpp�Ŏ���)
    /**
     * @brief Entity��Signature���ύX���ꂽ�Ƃ��ASystem��Entity���X�g���X�V���܂��B
     */
    void EntitySignatureChanged(EntityID entityID, Signature entitySignature)
    {
        // �S�Ă�System�ɑ΂��ă`�F�b�N�����s
        for (auto const& pair : mSystems)
        {
            const char* typeName = pair.first;
            std::shared_ptr<System> system = pair.second;

            // System���v������Signature���擾
            Signature systemSignature = mSignatures[typeName];

            // Entity��Signature��System��Signature���܂��邩�ǂ���
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // Entity��System�̗v���𖞂����Ă���ꍇ (���X�g�ɒǉ�/�ێ�)

                auto& entities = system->entities;
                bool alreadyIn = false;
                for (EntityID id : entities) {
                    if (id == entityID) {
                        alreadyIn = true;
                        break;
                    }
                }
                if (!alreadyIn) {
                    entities.push_back(entityID);
                }
            }
            else
            {
                // Entity��System�̗v���𖞂����Ȃ��Ȃ����ꍇ (���X�g����폜)

                auto& entities = system->entities;
                for (auto it = entities.begin(); it != entities.end(); ++it)
                {
                    if (*it == entityID)
                    {
                        entities.erase(it);
                        break;
                    }
                }
            }
        }
    }
    /**
     * @brief Entity���j�����ꂽ�Ƃ��A�S�Ă�System��Entity���X�g����폜���܂��B
     */
    void EntityDestroyed(EntityID entityID)
    {
        // �S�Ă�System��Entity���X�g����ID���폜
        for (auto const& pair : mSystems)
        {
            auto& entities = pair.second->entities;
            for (auto it = entities.begin(); it != entities.end(); ++it)
            {
                if (*it == entityID)
                {
                    entities.erase(it);
                    break;
                }
            }
        }
    }

    // Coordinator��System�����[�v���邽�߂Ɏg�p����A�N�Z�T
    const std::unordered_map<const char*, std::shared_ptr<System>>& GetSystems() const
    {
        return mSystems;
    }

private:
    std::unordered_map<const char*, std::shared_ptr<System>> mSystems;
    std::unordered_map<const char*, Signature> mSignatures;
};

#endif // !___SYSTEM_H___