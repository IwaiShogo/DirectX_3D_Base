/*****************************************************************//**
 * @file	World.h
 * @brief	ECS���[���h�Ǘ��V�X�e���ƃG���e�B�e�B�r���_�[�̒�`
 * 
 * @details	���̃t�@�C���́AECS�A�[�L�e�N�`���̒��j�ƂȂ�World�N���X�ƁA
 *			�G���e�B�e�B���ȒP�ɍ쐬���邽�߂�EntityBuilder�N���X���`���܂��B
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

#ifndef ___WORLD_H___
#define ___WORLD_H___

// ===== �C���N���[�h =====
#include "Entity.h"
#include "Component.h"
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

// ===== �O���錾 =====
class World;

/**
 * @class EntityBuilder
 * @brief �G���e�B�e�B�쐬���ȒP�ɂ���r���_�[�p�^�[���N���X
 * 
 * @details
 * World::Create()�ɂ���ĕԂ���A���\�b�h�`�F�[�����g���āA�����̃R���|�[�l���g�����G���e�B�e�B��
 * �Ȍ��ɍ쐬�ł��܂��BWorld�N���X�ƘA�g���ē��삵�܂��B
 * 
 * @par �g�p��:
 * @code
 * // Entity player �� Build() ���ȗ����Ă� EntityBuilder ����ÖٓI�ɕϊ������
 * Entity player = world.Create()
 * .With<Transform>(DirectX::XMFLOAT3{0, 0, 0}) // �R���|�[�l���g�̏����l���w��
 * .With<MeshRenderer>(DirectX::XMFLOAT3{0, 1, 0})
 * .With<Rotator>(45.0f); // Behaviour�R���|�[�l���g�̒ǉ����\
 * @endcode
 * 
 * @see World �G���e�B�e�B�ƃR���|�[�l���g���Ǘ�����N���X
 */
class EntityBuilder
{
public:
    /**
     * @brief World�N���X��Create()����̂݌Ă΂��R���X�g���N�^
     * @param[in,out] w World�ւ̎Q��
     * @param[in] e ���ɐ������ꂽEntity
     */
    EntityBuilder(World& w, Entity e) : world_(w), entity_(e) {}

    /**
     * @brief �G���e�B�e�B�ɃR���|�[�l���g��ǉ����A���������܂��B
     * @tparam T �ǉ�����R���|�[�l���g�^ (IComponent���p��)
     * @param[in] args �R���|�[�l���gT�̃R���X�g���N�^����
     * @return EntityBuilder& ���\�b�h�`�F�[�����p�����邽�߂̃r���_�[���g�ւ̎Q��
     * 
     * @par �g�p��:
     * @code
     * .With<Transform>(pos, rot, scale)
     * @endcode
     */
    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args)
    {
        // 1. World����T�^��Store���擾
        Store<T>* store = world_.GetStore<T>();

        // 2. Entity�ɃR���|�[�l���g��ǉ����A�R���X�g���N�^�����ŏ�����
        T& component = store->Add(entity_, std::forward<Args>(args)...);

        // 3. Behaviour�R���|�[�l���g�̏ꍇ�́AWorld��Behaviour���X�g�ɓo�^
        if (std::is_base_of<Behaviour, T>::value) {
            // World����private�֐� registerBehaviour ���Ăяo��
            world_.registerBehaviour(entity_, static_cast<Behaviour*>(&component));
        }

        return *this;
    }

    /**
     * @brief �r���h���������AEntity��Ԃ��܂��B
     * @return Entity �������ꂽEntity
     */
    Entity Build()
    {
        return entity_;
    }

    /**
     * @brief EntityBuilder����Entity�ւ̈ÖٓI�Ȍ^�ϊ��I�y���[�^
     * * @details
     * ����ɂ��AEntity player = world.Create()...Build(); �� Build() ���ȗ��ł��܂��B
     */
    operator Entity() const { return entity_; }

private:
    World& world_;      ///< World�ւ̎Q�Ɓi�R���|�[�l���g�ǉ��Ɏg�p�j
    Entity entity_;     ///< ���ݍ\�z����Entity
};

// --------------------------------------------------
// 1. �R���|�[�l���g�X�g�A�̊�� (IStore, Store<T>)
// --------------------------------------------------

/**
 * @struct	IStore
 * @brief	�S�ẴR���|�[�l���g�X�g�A�̒��ۊ��N���X
 * 
 * @details	World�N���X���^���ӎ������Ɂistd::type_index�Łj�����
 *			�R���|�[�l���g�X�g�A�ւ̃|�C���^��ێ��ł���悤��
 *			���邽�߂̃C���^�[�t�F�[�X�ł��B
 */
struct IStore
{
	virtual ~IStore() = default;

