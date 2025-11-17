/*****************************************************************//**
 * @file	TagComponent.h
 * @brief	Entityに特定の役割を示すタグを付与するComponent。
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/06	初回作成日
 * 			作業内容：	- 追加：文字列によるEntityの役割識別用TagComponentを追加。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TAG_COMPONENT_H___
#define ___TAG_COMPONENT_H___

#include <string>

/**
 * @struct TagComponent
 * @brief Entityに一意の識別子（タグ）を付与する
 */
struct TagComponent
{
	std::string tag; ///< エンティティの役割（例: "player", "guard", "item"）

	TagComponent(const std::string& entityTag = "default") : tag(entityTag) {}
};

// Componentの自動登録 (このフレームワークの標準機能と仮定)
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TagComponent)

#endif // !___TAG_COMPONENT_H___