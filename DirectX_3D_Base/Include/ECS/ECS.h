/*****************************************************************//**
 * @file	ECS.h
 * @brief	ECS全体集約ヘッダーファイル
 * 
 * @details	
 * このファイルをインクルードするだけで、必要なECS関連の機能がすべて利用可能になります。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/31	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ECS_H___
#define ___ECS_H___

// ===== インクルード =====
// ECSの共通定義とTypedef
#include "Types.h"
#include "Coordinator.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

// コンポーネントとシステムの集約
#include "AllComponents.h"
#include "AllSystems.h"

#endif // !___ECS_H___