	/**
	 * [void - RemoveComponent]
	 * @brief	�G���e�B�e�B���j�����ꂽ�ہA���̃R���|�[�l���g���X�g�A����폜����
	 * 
	 * @param	[in] e	�폜�Ώۂ̃G���e�B�e�B 
	 */
	virtual void RemoveComponent(Entity e) = 0;
};

/**
 * @class Store<T>
 * @brief ����̃R���|�[�l���g�^T���i�[����X�g�A
 * 
 * @tparam T IComponent���p�������R���|�[�l���g�^
 * @details
 * std::unordered_map���g�p����EntityID�ƃR���|�[�l���g�̎��f�[�^��Ή��t���܂��B
 * ����ɂ��A�R���|�[�l���g�̓��I�Ȓǉ��E�폜���_��ɍs���܂��B
 * 
 * @warning T��IComponent���p�����Ă���K�v������܂��B
 */
template<typename T>
class Store : public IStore
{
public:
    /**
     * @brief ����̃G���e�B�e�B�ɃR���|�[�l���g��ǉ����܂��B
     * @param[in] e �ΏۃG���e�B�e�B
     * @param[in] args �R���|�[�l���gT�̃R���X�g���N�^����
     * @return T& �ǉ����ꂽ�R���|�[�l���g�ւ̎Q��
     * 
     * @code
     * w.GetStore<Transform>()->Add(entity, position, rotation);
     * @endcode
     */
    template<typename... Args>
    T& Add(Entity e, Args&&... args)
    {
        // �z�unew���g�p���āA�����̃������i��������΁j���ė��p���邩�A�V�����\�z
        auto result = data_.try_emplace(e.id, std::forward<Args>(args)...);

        // ���ɃR���|�[�l���g�����݂���ꍇ�́A�㏑�����Ă��܂�
        if (!result.second) {
            // �����̗v�f�̃f�X�g���N�^���Ăяo���A�V�����v�f���\�z
            result.first->second.~T();
            new (&result.first->second) T(std::forward<Args>(args)...);
        }

        return result.first->second;
    }

    /**
     * @brief ����̃G���e�B�e�B����R���|�[�l���g���擾���܂��B
     * @param[in] e �ΏۃG���e�B�e�B
     * @return T& �R���|�[�l���g�ւ̎Q��
     * 
     * @warning �R���|�[�l���g�����݂��Ȃ��ꍇ�A�A�T�[�g�i���s���G���[�j���������܂��B
     */
    T& Get(Entity e)
    {
        assert(Has(e) && "Component does not exist on this Entity!");
        return data_.at(e.id);
    }

    /**
     * @brief ����̃G���e�B�e�B�̃R���|�[�l���g�����݂��邩���m�F���܂��B
     */
    bool Has(Entity e) const
    {
        return data_.count(e.id);
    }

    /**
     * @brief ����̃G���e�B�e�B����R���|�[�l���g���폜���܂��B
     */
    void Remove(Entity e)
    {
        data_.erase(e.id);
    }

    /**
     * @brief IStore�C���^�[�t�F�[�X�̎����B�G���e�B�e�B�j�����ɌĂ΂�܂��B
     */
    void RemoveComponent(Entity e) override
    {
        Remove(e);
    }

    // �`��Ȃǂ̃��[�v�ŃC�e���[�V�������邽�߂ɃX�g�A�S�̂ւ̃A�N�Z�X���
    const std::unordered_map<Entity::ID, T>& GetAll() const { return data_; }

private:
    // Entity ID -> �R���|�[�l���g�f�[�^ �̃}�b�s���O
    std::unordered_map<Entity::ID, T> data_;
};


// --------------------------------------------------
// 2. World �N���X (ECS�̒��j)
// --------------------------------------------------

/**
 * @class World
 * @brief �G���e�B�e�B�ƃR���|�[�l���g���Ǘ�����ECS�̒��j�N���X
 * 
 * @details
 * World�N���X�́AEntity ID�̃��C�t�T�C�N���Ǘ��A�S�ẴR���|�[�l���g�X�g�A�A
 * �����Behaviour�R���|�[�l���g�̍X�V��S�����܂��B
 * 
 * @par World�ւ̃A�N�Z�X:
 * System��Behaviour�R���|�[�l���g�́A����World�N���X��ʂ���
 * ����Entity�̃R���|�[�l���g�ɃA�N�Z�X���܂��B
 * 
 * @par �g�p��:
 * @code
 * World world;
 * // ��������Setup()�Ȃǂōs��
 * world.Update(deltaTime);
 * @endcode
 * 
 * @see EntityBuilder �G���e�B�e�B�쐬�̂��߂̃r���_�[
 */
class World
{
public:
    World() = default;
    ~World() { /* �S�Ă�IStore�̃|�C���^���폜���鏈�����K�v */ }

    // ------------------------------------
    // Entity�̃��C�t�T�C�N��
    // ------------------------------------

