/*****************************************************************//**
 * @file	ECSInitializer.cpp
 * @brief	ECSVXeŜ̏W񂵁AV[Init()Ӗ𕪗邽߂̃wp[NX̎
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/10/31	쐬
 * 			ƓeF	- ǉF
 *
 * @update	2025/11/08	ŏIXV
 * 			ƓeF	- ǉFxAI̒ǉ
 *
 * @note	iȗj
 *********************************************************************/

 // ===== CN[h =====
#include "ECS/ECSInitializer.h"
#include "ECS/AllComponents.h"
#include "ECS/AllSystems.h"

// Screen transition (card-tilt fade)
#include "ECS/Components/Core/ScreenTransitionComponent.h"
#include "ECS/Systems/Core/ScreenTransitionSystem.h"

#include "ECS/Systems/UI/UIInputSystem.h"
#include "ECS/Components/UI/UIButtonComponent.h"
#include "ECS/Systems/Core/ResultControlSystem.h"


#include <iostream>

using namespace ECS;

// ÓIo[ϐ s_systems ̎̂`Amۂ
std::unordered_map<std::type_index, std::shared_ptr<ECS::System>> ECS::ECSInitializer::s_systems;

/**
 * [void - RegisterComponents]
 * @brief	SẴR|[lgCoordinatorɓo^B
 *
 * @param	[in] coordinator
 */
void ECSInitializer::RegisterComponents(Coordinator* coordinator)
{
    // R|[lg̓o^iœo^j
    for (const auto& registerFn : GetComponentRegisterers())
    {
        registerFn(coordinator);
    }

    std::cout << "ECSInitializer: All Components registered." << std::endl;
}

void ECSInitializer::RegisterSystemsAndSetSignatures(Coordinator* coordinator)
{
    // ============================================================
    // VXe̓o^ƃVOl`̐ݒi牺ɒǉj
    // o^ɃVXesB
    // ============================================================

    // ------------------------------------------------------------
    // 1. UpdateiXVj
    // ------------------------------------------------------------

    // @system  PlayerControlSystem
    // @brief   L[́ARg[[
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PlayerControlSystem,
        /* Components   */  PlayerControlComponent, TransformComponent, RigidBodyComponent, AnimationComponent
    );

    // @system  PhysicsSystem
    // @brief   vZiʒu̍XVj
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  PhysicsSystem,
        /* Components   */  RigidBodyComponent, TransformComponent, CollisionComponent
    );

    // @system  CollectionSystem
    // @brief   ACeWbN
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollectionSystem,
        /* Components   */  CollectableComponent, TransformComponent
    );

    // @system  CollisionSystem
    // @brief   ՓˌoƉiʒȕCj
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CollisionSystem,
        /* Components   */  CollisionComponent, TransformComponent, RigidBodyComponent
    );

    // @system  GameControlSystem
    // @brief   Q[Xe[g
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GameControlSystem,
        /* Components   */  GameStateComponent
    );

    // @system  CameraControlSystem
    // @brief   Jir[EvWFNVs̍XVj
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  CameraControlSystem,
        /* Components   */  CameraComponent
    );

    // @system  BasicCameraSystem
    // @brief   ŒJ
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  BasicCameraSystem,
        /* Components   */  BasicCameraComponent, TransformComponent
    );

#ifdef _DEBUG
    // @system  DebugDrawSystem
    // @brief   fobO`VXe
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  DebugDrawSystem,
        /* Components   */  DebugComponent
    );
#endif

    // @system  GuardAISystem
    // @brief   xAI
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  GuardAISystem,
        /* Components   */  GuardComponent, TransformComponent, RigidBodyComponent
    );

    // @system UIInoutSystem
    // @brief  }EXJ[\̔
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        UIInputSystem,
        UIButtonComponent, TransformComponent
    );

    // @system  CursorSystem
    // @brief   J[\UI
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        CursorSystem,
        UICursorComponent, TransformComponent
    );

    // @system  AudioSystem
    // @brief   Đ
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  AudioSystem,
        /* Components   */  SoundComponent
    );

    // @system  AnimationSystem
    // @brief   Aj[VXV
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  AnimationSystem,
        /* Components   */  ModelComponent, AnimationComponent
    );

    // @system  LifeTimeSystem
    // @brief   
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  LifeTimeSystem,
        /* Components   */  LifeTimeComponent
    );

    // @system  TitleControlSystem
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  TitleControlSystem,
        /* Components   */  TitleControllerComponent
    );

    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        ResultControlSystem,
        TagComponent, UIButtonComponent

    );
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        OpeningControlSystem,
        TagComponent, UIButtonComponent

    );

    // 2. VXeo^ƃVOl`ݒ
    REGISTER_SYSTEM_AND_INIT(
        coordinator,
        FloatingSystem,
        TransformComponent, FloatingComponent
    );

    // @system  EnemySpawnSystem
    // @brief   x
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  EnemySpawnSystem,
        /* Components   */  EnemySpawnComponent
    );

    // @system  EffectSystem
    // @brief   GtFNg
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  EffectSystem,
        /* Components   */  EffectComponent, TransformComponent
    );

    // @system  ScreenTransitionSystem
    // @brief   画面遷移（カード斜めフェード）: Transform + UIImage + ScreenTransitionComponent を更新
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  ScreenTransitionSystem,
        /* Components   */  TransformComponent, UIImageComponent, ScreenTransitionComponent
    );


    // ------------------------------------------------------------
    // 2. Drawi`揈j
    // ------------------------------------------------------------

    // @system  FlickerSystem
    // @brief   _
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  FlickerSystem,
        /* Components   */  FlickerComponent
    );

    // @system  RenderSystem
    // @brief   JݒAfobOObh`恕Entities̕`
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  RenderSystem,
        /* Components   */  RenderComponent, TransformComponent
    );

    // @system  UIRenderSystem
    // @brief   UI̕`
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  UIRenderSystem,
        /* Components   */  UIImageComponent
    );

    // ------------------------------------------------------------
    // 3. ̑UpdatesȂVXe
    // ------------------------------------------------------------

    // @system  MapGenerationSystem
    // @brief   _}bv𐶐
    REGISTER_SYSTEM_AND_INIT(
        /* Coordinator  */  coordinator,
        /* System       */  MapGenerationSystem,
        /* Components   */  MapComponent
    );

    std::cout << "ECSInitializer: All Systems registered and initialized." << std::endl;
}

/**
 * [void - InitECS]
 * @brief	CoordinatorSystem֘AtGg|CgB
 *
 * @param	[in] coordinator
 */
void ECSInitializer::InitECS(std::shared_ptr<Coordinator>& coordinator)
{
    // Coordinator̐|C^擾
    Coordinator* rawCoordinator = coordinator.get();

    // 1. Coordinator̂̏ (ECSRÃf[^\̏)
    rawCoordinator->Init();

    // 2. R|[lg̓o^
    RegisterComponents(rawCoordinator);

    // 3. VXe̓o^ƃVOl`̐ݒ (ÓI}bvɊi[)
    RegisterSystemsAndSetSignatures(rawCoordinator);
}

/**
 * @brief ECSɊ֘ASĂ̐ÓI\[XN[AbvB
 */
void ECSInitializer::UninitECS()
{
    s_systems.clear(); // SẴVXeSharedPtr
}