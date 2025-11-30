/*****************************************************************//**
 * @file	UIInteractableComponent.h
 * @brief	UIのインタラクション(クリック等)状態を管理するコンポーネント
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author Oda Kaito
 * ------------------------------------------------------------
 *
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 *
 *
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/


#ifndef __UI_INTERACTABLE_COMPONENT_H__
#define __UI_INTERACTABLE_COMPONENT_H__

#include <string>
#include "ECS/ComponentRegistry.h"
using namespace std;

struct UIInteractableComponent
{
	//--- 設定値 ---
	// 判定サイズ (TransformのScaleとは別に当たり判定を持ちたい場合に使用。マイナスの場合はTransform.scaleを使用する設定とみなす)
	float width = -1.0f;
	float height = -1.0f;

	// クリック時に発行するイベントＩＤ(任意)
	string onClickEventId = "";

	//アニメーション用の基本スケール
    float baseScaleX = 1.0f;
    float baseScaleY = 1.0f;
    bool doHoverAnim = true; // ホバーアニメーションをするか？


	// --- 状態 (システムが書き込む) ---
	bool isHovered = false; // マウスが乗っているか
	bool isClicked = false; // クリックされた瞬間か
	bool isPressed = false; // 押され続けているか

	//演出進行フラグ（システムが書き込む）
	bool isTransitionExpanding = false;

	UIInteractableComponent(float w = -1.0f, float h = -1.0f, bool anim = true)
		:width(w), height(h)
	{

	}




	/*UIInteractableComponent(const string& eventId = "")
		:onClickEventId(eventId)
	{

	}*/


};

REGISTER_COMPONENT_TYPE(UIInteractableComponent)

#endif // __UI_INTERACTABLE_COMPONENT_H__