    /**
     * @brief �V�����G���e�B�e�BID�𐶐����܂��B
     * @return Entity �V�����쐬���ꂽ�G���e�B�e�B
     * 
     * @note ���̊֐��𒼐ڎg�p�������ɁACreate()���\�b�h���g�p���A
     * EntityBuilder��ʂ��ăR���|�[�l���g��ǉ����邱�Ƃ𐄏����܂��B
     */
    Entity CreateEntity()
    {
        Entity::ID id = ++nextId_;
        alive_[id] = true;
        return { id };
    }

    /**
     * @brief �G���e�B�e�B��j�����A�֘A����S�ẴR���|�[�l���g���폜���܂��B
     * @param[in] e �j���Ώۂ̃G���e�B�e�B
     * 
     * @warning �G���e�B�e�B���L���iIsValid() == true�j�ł��邱�Ƃ��m�F���Ă��������B
     */
    void DestroyEntity(Entity e)
    {
        if (!e.IsValid() || !alive_.count(e.id)) return;

        // 1. �S�Ă�IStore����R���|�[�l���g���폜
        for (const auto& func : erasers_) {
            func(e);
        }

        // 2. Behaviour�R���|�[�l���g�����X�g����폜
        unregisterBehaviour(e);

        // 3. ID�����
        alive_.erase(e.id);
    }

    // ------------------------------------
    // Component�̑���
    // ------------------------------------

    /**
     * @brief ����̃R���|�[�l���g�^T�̃X�g�A���擾���܂��B
     * @tparam T IComponent���p�������R���|�[�l���g�^
     * @return Store<T>* �X�g�A�ւ̃|�C���^
     * 
     * @warning �R���|�[�l���g�^�����o�^�̏ꍇ�A�X�g�A��V�K�쐬���A�|�C���^��Ԃ��܂��B
     * ����͒ʏ�A���������ɂ̂ݎ��s�����ׂ��ł��B
     */
    template<typename T>
    Store<T>* GetStore()
    {
        std::type_index typeID = GetComponentTypeID<T>();

        if (stores_.count(typeID) == 0)
        {
            // �X�g�A���܂����݂��Ȃ��ꍇ�A�V�����쐬���A�}�b�v�ɓo�^
            Store<T>* newStore = new Store<T>(); // new �ŃC���X�^���X��
            stores_[typeID] = newStore;

            // �폜�p�֐��i�C���C�T�[�j���o�^
            erasers_.push_back([this, typeID](Entity e) {
                // RemoveComponent���Ăяo�����߂̃����_�֐�
                auto it = stores_.find(typeID);
                if (it != stores_.end()) {
                    it->second->RemoveComponent(e);
                }
                });
            return newStore;
        }
        // �_�E���L���X�g���ĕԂ�
        return static_cast<Store<T>*>(stores_.at(typeID));
    }

    /**
     * @brief ����̃G���e�B�e�B����R���|�[�l���g���擾���܂��B
     * @tparam T IComponent���p�������R���|�[�l���g�^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return T& �R���|�[�l���g�ւ̎Q��
     * 
     * @warning �R���|�[�l���g�����݂��Ȃ��ꍇ�A�A�T�[�g�i���s���G���[�j���������܂��B
     */
    template<typename T>
    T& Get(Entity e)
    {
        return GetStore<T>()->Get(e);
    }

    /**
     * @brief ����̃G���e�B�e�B����R���|�[�l���g���擾���܂��B���݂��Ȃ��ꍇ��nullptr��Ԃ��܂��B
     * @tparam T IComponent���p�������R���|�[�l���g�^
     * @param[in] e �ΏۃG���e�B�e�B
     * @return T* �R���|�[�l���g�ւ̃|�C���^�B���݂��Ȃ��ꍇ��nullptr�B
     */
    template<typename T>
    T* TryGet(Entity e)
    {
        Store<T>* store = GetStore<T>();
        if (store->Has(e)) {
            return &store->Get(e);
        }
        return nullptr;
    }

    // ------------------------------------
    // World�̍X�V���W�b�N (��̃X�e�b�v�Ŏ���)
    // ------------------------------------
    void Update(float dt)
    {
        // 1. WorldMatrix�̌v�Z
        // 2. Behaviour Component��OnStart/OnUpdate�Ăяo�� (���̃X�e�b�v)
    }

    /**
     * @brief �V����Entity�𐶐����A�r���_�[���J�n���܂��B
     * @return EntityBuilder Entity�\�z�̂��߂̃r���_�[�C���X�^���X
     */
    EntityBuilder Create()
    {
        Entity e = CreateEntity();
        return { *this, e };
    }

