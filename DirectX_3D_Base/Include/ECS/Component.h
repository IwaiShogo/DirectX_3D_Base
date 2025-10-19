/*****************************************************************//**
 * @file	Component.h
 * @brief	ECS�R���|�[�l���g�V�X�e���̊��N���X�ƃ}�N����`
 * 
 * @details	Entity Component System (ECS) �A�[�L�e�N�`���ɂ�����
 *			�R���|�[�l���g�̊�{��`��񋟁B
 *			����ECS�́A**�f�[�^�R���|�[�l���g**��**Behaviour�R���|�[�l���g**
 *			�̂Q��ނ��T�|�[�g���܂��B
 *			���̃t�@�C���͂����̊��N���X�ƁA��`���Ȍ��ɂ��邽�߂�
 *			�}�N����񋟂��܂��B
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/17	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

#ifndef ___COMPONENT_H___
#define ___COMPONENT_H___

// ===== �C���N���[�h =====
#include "Entity.h"		// Entity
#include <typeindex>	// ComponentTypeID�̑����std::type_index���g�p���邱�Ƃ�z��

// ===== �O���錾 =====
class World;

/**
 * @interfaace	IComponent
 * @brief	�S�ẴR���|�[�l���g�̊��C���^�[�t�F�[�X�i�f�[�^�R���|�[�l���g�̐e�j
 * 
 * @details	���̃N���X���p�����邱�ƂŁAWorld���R���|�[�l���g�̕���� (std::type_index)	��
 *			���ʂ��A�����I�ɊǗ��ł���悤�ɂȂ�܂��B
 * 
 * @note	�f�[�^�݂̂����R���|�[�l���g�͂���IComponent�𒼐ڌp�����邩�A
 *			��q��'DEFINE_DATA_COMPONENT'�}�N�����g�p���Ă��������B
 */
struct IComponent
{
	virtual ~IComponent() = default;
};

// --------------------------------------------------
// Behaviour Component (���W�b�N�����R���|�[�l���g)
// --------------------------------------------------

/**
 * @interface   Behaviour
 * @brief   ���t���[���X�V���W�b�N�����R���|�[�l���g�̊��N���X
 * 
 * @details �]����System�̃��W�b�N���R���|�[�l���g���g�Ɏ������邱�ƂŁA
 *          �G���e�B�e�B�̐U�镑������蒼���I�ɁA�I�u�W�F�N�g�w���ɋ߂��`�ŋL�q�ł��܂��B
 * 
 * @note    Behaviour�R���|�[�l���g�́AWorld::Update()���Ŏ����I�ɍX�V����܂��B
 */
struct Behaviour : IComponent
{
    /**
     * @brief �R���|�[�l���g��World�ɒǉ�����A�L���ɂȂ����Ƃ��Ɉ�x�����Ă΂�鏉��������
     * @param[in,out] w ���[���h�ւ̎Q��
     * @param[in] self ���̃R���|�[�l���g���t���Ă���G���e�B�e�B
     */
    virtual void OnStart(World& w, Entity self) {}

    /**
     * @brief ���t���[���Ă΂��X�V����
     * @param[in,out] w ���[���h�ւ̎Q��
     * @param[in] self ���̃R���|�[�l���g���t���Ă���G���e�B�e�B
     * @param[in] dt �f���^�^�C���i�O�t���[������̌o�ߕb���j
     */
    virtual void OnUpdate(World& w, Entity self, float dt) = 0;
};

// --------------------------------------------------
// �֗��ȃ}�N����` (�g���₷������̂��߂̋@�\)
// --------------------------------------------------

/**
 * @def     DEFINE_DATA_COMPONENT
 * @brief   �f�[�^�R���|�[�l���g���ȒP�ɒ�`����}�N��
 * 
 * @param   ComponentName �R���|�[�l���g�̖��O
 * @param   ... �����o�ϐ��̒�`�i�����̃Z�~�R�����͕s�v�j
 * 
 * @par     �g�p��:
 * @code
 *  // �̗̓R���|�[�l���g
 *  DEFINE_DATA_COMPONENT(Health,
 *  float hp = 100.0f;
 *  float maxHp = 100.0f;
 *  )
 *  // �^�O�R���|�[�l���g�i�f�[�^�Ȃ��j
 *  DEFINE_DATA_COMPONENT(Player, )
 * @endcode
 * 
 * @note    �����o�ϐ��̒�`�̖����ɃZ�~�R������**�s�v**�ł��i�}�N�����Ŏ����I�ɒǉ�����܂��j�B
 */
#define DEFINE_DATA_COMPONENT(ComponentName, ...) \
    struct ComponentName : IComponent { \
        __VA_ARGS__ \
    }

/**
 * @def     DEFINE_BEHAVIOUR
 * @brief   Behaviour�R���|�[�l���g���ȒP�ɒ�`����}�N��
 * 
 * @param   BehaviourName �R���|�[�l���g�̖��O
 * @param   DataMembers �����o�ϐ��̒�`�i�Z�~�R�����K�{�j
 * @param   UpdateCode OnUpdate()���Ŏ��s����R�[�h�i�Z�~�R�����K�{�j
 * 
 * @details OnUpdate()�̎����𒼐ڏ����邽�߁A���I�ȃ��W�b�N����̃t�@�C�����ŊȌ��ɒ�`�ł��܂��B
 * 
 * @par     �g�p��:
 * @code
 *  // �㉺�ɗh���Behaviour
 *  DEFINE_BEHAVIOUR(Blinker,
 *      float timer = 0.0f;
 *  ,
 *      timer += dt;
 *      if (timer > 1.0f) {
 *      // 1�b���Ƃɓ_�Ń��W�b�N
 *      timer = 0.0f;
 *      }
 *  )
 * @endcode
 * 
 * @warning DataMembers��UpdateCode�̊Ԃ̃J���}��Y���ƃR���p�C���G���[�ɂȂ�܂��B
 */
#define DEFINE_BEHAVIOUR(BehaviourName, DataMembers, UpdateCode) \
    struct BehaviourName : Behaviour { \
        DataMembers \
        void OnUpdate(World& w, Entity self, float dt) override { \
            UpdateCode \
        } \
    }

// --------------------------------------------------
// �^ID�w���p�[
// --------------------------------------------------

/**
 * @brief   �R���|�[�l���g�^T�ɑΉ����� std::type_index ���擾���܂��B
 * 
 * @tparam  T IComponent���p�������R���|�[�l���g�^
 * @return  std::type_index �R���|�[�l���g�̌^����ӂɎ����C���f�b�N�X
 * 
 * @note    ���̃C���f�b�N�X�́AWorld�N���X�����ŃR���|�[�l���g�X�g�A�����ʂ��邽�߂̃L�[�Ƃ��Ďg�p����܂��B
 */
template<typename T>
inline std::type_index GetComponentTypeID() noexcept
{
    static_assert(std::is_base_of<IComponent, T>::value, "T must inherit from IComponent.");
    return std::type_index(typeid(T));
}

#endif // !___COMPONENT_H___