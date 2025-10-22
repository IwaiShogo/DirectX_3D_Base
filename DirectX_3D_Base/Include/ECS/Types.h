/*****************************************************************//**
 * @file	Types.h
 * @brief	ECS�ŗ��p�����{�ƂȂ���̒�`
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

#ifndef ___TYPES_H___
#define ___TYPES_H___

// ===== �C���N���[�h =====
#include <cstdint>
#include <bitset>
#include <memory>
#include <typeinfo>
#include <set>

// --------------------------------------------------
// ECS�̊�{�^��` (���S�҂ł��������₷���悤�ɃV���v���ȃG�C���A�X���g�p)
// --------------------------------------------------

/// @brief Entity��ID�B0���珇�Ɋ���U��B
using Entity = std::uint32_t;
/// @brief Entity�̍ő吔�B�Q�[���̋K�͂ɉ����Ē�������B
constexpr Entity MAX_ENTITIES = 5000;

/// @brief Component��ID�B0���珇�Ɋ���U��B
using ComponentType = std::uint8_t;
/// @brief Component�̍ő吔�B�ő�64��ނ܂Œ�`�\�i�r�b�g�Z�b�g�T�C�Y�Ɉˑ��j�B
constexpr ComponentType MAX_COMPONENTS = 32; // 64���\�����A�܂��̓R���p�N�g��32��
// Component�̍ő吔�𒴂����ꍇ�A�r�b�g�Z�b�g�̃T�C�Y��ύX����K�v������B

/// @brief Component�̏W���i����/�}�X�N�j�B�ǂ�Component��Entity�������A�ǂ�System���������邩�������B
using Signature = std::bitset<MAX_COMPONENTS>;

// --------------------------------------------------
// System��Component�̊��N���X (�����ȃC���^�[�t�F�[�X)
// --------------------------------------------------

/// @brief �S�Ă�Component���p������}�[�J�[�C���^�[�t�F�[�X�i�f�[�^�R���e�i�̖ڈ�j
struct IComponent
{
    virtual ~IComponent() = default;
};

/// @brief �S�Ă�System���p��������C���^�[�t�F�[�X
class System
{
public:
    /// @brief ����System���������ׂ�Entity�̏W��
    Signature componentSignature;

    /// @brief ����System����������Entity��ID�W��
    std::shared_ptr<std::set<Entity>> entities;

    System() : entities(std::make_shared<std::set<Entity>>()) {}
    virtual ~System() = default;

    /// @brief ���������� (Coordinator�ŏ��������Ɉ�x�����Ăяo�����)
    virtual void Initialize() {}

    /// @brief �X�V���� (���C�����[�v�Ŗ��t���[���Ăяo�����)
    virtual void Update(float deltaTime) = 0;
};

#endif // !___TYPES_H___