    // ------------------------------------
// World�̍X�V���W�b�N
// ------------------------------------

/**
 * @brief World�ɑ��݂��邷�ׂĂ�Behaviour�R���|�[�l���g���X�V���܂��B
 * 
 * @param[in] dt �f���^�^�C���i�O�t���[������̌o�ߕb���j
 * 
 * @details
 * ���̊֐��̓Q�[���̃��C�����[�v���疈�t���[���Ă΂��ׂ��ł��B
 * �ȉ��̏��������̏����Ŏ��s���܂��F
 * 1. WorldMatrix�̌v�Z�iTransform�̍X�V�j
 * 2. �����s��Behaviour�R���|�[�l���g�ɑ΂���OnStart()���Ăяo��
 * 3. �S�Ă�Behaviour�R���|�[�l���g�ɑ΂���OnUpdate(dt)���Ăяo��
 * 
 * @par �g�p��:
 * @code
 * // ���C�����[�v��
 * float deltaTime = timer.GetDeltaTime();
 * world.Update(deltaTime);
 * @endcode
 */
    void Update(float dt)
    {
        // 1. WorldMatrix�̌v�Z�i����̃X�e�b�v��RenderSystem�ƘA�g���邪�A�����ł�Transform�R���|�[�l���g�݂̂��X�V�j
        // ����̐݌v�ł́ATransform�̍X�V�i��]�Ȃǁj��Behaviour�R���|�[�l���g�iRotator�Ȃǁj���S�����A
        // WorldMatrix�̌v�Z��RenderSystem�̂悤�Ȑ�p�V�X�e�����S�����܂��B
        // �������ARotator�̂悤��Behaviour�R���|�[�l���g�����g��Transform���X�V�����ꍇ�A
        // �`��O��WorldMatrix���X�V�����K�v������܂��B

        // �� ���̐݌v�ł́AWorldMatrix�̌v�Z��RenderSystem���s���O��RenderSystem���ōs���̂��œK�ł��B
        //    �����ł�Behaviour�̍X�V�ɏW�����ATransform�̍X�V��Behaviour�R���|�[�l���g�ɔC���܂��B

        // 2. Behaviour Component��OnStart/OnUpdate�Ăяo��
        for (auto& entry : behaviours_)
        {
            // Behaviour���t���Ă���G���e�B�e�B���܂��������Ă��邩���m�F�i���ȈՎ����BDestroyEntity�Ń��X�g����폜����邽�ߒʏ�͕s�v�j
            if (!alive_.count(entry.e.id)) continue;

            // 2-1. ������s��: OnStart���Ăяo��
            if (!entry.started)
            {
                entry.b->OnStart(*this, entry.e);
                entry.started = true;
            }

            // 2-2. ���t���[�����s: OnUpdate���Ăяo��
            entry.b->OnUpdate(*this, entry.e, dt);
        }
    }

    // EntityBuilder����̂݃A�N�Z�X�����v���C�x�[�g�֐��⃁���o
private:
    // World�N���X��private�Z�N�V�����ɒǉ�
    // Behaviour�Ǘ��p�G���g��
    struct BEntry {
        Entity e;           ///< �G���e�B�e�B
        Behaviour* b;       ///< Behaviour�ւ̃|�C���^
        bool started = false; ///< OnStart���Ă΂ꂽ��
    };

    // Behaviour���X�g
    std::vector<BEntry> behaviours_;

    /**
     * @brief Behaviour�R���|�[�l���g��World�̍X�V���X�g�ɓo�^���܂��B
     * @param[in] e �ΏۃG���e�B�e�B
     * @param[in] b �o�^����Behaviour�R���|�[�l���g�ւ̃|�C���^
     */
    void registerBehaviour(Entity e, Behaviour* b)
    {
        behaviours_.push_back({ e, b, false });
    }

    /**
     * @brief Behaviour�̓o�^���������܂��B�iEntity�j�����Ɏg�p�j
     * @param[in] e �o�^��������G���e�B�e�B
     */
    void unregisterBehaviour(Entity e)
    {
        // EntityID����v������̂����X�g����폜�i���萫�̂���erase-remove�C�f�B�I���̊ȈՔŁj
        for (auto it = behaviours_.begin(); it != behaviours_.end(); ++it) {
            if (it->e.id == e.id) {
                // Behaviour�̓|�C���^�Ȃ̂ŁA�����Ń���������͍s���܂���iStore���S���j
                behaviours_.erase(it);
                break;
            }
        }
    }

    uint32_t nextId_ = 0;                              ///< ���̃G���e�B�e�BID
    std::unordered_map<uint32_t, bool> alive_;         ///< �G���e�B�e�B�̐������
    std::unordered_map<std::type_index, IStore*> stores_; ///< �R���|�[�l���g�X�g�A
    std::vector<std::function<void(Entity)>> erasers_;  ///< �폜�p�֐����X�g

    // Behaviour���X�g�͎��̃X�e�b�v�Ŏ���

    friend class EntityBuilder;
};

#endif // !___WORLD_H___