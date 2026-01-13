/*****************************************************************//**
 * @file	SonarComponent.h
 * @brief	波紋エフェクト
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/05	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SONAR_COMPONENT_H___
#define ___SONAR_COMPONENT_H___

struct SonarComponent
{
	float timer;        // 経過時間
	float maxTime;      // 消えるまでの時間
	float startScale;   // 初期サイズ
	float maxScale;     // 最大サイズ

	SonarComponent(float duration = 1.5f, float start = 0.0f, float end = 500.0f)
		: timer(0.0f), maxTime(duration), startScale(start), maxScale(end)
	{
	}
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(SonarComponent)

#endif