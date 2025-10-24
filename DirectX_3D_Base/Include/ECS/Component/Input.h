/*****************************************************************//**
 * @file	Input.h
 * @brief	入力状態を保持するコンポーネント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___INPUT_H___
#define ___INPUT_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include <DirectXMath.h>
#include <map>
#include <string>

/**
 * @struct	Input
 * @brief	ユーザーからの入力状態（移動、アクションなど）を保持するコンポーネント
 * @note	InputSystemによって値が書き込まれ、MovementSystemなどによって読み取られます。
 */
struct Input
	: public IComponent
{
    // --------------------------------------------------
    // 移動関連の入力
    // --------------------------------------------------

    // プレイヤーの移動ベクトル（正規化された方向、例: 前方(0,0,1), 後方(0,0,-1)）
    DirectX::XMFLOAT3 movementVector = { 0.0f, 0.0f, 0.0f };

    // どのキー/ボタンが現在押されているかを管理するフラグ
    bool isMovingForward = false;
    bool isMovingBackward = false;
    bool isMovingLeft = false;
    bool isMovingRight = false;

    // --------------------------------------------------
    // アクション関連の入力
    // --------------------------------------------------

    bool isAction1Pressed = false; // 例: 攻撃ボタン
    bool isJumpPressed = false; // 例: ジャンプボタン

    // --------------------------------------------------
    // マウス関連の入力 (カメラ操作などに利用)
    // --------------------------------------------------

    float mouseDeltaX = 0.0f; // 前フレームからのX軸移動量
    float mouseDeltaY = 0.0f; // 前フレームからのY軸移動量
    bool isMouseRightClick = false; // 右クリックの状態

    // 処理後に入力値をリセットする必要があるかどうか
    bool shouldResetAfterUse = true;
};

#endif // !___INPUT_H___