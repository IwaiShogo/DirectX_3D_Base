/*****************************************************************//**
 * @file	ScanLineComponent.h
 * @brief	
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

#ifndef ___SCAN_LINE_COMPONENT___
#define ___SCAN_LINE_COMPONENT___

struct ScanLineComponent
{
	float speed;        // 移動速度 (pixels/sec)
	float startY;       // 開始Y座標
	float endY;         // 終了Y座標
	bool movingDown;    // 現在の移動方向

	ScanLineComponent(float spd = 200.0f, float start = 0.0f, float end = 720.0f)
		: speed(spd), startY(start), endY(end), movingDown(true)
	{
	}
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(ScanLineComponent)

#endif // !___SCAN_LINE_COMPONENT___