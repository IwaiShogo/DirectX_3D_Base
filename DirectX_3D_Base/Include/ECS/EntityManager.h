/*****************************************************************//**
 * @file	EntityManager.h
 * @brief	Entity�̐�����Signature�Ǘ�
 * 
 * @details	Entity��P�Ȃ�ID�Ƃ��Ĉ����A
 *          �ǂ�Component�������Ă��邩������Signature���Ǘ����܂��B
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

#ifndef ___ENTITYMANAGER_H___
#define ___ENTITYMANAGER_H___

// ===== �C���N���[�h =====
#include "Types.h"
#include <queue>
#include <array>
#include <stdexcept>

/**
 * @class EntityManager
 * @brief Entity ID�̔��s�ƁA����Entity������Component��Signature���Ǘ�����
 */
class EntityManager
{
private:
    // �ė��p�\��Entity ID��ێ�����L���[
    std::queue<Entity> availableEntities_;

    // �eEntity ID�ɑΉ�����Component Signature��ێ�����z��
    // Signature�́A�ǂ�Component Type�������Ă��邩�������r�b�g�}�X�N
    std::array<Signature, MAX_ENTITIES> signatures_;

    // ���݂܂łɐ������ꂽEntity�̑���
    std::uint32_t livingEntityCount_ = 0;

public:
    EntityManager()
    {
        // �������FMAX_ENTITIES�܂ł�ID���L���[�ɓ�������
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            availableEntities_.push(entity);
        }
    }

    /// @brief �V����Entity�𐶐����A��ӂ�ID�����蓖�Ă�
    /// @return Entity ID
    Entity CreateEntity()
    {
        if (livingEntityCount_ >= MAX_ENTITIES)
        {
            throw std::runtime_error("Error: Maximum number of entities reached.");
        }

        // �L���[����Entity ID���擾
        Entity id = availableEntities_.front();
        availableEntities_.pop();
        ++livingEntityCount_;

        return id;
    }

    /// @brief Entity��j�����AID���ė��p�\�ɂ���
    /// @param[in] entity �j������Entity ID
    void DestroyEntity(Entity entity)
    {
        // Entity ID�̃V�O�l�`�������Z�b�g
        signatures_[entity].reset();

        // Entity ID���L���[�ɖ߂�
        availableEntities_.push(entity);
        --livingEntityCount_;
    }

    /// @brief Entity��Component Signature���擾
    /// @param[in] entity Entity ID
    /// @return Component Signature (bitset)
    Signature GetSignature(Entity entity) const
    {
        return signatures_[entity];
    }

    /// @brief Entity��Component Signature��ݒ�
    /// @param[in] entity Entity ID
    /// @param[in] signature �ݒ肷��Signature
    void SetSignature(Entity entity, Signature signature)
    {
        signatures_[entity] = signature;
    }
};

#endif // !___ENTITYMANAGER_H___