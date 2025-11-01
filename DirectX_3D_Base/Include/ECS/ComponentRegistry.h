/*****************************************************************//**
 * @file	ComponentRegistry.cpp
 * @brief	Meyers' Singletonパターンを使用して安全にグローバルな登録リストを管理します。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/01	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COMPONENT_REGISTRY_H___
#define ___COMPONENT_REGISTRY_H___

 // ===== インクルード =====
#include "Coordinator.h"
#include <vector>
#include <functional>
#include <iostream>

namespace ECS
{
    // Componentの登録処理の型定義
    using ComponentRegisterFn = std::function<void(Coordinator*)>;

    /**
     * @brief Component登録関数リストへの安全なアクセサ (Meyers' Singletonパターン)
     * @details Componentの型登録関数が静的に追加されるリストを返します。
     */
    inline std::vector<ComponentRegisterFn>& GetComponentRegisterers()
    {
        // 静的ローカル変数として定義することで、初期化と破壊順序の安全性を保証
        static std::vector<ComponentRegisterFn> s_registerers;
        return s_registerers;
    }

    /**
     * @brief Componentの自動登録ヘルパーマクロ
     * @details 各Componentヘッダーの静的初期化子を利用して、登録関数をリストに追加します。
     */
#define REGISTER_COMPONENT_TYPE(ComponentType) \
    namespace { \
        /* ComponentTypeIDの一意性を確保するため、静的変数名にComponentTypeを組み込む */ \
        static struct ComponentRegistrationHelper_##ComponentType { \
            ComponentRegistrationHelper_##ComponentType() { \
                /* GetComponentRegisterers()を呼び出し、リストに登録関数を追加 */ \
                ECS::GetComponentRegisterers().push_back([](ECS::Coordinator* coord) { \
                    coord->RegisterComponentType<ComponentType>(); \
                    /* std::cout << "Component Registered: " #ComponentType << std::endl; */ \
                }); \
            } \
        } s_componentReg_##ComponentType; \
    }
}

#endif // !___COMPONENT_REGISTRY_H___